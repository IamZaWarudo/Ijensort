#include <cstdio>
#include <vector>
#include <string>

#include <chrono>
#include <iostream>
#include <string>
#include <algorithm>
#include <TSystem.h>
#include <TTree.h>
#include <TFile.h>

#include <evtLoop.h>
#include <ddasLoop.h>
#include <ddasHit.h>
#include <GChannel.h>
#include <GHistogramer.h>
#include <GBCS.h>
#include <globals.h>
#include <TOFCorrector.h>
#include <PIDGates.h>

// Implant -> 0
// Decay   -> 1
//
// IsotopeID -> Int to write a Root TTRee
static int isotopeCode(const std::string& iso) {
  if(iso == "Na") return 0;
return -1;    // for the things that are not in the TCut gate
}

struct EventRecord {   // things recorded in the tree
  int                  type;
  long long            id;
  int                  isotope;
  double               x;
  double               y;
  double               timestamp;
  double               energy;
  int                  run;
  std::vector<double>  gammas;
};

// Declaring TTree writes...
static TTree* gTree = nullptr;
static EventRecord gRec;
static int gRun = -1;
static long long gEventCounter = 0;
static void FillTree(GBCS& bcs, const std::vector<ddasHit>& event, const TOFCorrector* tofCorrector);

void ProcessEvent(GBCS& bcs, const std::vector<ddasHit>& event, const TOFCorrector* tofCorrector);

