// This is sooo weird
//
// hours wasted : 18 
//
// Note:  adapting betasort "pause and correlate" mechanism, prune old implant, finalize decays after window passes. flush at end. 
//              Keeping my custom correlation logic though. per-isotope window, 3x3 window, flagging ambiguos correlations, forward - backward subtraction
//
// fix bug - search window is 2*tc but decay is flushed every maxTc, stupid mistake (* _ *) 

#include <cstdio>
#include <deque>
#include <vector>
#include <string>
#include <cmath>
#include <filesystem>
#include <chrono>

#include <TChain.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <globals.h>

// configuration
static const char* IsotopeName[1] = {"Na"};
static const double HalfLife[1] = {13.2};   // half-life (in ms) taken from  https://www.nndc.bnl.gov/nudat3/
static const double Ticks_per_ms = 100000.0;     // 10 ns/tick → 100,000 ticks per ms

// calculating tc for each isotope
static double tcTicks(int iso) { return 10.0 * HalfLife[iso] * Ticks_per_ms; } // k - value 
// static double tcTicks(int iso) { return 100000000.0; }   // 1 sec hardcoded = 1e8 ticks (10 ns/tick)
static double maxTc() {
    double m = 0;
    for(int i = 0; i < 1; i++) m = std::max(m, tcTicks(i));  
    return tcTicks(0);
}

struct Event {
  int      type;        // 0 = implant, 1 = decay
  long long id;
  int      isotope;
  double   x, y;
  double   timestamp;
  double   energy;
  int      run;
  std::vector<double>* gammas = nullptr;   // TChain sets this pointer
};

struct Implant {
  long long id;
  int    isotope;
  int    x, y;
  double t;
  double energy;
};

struct Decay {
  long long id;
  int    x, y;
  double t;
  double energy;
  std::vector<double> gammas;
};

struct Correlation {
  long long implantId, decayId;
  int    isotope;
  int    xI, yI, xD, yD;
  double tI, tD, dt;
  int    dx, dy;
  double decayEnergy;
  bool   samePixel;
  int    nCandidates;
  bool   accepted;
  bool   ambiguous;
  bool   backward;      // true = background correlation
  std::vector<double> gammas;
};


// Correlation
class Correlator {
public:
    Correlator() {
        for(int i = 0; i < 1; i++) {
            double Window = 10.0 * HalfLife[i];       // per-Isotope forward dt spectrum
            fFwd[i] = new TH1D(Form("fwd_%s", IsotopeName[i]), Form("forward dt %s;dt (ms);counts", IsotopeName[i]),
                         1000, 0, Window);            // per-Isotope backward dt spectrum
            fBwd[i] = new TH1D(Form("bwd_%s", IsotopeName[i]), Form("backward dt %s;dt (ms);counts", IsotopeName[i]),
                         1000, 0, Window);}
    }


  // an implant arrives: store it in its pixel
  void AddImplant(const Event& e) {
    Implant im{e.id, e.isotope, (int)std::lround(e.x), (int)std::lround(e.y), e.timestamp, e.energy};
    if(im.isotope < 0 || im.isotope >= 1) return;
    if(im.x < 0 || im.x >= 40 || im.y < 0 || im.y >= 40) return;
    fGrid[im.x][im.y].push_back(im);
  }

    void ProcessDecayForward(const Event& e) {
    int xD = (int)std::lround(e.x), yD = (int)std::lround(e.y);
    if(xD < 0 || xD >= 40 || yD < 0 || yD >= 40) return;

    std::vector<Implant> candidates;

    for(int ix = std::max(0, xD-1); ix <= std::min(39, xD+1); ++ix) {
      for(int iy = std::max(0, yD-1); iy <= std::min(39, yD+1); ++iy) {
        auto& list = fGrid[ix][iy];
        for(auto& im : list) {
          double dt = e.timestamp - im.t;
          if(dt > 0 && dt <= tcTicks(im.isotope)) {
            candidates.push_back(im);
            double dtMs = dt / Ticks_per_ms;
            fFwd[im.isotope]->Fill(dtMs);        // fill EVERY forward pair (matches backward)
          }
        }
      }
    }

    // records: keep the accepted/ambiguous bookkeeping (parent ID), separate from the spectrum
    int n = candidates.size();
    if(n == 1) {
      WriteRecord(candidates[0], e, n, /*accepted*/true, /*ambiguous*/false, /*backward*/false);
    } else if(n > 1) {
      for(const auto& im : candidates)
        WriteRecord(im, e, n, false, true, false);
    }
  }