int main(int argc, char** argv) {
  if(argc < 2) { printf("usage: sort file.evt [file.evt ...]\n"); return 1; }

  std::vector<std::string> files;

    if(argc < 2) {
    std::cerr << "usage: sort [correction.tof] file.evt [file.evt ...]\n";
    return 1;
  }

  std::vector<std::string> inputFiles;
  std::string correctionFile;

  for(int i = 1; i < argc; ++i) {
    std::filesystem::path argument(argv[i]);
    const std::string extension = argument.extension().string();

    if(extension == ".evt") {
      inputFiles.push_back(argument.string());
    } else if(extension == ".tof") {
      if(!correctionFile.empty()) {
        std::cerr << "Only one .tof file may be supplied\n";
        return 1;
      }
      correctionFile = argument.string();
    } else {
      std::cerr << "Unsupported input file: " << argument << "\n";
      return 1;
    }
  }

  if(inputFiles.empty()) {
    std::cerr << "No EVT files supplied\n";
    return 1;
  }

  std::sort(inputFiles.begin(), inputFiles.end());

  std::string homedir = std::getenv("HOME");
  GChannel::ReadDetmap(Form("%s/Sandbox/Ijensort/cals/detmap3.tsv",homedir.c_str()));
  PIDGates::Get().Load(Form("%s/Sandbox/Ijensort/cals/pid_cuts.root", homedir.c_str()));

  std::filesystem::path p(inputFiles.front());
  std::string stem = p.stem().string();

  int run = -1;
  gRun = run;
  if(std::sscanf(stem.c_str(), "run-%d-%*d", &run) != 1) {
    std::cerr << "Invalid EVT filename: " << stem << "\n";
    return 1;
  }
  gSystem->mkdir("hist", true);
  std::string ofile;
  if(correctionFile.empty())
    ofile = Form("hist/hist%04d.root", run);
  else
    ofile = Form("hist/hist%04d_tofcorrected.root", run);

  GHistogramer::Get().SetOutFile(ofile);

  std::unique_ptr<TOFCorrector> tofCorrector;
    if(!correctionFile.empty()) {
      std::cout << "Loading TOF correction: " << correctionFile << "\n";
      tofCorrector = std::make_unique<TOFCorrector>(correctionFile);
    }

  evtLoop  reader(inputFiles, 500000, true);
  ddasLoop converter(reader, 200, 1);


// Creating the tree + branches
  gSystem->mkdir(Form("%s/Sandbox/Ijensort/tree", homedir.c_str()), true);
  TFile* treeFile = new TFile(Form("%s/Sandbox/Ijensort/tree/tree%04d.root", homedir.c_str(), run), "recreate");
  gTree = new TTree("events", "implant/decay events");
  gTree->Branch("type",      &gRec.type);
  gTree->Branch("id",        &gRec.id);
  gTree->Branch("isotope",   &gRec.isotope);
  gTree->Branch("x",         &gRec.x);
  gTree->Branch("y",         &gRec.y);
  gTree->Branch("timestamp", &gRec.timestamp);
  gTree->Branch("energy",    &gRec.energy);
  gTree->Branch("run",       &gRec.run);
  gTree->Branch("gammas",    &gRec.gammas);


  reader.Start();
  converter.Start();

  int64_t  lastPos    = 0;
  uint64_t lastBlocks = 0;
  uint64_t lastHits   = 0;
  auto lastTime  = std::chrono::steady_clock::now();
  auto lastPrint = lastTime;

  std::cout << HIDE_CURSOR << std::flush;

  GBCS bcs;
  std::vector<ddasHit> event;
  
  // implant rate variables
  double tFirst = -1; 
  double tLast  = 0;


  while(!converter.Finished() || !converter.Empty()) {
    if(converter.TryPop(event)) {
      double t = event.front().GetTime();   // Run Duration
      if(tFirst<0) tFirst = t;              // rate
      if(t>tLast) tLast = t;                // calculation
      ProcessEvent(bcs, event, tofCorrector.get());
      event.clear();
    }

  auto now = std::chrono::steady_clock::now();
  if(now - lastPrint > std::chrono::milliseconds(500)) {
    auto e = reader.GetStats();
    auto d = converter.GetStats();

    constexpr int barWidth = 30;
    double percent = e.Percent();
    int filled = std::clamp(static_cast<int>((percent / 100.0) * barWidth), 0, barWidth);
    std::string bar(filled, '#');
    bar.append(barWidth - filled, '-');

    double dt = std::chrono::duration<double>(now - lastTime).count();
    double mbps = 0, blockRate = 0, hitRate = 0;
    if(dt > 0) {
      mbps      = (e.filePos     - lastPos)    / dt / 1024.0 / 1024.0;
      blockRate = (e.blocksRead  - lastBlocks) / dt;
      hitRate   = (d.hitsBuilt   - lastHits)   / dt;
    }

    printf(CLEAR_LINE "[%s] %6.2f%%  file %llu/%llu  %.1f/%.1f MB  %7.1f MB/s\n",
        bar.c_str(), percent,
        (unsigned long long)e.currentFile, (unsigned long long)e.totalFiles,
        e.filePos / 1024.0 / 1024.0, e.fileSize / 1024.0 / 1024.0, mbps);
    printf(CLEAR_LINE "blocks=%llu (%6.0f/s)  hits=%llu (%6.0f/s)",
        (unsigned long long)e.blocksRead, blockRate,
        (unsigned long long)d.hitsBuilt,  hitRate);
    fflush(stdout);
    printf(CURSOR_UP);
    fflush(stdout);

    lastPos = e.filePos; lastBlocks = e.blocksRead; lastHits = d.hitsBuilt;
    lastTime = now; lastPrint = now;
  }

  }

  printf(CURSOR_DOWN "\n" SHOW_CURSOR);

  treeFile->cd();
  gTree->Write();
  treeFile->Close();
  reader.Stop();
  double duration = (tLast - tFirst) / 1.0e8;
  printf("run duration: %.1f s\n", duration);
  converter.Stop();
  GHistogramer::Get().Close();
  return 0;
}