  // hold decays for the backward search
  void AddDecay(const Event& e) {
    Decay d{e.id, (int)std::lround(e.x), (int)std::lround(e.y), e.timestamp, e.energy,
            e.gammas ? *e.gammas : std::vector<double>{}};
    if(d.x < 0 || d.x >= 40 || d.y < 0 || d.y >= 40) return;
    fDecayGrid[d.x][d.y].push_back(d);
  }

  // BACKWARD search for one implant about to be retired:
  // decays in its 3x3 with -tc <= (tD - tI) < 0
  void BackwardSearch(const Implant& im) {
    for(int ix = std::max(0, im.x-1); ix <= std::min(39, im.x+1); ++ix) {
      for(int iy = std::max(0, im.y-1); iy <= std::min(39, im.y+1); ++iy) {
        for(const auto& d : fDecayGrid[ix][iy]) {
          double dt = d.t - im.t;
          if(dt < 0 && dt >= -tcTicks(im.isotope)) {
            double dtMs = std::abs(dt) / Ticks_per_ms;
            fBwd[im.isotope]->Fill(dtMs);      // fill EVERY backward decay, no uniqueness
          }
        }
      }
    }
  }


  // retirement: called as current_time advances.
  // an implant is safe to retire once current_time > im.t + tc (no future
  // decay can still be in its forward window). At retirement, do its
  // backward search (all earlier decays are known).
  void Retire(double current_time) {
    for(int x = 0; x < 40; ++x)
      for(int y = 0; y < 40; ++y) {
        auto& list = fGrid[x][y];
        while(!list.empty() && current_time - list.front().t > tcTicks(list.front().isotope)) {
          BackwardSearch(list.front());
          list.pop_front();
        }
      }
    // evict decays older than 2 * max window (no implant will look back that far)
      for(int x = 0; x < 40; ++x)
      for(int y = 0; y < 40; ++y) {
        auto& dlist = fDecayGrid[x][y];
        while(!dlist.empty() && current_time - dlist.front().t > 3.0 * maxTc())
          dlist.pop_front();
  }
}

  void Flush() {
    // end of data: retire everything remaining
    for(int x = 0; x < 40; ++x)
      for(int y = 0; y < 40; ++y)
        for(auto& im : fGrid[x][y])
          BackwardSearch(im);
  }

  void Write() {
    for(int i = 0; i < 1; i++) {
      fFwd[i]->Write();
      fBwd[i]->Write();
      TH1D* tru = (TH1D*)fFwd[i]->Clone(Form("true_%s", IsotopeName[i]));
      tru->Add(fBwd[i], -1.0);
      tru->Write();
    }
    fCorrTree->Write();
  }