void ProcessEvent(GBCS& bcs, const std::vector<ddasHit>& event, const TOFCorrector* tofCorrector) {
  bcs.Reset();

for(const auto &hit : event) {
    switch(hit.GetId()) {
      case 0 ... 39:     // front High Gain
        bcs.fHighGain.AddFrontHit(hit);
        break;
      case 40 ... 79:    // front Low  Gain
        bcs.fLowGain.AddFrontHit(hit);
        break;
      case 80 ... 119:   // back  High Gain
        bcs.fHighGain.AddBackHit(hit);
        break;
      case 120 ... 159:  // back  Low  Gain
        bcs.fLowGain.AddBackHit(hit);
        break;
      case 176:
        bcs.fI2N.Unpack(hit);
        break;
      case 177:
        bcs.fI2S.Unpack(hit);
        break;
      case 180:
        bcs.fI2TAC.Unpack(hit);
        break;
      case 181:
        bcs.fPin1.Unpack(hit);
        break;
      case 182:
        bcs.fPin2.Unpack(hit);
        break;
      case 183:
        bcs.fPin3.Unpack(hit);
        break;
      case 208 ... 271:  // 16 Clover x 4 crystals
        bcs.fClover.Unpack(hit, hit.GetId() - 208);
        break;
      case 272 ... 287:  // SSSD strips 0-15 (16 strips)
        bcs.fSSSD.AddHit(hit);
      break;

      default:
        break;
    }
  }

  bcs.fHighGain.Build();
  bcs.fLowGain.Build();
  bcs.fClover.BuildAddback();


// Fill histograms for each hit in the event
  for(auto hit : event) {
    GHistogramer::Get().Fill("All_Channel/ecal",16000,0,64000,hit.GetEcal(),
                                       300,0,300,hit.GetId());
    GHistogramer::Get().Fill("All_Channel/raw",16000,0,64000,hit.GetCharge(),
                                       300,0,300,hit.GetId());}


  // The one histogram that matters first: the SSSD spectrum that tells us t_h1/t_h2.
  double sssdMax = bcs.fSSSD.MaximumEnergy();
  if(sssdMax > 0)
    GHistogramer::Get().Fill("sssd/max_energy", 4000, 0, 32000, sssdMax);


  //Position Plots
  if(bcs.fHighGain.HasPosition()){     // High-gain -> Decay (car crash down the street)
    GHistogramer::Get().Fill("dssd/high_gain_position", 80, 0, 80, bcs.fHighGain.X(),
                                                        80, 0, 80, bcs.fHighGain.Y());}
  if(bcs.fLowGain.HasPosition()){     // Low-gain -> Implant (meteor crash down the street)
    GHistogramer::Get().Fill("dssd/low_gain_position", 80, 0, 80, bcs.fLowGain.X(),
                                                       80, 0, 80, bcs.fLowGain.Y());}

  if(bcs.IsImplant() && bcs.fLowGain.HasPosition()) {
    GHistogramer::Get().Fill("dssd/implant_map",80, 0, 80, bcs.fLowGain.X(),
                                                80, 0, 80, bcs.fLowGain.Y());}
  if(bcs.IsDecay() && bcs.fHighGain.HasPosition()) {
      GHistogramer::Get().Fill("dssd/Decay_map",80, 0, 80, bcs.fHighGain.X(),
                                                  80, 0, 80, bcs.fHighGain.Y());}
  

// Idea - Implantation rate plot to normalize the weird IsImplant plot


// Validity check between PIN1 and I2N
  if((bcs.fPin1.Time() > 10) && (bcs.fI2N.Time()>10)) { 
    GHistogramer::Get().Fill("PID/pidN",4000,0,64000,bcs.TOFN(),
                                  4000,0,16000,bcs.dE());
    GHistogramer::Get().Fill("TOF/tofN",3600,0,7200,bcs.fPin1.Time()/1.e8,
                                     4000,0,64000,bcs.TOFN());
  }

// Validity check between PIN1 and I2S
  if((bcs.fPin1.Time() > 10) && (bcs.fI2S.Time()>10)) { 
  const double runtime = bcs.fPin1.Time() / 1.e8;
  const double rawTOF = bcs.TOFS();

    GHistogramer::Get().Fill("PID/pidS",4000,0,64000,bcs.TOFS(),
                                  4000,0,16000,bcs.dE());
    GHistogramer::Get().Fill("TOF/tofS",3600,0,7200,bcs.fPin1.Time()/1.e8,
                                     4000,0,64000,bcs.TOFS());
  
    // tof_S used for the .tof calibration
   if(tofCorrector) {
     const double correctedTOF = tofCorrector->Correct(rawTOF,runtime);

     GHistogramer::Get().Fill("TOF/tofS_corr",3600, 0, 7200, runtime,
                                               4000, 0, 64000, correctedTOF);
   // TOF is x-axis for PID
     GHistogramer::Get().Fill("PID/pidS_Corr",4000, 0, 64000, correctedTOF,
                                               4000, 0, 16000, bcs.dE());
   }
}

// PID with veto plot (for particle identification)
  if(bcs.IsImplant() && (bcs.fPin1.Time() > 10) && (bcs.fI2S.Time()>10)) { 
   const double runtime = bcs.fPin1.Time() / 1.e8;
   const double rawTOF = bcs.TOFS();

    GHistogramer::Get().Fill("PID/pidS_Veto",4000,0,64000,bcs.TOFS(),
                                  4000,0,16000,bcs.dE());
    // tof_S used for the .tof calibration
   if(tofCorrector) {
     const double correctedTOF = tofCorrector->Correct(rawTOF,runtime);

   // TOF is x-axis for PID
     GHistogramer::Get().Fill("PID/pidS_Veto_Corr",4000, 0, 64000, correctedTOF,
                                               4000, 0, 16000, bcs.dE());

  // One-Implant-Position map per isotope
    std::string isotope = PIDGates::Get().Identify(correctedTOF, bcs.dE());
    if(!isotope.empty() && bcs.fLowGain.HasPosition()) {
      GHistogramer::Get().Fill(Form("dssd/implant_%s", isotope.c_str()),80,0,80, bcs.fLowGain.X(),
                                                                        80,0,80, bcs.fLowGain.Y());
    }
   }
}

// Clover Plots
  for(const auto& ab : bcs.fClover.addbackHits) {
    GHistogramer::Get().Fill("clover/addback", 4000, 0, 32000, ab.fEcal,
                             16, 0, 16, ab.fId);
   // GHistogramer::Get().Fill("clover/addback_mult", 8, 0, 8, ab.Mult());
  }
  for(const auto& h : bcs.fClover.hits) {
    GHistogramer::Get().Fill("clover/crystal", 4000, 0, 32000, h.fEcal,
                             64, 0, 64, h.fId);
  }

// Decay - Gamma coincidence
  if(bcs.IsDecay() && bcs.fHighGain.HasPosition()) {
    for(const auto& ab : bcs.fClover.addbackHits){
      GHistogramer::Get().Fill("decay/coincident_gamma", 16000,0,16000, ab.fEcal);
    }
  }




// stupid plot - I dont even know what you are??
 for(const auto& front : bcs.fHighGain.fFront) {
    for(const auto& back : bcs.fHighGain.fBack) {
      // int front_slot = (front.GetAddress() & 0xff00) >> 8;
      // int back_slot  = (back.GetAddress()  & 0xff00) >> 8;
      GHistogramer::Get().Fill(
          Form("dssd/High_front_energy_vs_back_energy"),4000, 0, 32000, front.GetEcal(),
                                                        4000, 0, 32000, back.GetEcal());}
}

//plot plots plotss......yay!

  FillTree(bcs, event, tofCorrector);
}


// Tree Filling function
static void FillTree(GBCS& bcs, const std::vector<ddasHit>& event, const TOFCorrector* tofCorrector) {
  const long long eventId = gEventCounter++;

  // Implant record
  if(bcs.IsImplant() && bcs.fLowGain.HasPosition() && tofCorrector) {
    const double runtime      = bcs.fPin1.Time() / 1.e8;
    const double correctedTOF = tofCorrector->Correct(bcs.TOFS(), runtime);
    std::string  isotope      = PIDGates::Get().Identify(correctedTOF, bcs.dE());

    if(!isotope.empty()) {
      gRec.type      = 0;
      gRec.id        = eventId;
      gRec.isotope   = isotopeCode(isotope);
      gRec.x         = bcs.fLowGain.X();
      gRec.y         = bcs.fLowGain.Y();
      gRec.timestamp = event.front().GetTime();
      gRec.energy    = bcs.fLowGain.Energy();
      gRec.run       = gRun;
      gRec.gammas.clear();
      gTree->Fill();
    }
  }

  // Decay record
  if(bcs.IsDecay() && bcs.fHighGain.HasPosition()) {
    gRec.type      = 1;
    gRec.id        = eventId;
    gRec.isotope   = -1;
    gRec.x         = bcs.fHighGain.X();
    gRec.y         = bcs.fHighGain.Y();
    gRec.timestamp = event.front().GetTime();
    gRec.energy    = bcs.fHighGain.Energy();
    gRec.run       = gRun;
    gRec.gammas.clear();
    for(const auto& ab : bcs.fClover.addbackHits)
      gRec.gammas.push_back(ab.fEcal);
    gTree->Fill();
  }
}