  void SetupTree(TFile* f) {
    f->cd();
    fCorrTree = new TTree("correlations", "implant-decay correlations");
    fCorrTree->Branch("implantId",       &fRec.implantId);
    fCorrTree->Branch("decayId",         &fRec.decayId);
    fCorrTree->Branch("isotope",         &fRec.isotope);
    fCorrTree->Branch("xI",              &fRec.xI); 
    fCorrTree->Branch("yI",              &fRec.yI);
    fCorrTree->Branch("xD",              &fRec.xD); 
    fCorrTree->Branch("yD",              &fRec.yD);
    fCorrTree->Branch("tI",              &fRec.tI); 
    fCorrTree->Branch("tD",              &fRec.tD);
    fCorrTree->Branch("dt",              &fRec.dt);
    fCorrTree->Branch("dx",              &fRec.dx); 
    fCorrTree->Branch("dy",              &fRec.dy);
    fCorrTree->Branch("decayEnergy",     &fRec.decayEnergy);
    fCorrTree->Branch("samePixel",       &fRec.samePixel);
    fCorrTree->Branch("nCandidates",     &fRec.nCandidates);
    fCorrTree->Branch("accepted",        &fRec.accepted);
    fCorrTree->Branch("ambiguous",       &fRec.ambiguous);
    fCorrTree->Branch("gammas",          &fRec.gammas);
  }

private:
  void WriteRecord(const Implant& im, const Event& e, int n, bool acc, bool amb, bool bwd) {
    fRec.implantId = im.id;   fRec.decayId = e.id;   fRec.isotope = im.isotope;
    fRec.xI = im.x; fRec.yI = im.y;
    fRec.xD = (int)std::lround(e.x); fRec.yD = (int)std::lround(e.y);
    fRec.tI = im.t; fRec.tD = e.timestamp; fRec.dt = e.timestamp - im.t;
    fRec.dx = fRec.xD - im.x; fRec.dy = fRec.yD - im.y;
    fRec.decayEnergy = e.energy;
    fRec.samePixel = (fRec.dx == 0 && fRec.dy == 0);
    fRec.nCandidates = n;
    fRec.accepted = acc; fRec.ambiguous = amb;
    fRec.gammas = e.gammas ? *e.gammas : std::vector<double>{};
    fCorrTree->Fill();
  }

  std::deque<Implant> fGrid[40][40];
  std::deque<Decay>   fDecayGrid[40][40];
  TH1D* fFwd[1];   // same changes here 4 -> 1 , only see the sodium blob
  TH1D* fBwd[1];   
  TTree* fCorrTree = nullptr;
  Correlation fRec;
};

// ================= main =================
int main(int argc, char** argv) {
  if(argc < 2) { printf("usage: correlate tree1.root [tree2.root ...]\n"); return 1; }

  TChain chain("events");
  for(int i = 1; i < argc; ++i) chain.Add(argv[i]);

  Event e;
  chain.SetBranchAddress("type",      &e.type);
  chain.SetBranchAddress("id",        &e.id);
  chain.SetBranchAddress("isotope",   &e.isotope);
  chain.SetBranchAddress("x",         &e.x);
  chain.SetBranchAddress("y",         &e.y);
  chain.SetBranchAddress("timestamp", &e.timestamp);
  chain.SetBranchAddress("energy",    &e.energy);
  chain.SetBranchAddress("run",       &e.run);
  chain.SetBranchAddress("gammas",    &e.gammas);


  std::filesystem::path p(argv[1]);
  std::string stem = p.stem().string();
  int run = -1;
  std::sscanf(stem.c_str(), "tree%d", &run);
  std::string ofile = Form("correlation%04d.root", run);

  TFile out(ofile.c_str(), "recreate");
  Correlator corr;
  corr.SetupTree(&out);

  Long64_t nEntries = chain.GetEntries();
  printf("correlating %lld events...\n", nEntries);

  auto lastTime = std::chrono::steady_clock::now();  
  Long64_t lastEntry = 0; 

  for(Long64_t i = 0; i < nEntries; ++i) {
    chain.GetEntry(i);

    if(e.type == 0) {                 // implant
      corr.AddImplant(e);
    } else {                          // decay
      corr.ProcessDecayForward(e);
      corr.AddDecay(e);
    }

    corr.Retire(e.timestamp);

    if(i % 100000 == 0) {
      auto now = std::chrono::steady_clock::now();
      double dt = std::chrono::duration<double>(now - lastTime).count();
      double rate = dt > 0 ? (i - lastEntry) / dt : 0;   // events/s since last print

      double percent = 100.0 * i / nEntries;
      int barWidth = 30;
      int filled = (int)(percent / 100.0 * barWidth);
      printf("\r[");
      for(int b = 0; b < barWidth; ++b) putchar(b < filled ? '#' : '-');
      printf("] %6.2f%%  %.2f M events/s  (%lld/%lld)",
             percent, rate / 1e6, i, nEntries);
      fflush(stdout);

      lastTime = now;
      lastEntry = i;
    }
  }
  printf("\n");


  corr.Flush();
  corr.Write();
  out.Close();
  printf(" ( ˶ˆᗜˆ˵ )  Yay! - %s written \n", ofile.c_str());
  return 0;
}
