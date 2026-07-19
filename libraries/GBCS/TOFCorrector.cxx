#include <vector>

#include <TOFCorrector.h>

#include <TCanvas.h>
#include <TVirtualPad.h>

#include <TFile.h>
#include <TSpectrum.h>
#include <TSpline.h>
#include <TProfile.h>
#include <TH2.h>
#include <TRandom.h>

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

#include <TGraph.h>
#include <TH1D.h>
#include <TSystem.h>

#include <utils.h>


namespace {

struct PeakGate {
  double expected;
  double low;
  double high;
};

const std::array<PeakGate, 5> peakGates = {{
    { 4840.0,  4000.0,  5700.0},
    { 8248.0,  7400.0,  9100.0},
    {10376.0,  9500.0, 10900.0},
    {11608.0, 11000.0, 12500.0},
    {13192.0, 12600.0, 14000.0}
}};

double Median(std::vector<double> values) {
  if(values.empty())
    return 0.0;

  std::sort(values.begin(), values.end());
  const std::size_t middle = values.size() / 2;

  if(values.size() % 2 == 1)
    return values[middle];

  return 0.5 *
      (values[middle - 1] + values[middle]);
}

std::vector<double> RunningMedian(const std::vector<double>& values, std::size_t halfWidth = 2) {
  std::vector<double> result(values.size());

  for(std::size_t i = 0; i < values.size(); ++i) {
    const std::size_t first = i > halfWidth ? i - halfWidth : 0;
    const std::size_t last = std::min(i + halfWidth, values.size() - 1);
    std::vector<double> window( values.begin() + first, values.begin() + last + 1);
    result[i] = Median(window);
  }

  return result;
}

bool MeasurePeakCenter(TH1D* projection, const PeakGate& gate, double& center) {
  const int gateLowBin = projection->GetXaxis()->FindBin(gate.low);

  const int gateHighBin = projection->GetXaxis()->FindBin(gate.high);

  int maximumBin = -1;
  double maximumCounts = 0.0;

  // Find at most one maximum inside this physical gate.
  for(int bin = gateLowBin; bin <= gateHighBin; ++bin) {

    const double counts = projection->GetBinContent(bin);

    if(counts > maximumCounts) {
      maximumCounts = counts;
      maximumBin = bin;
    }
  }

  if(maximumBin < 0 || maximumCounts <= 0.0)
    return false;

  // Calculate a centroid near the local maximum.
  constexpr int centroidHalfWidth = 10;
  const int centroidLowBin = std::max(gateLowBin, maximumBin - centroidHalfWidth);
  const int centroidHighBin = std::min(gateHighBin, maximumBin + centroidHalfWidth);
  double weightedSum = 0.0;
  double countSum = 0.0;

  for(int bin = centroidLowBin; bin <= centroidHighBin; ++bin) {
    const double counts = projection->GetBinContent(bin);
    weightedSum += counts * projection->GetBinCenter(bin);
    countSum += counts;
  }

  // Reject time slices without enough statistics.
  if(countSum < 500.0)
    return false;

  center = weightedSum / countSum;
  return true;
}

} // namespace


TOFCorrector::TOFCorrector() : fTofTime(0) { }

TOFCorrector::TOFCorrector(std::string fname) : fTofTime(0) { 
  TDirectory *current =gDirectory;
  TFile *temp = TFile::Open(fname.c_str());
  TOFCorrector *tc = (TOFCorrector*)temp->Get("TOFCorrector");
  tc->Copy(*this);
  temp->Close();
  current->cd();
}

TOFCorrector::~TOFCorrector() { }


void TOFCorrector::Copy(TOFCorrector &other) const { 
  TObject::Copy(other);

  other.fNPeaks        = fNPeaks;
  other.fExpectedBin   = fExpectedBin;
  other.fExpectedValue = fExpectedValue;
  other.fSplines        = fSplines;
  //if(fTofTime)
  other.fTofTime = fTofTime;

}



//  void TOFCorrector::FitTOF(TH2 *tof_time) {
//    if(!tof_time)
//      tof_time =fTofTime;    
//  
//    TH1D *py      = fTofTime->ProjectionY();
//  
//    TSpectrum *s = new TSpectrum;
//    double threshold = 0.0005;
//    int iterations =0;
//    s->Search(py,1,"goff",threshold);
//  
//    while(s->GetNPeaks()!=EXPECTED_PEAKS && iterations<1000) {
//      if(s->GetNPeaks()>EXPECTED_PEAKS)
//        threshold+=threshold*.005;
//      else  
//        threshold-=threshold*.005;
//      s->Search(py,1,"goff",threshold);
//    }
//  
//    if(s->GetNPeaks()!=EXPECTED_PEAKS)
//      return;
//  
//  
//    for(int i=0;i<s->GetNPeaks();i++) {  
//      //peaks.push_back(std::make_pair(s->GetX()[i],py->GetXaxis()->FindBin(s->GetX()[i])));
//  
//      double ntof = s->GetPositionX()[i];
//      int   btof = py->GetXaxis()->FindBin(ntof);
//      int   ltof = btof-TOF_WIDTH/2;
//      int   htof = btof+TOF_WIDTH/2;
//      //TH1D *p = tof->ProjectionX(Form("p%i",i+1),ltof,htof);
//      TProfile *p = fTofTime->ProfileX(Form("p%i",i+1),ltof,htof);
//  
//      TGraph gr;
//      for(int i=1;i<=p->GetNbinsX();i++) { 
//        if(int(p->GetBinContent(i))!=0) {
//          gr.AddPoint(p->GetBinCenter(i),p->GetBinContent(i));
//        }
//      }
//      TSpline3 *spline = new TSpline3(Form("sp%i",i+1),gr.GetX(),gr.GetY(),gr.GetN());
//  
//      spline->SetLineWidth(2);
//      spline->SetLineColor(2);
//      spline->SetNpx(100000);
//      //splines3->Draw("same");
//      
//      //new TCanvas;
//      //gr.DrawClone("A*");
//      //p->DrawClone("same");
//      //spline->Draw("same");
//  
//      fExpectedBin.push_back(btof);
//      fExpectedValue.push_back(ntof);
//      fSplines.push_back(spline);
//  
//      printf("found peak %.1f:\t with limits %i to %i\n",ntof,ltof,htof);
//    }
//    fNPeaks =s->GetNPeaks();
//  
//  }


void TOFCorrector::FitTOF(TH2* tof_time) {
  TH2* histogram = tof_time ? tof_time : fTofTime;

  if(!histogram) {
    fprintf(stderr, "FitTOF: no histogram supplied\n");
    return;
  }

  for(TSpline3* spline : fSplines)
    delete spline;

  fSplines.clear();
  fExpectedBin.clear();
  fExpectedValue.clear();
  fNPeaks = 0;

// Spline parameters
  constexpr int binsPerSlice = 5;

  std::array<std::vector<double>, 5> times;
  std::array<std::vector<double>, 5> centers;

  for(int xFirst = 1; xFirst <= histogram->GetNbinsX(); xFirst += binsPerSlice) {
    const int xLast = std::min(xFirst + binsPerSlice - 1, histogram->GetNbinsX());

    std::unique_ptr<TH1D> projection(histogram->ProjectionY(Form("tof_slice_%d", xFirst), xFirst,xLast));
    const double timeLow = histogram->GetXaxis()->GetBinLowEdge(xFirst);
    const double timeHigh = histogram->GetXaxis()->GetBinUpEdge(xLast);
    const double timeCenter = 0.5 * (timeLow + timeHigh);

    for(std::size_t peak = 0;peak < peakGates.size();++peak) {
      double measuredCenter = 0.0;

      if(!MeasurePeakCenter(projection.get(), peakGates[peak], measuredCenter)) {
        continue;
      }
      times[peak].push_back(timeCenter);
      centers[peak].push_back(measuredCenter);
    }
  }

  for(std::size_t peak = 0; peak < peakGates.size();++peak) {

    if(times[peak].size() < 4) {
      fprintf(stderr,"Skipping peak near %.1f: only %zu points\n",peakGates[peak].expected,times[peak].size());
      continue;
    }
// Spline parameter
    std::vector<double> smoothedCenters = RunningMedian(centers[peak], 1);
    const double reference = Median(smoothedCenters);
    
    TSpline3* spline = new TSpline3(Form("sp_%zu", peak),times[peak].data(),smoothedCenters.data(),static_cast<int>(times[peak].size()));
    
    spline->SetLineColor(kRed);
    spline->SetLineWidth(2);
    spline->SetNpx(10000);
    fExpectedValue.push_back(reference);
    fExpectedBin.push_back(histogram->GetYaxis()->FindBin(reference));
    
    fSplines.push_back(spline);

    printf("Peak near %.1f: %zu points, reference %.1f\n",peakGates[peak].expected,times[peak].size(),reference);
  }
  fNPeaks = static_cast<int>(fSplines.size());
  printf("Built %d usable TOF-band splines\n",fNPeaks);
}


void TOFCorrector::MakeCorrectionFile(std::string fname, TH2  *tof_time) { 

  TFile *f =0;
  //if(fname.length()==0)
  //  f =_file0;
  //else
  f = TFile::Open(fname.c_str());

//  int run,subrun;
//  getRunNumber(fname,run,subrun);
//  printf("fname: %s\n",fname.c_str());
//  printf("run: %i\n",run);
//  printf("subrun: %i\n",subrun);

// so it can read histXXXX and histXXXX-XX
int run = -1;
int subrun = -1;

  if(!getRunNumber(fname, run, subrun)) {
    fprintf( stderr,"Could not determine run number from: %s\n",fname.c_str());
    return;
  }
  printf("fname: %s\n", fname.c_str());
  printf("run: %i\n", run);
  if(subrun >= 0)
    printf("subrun: %i\n", subrun);
  if(!f)
    return;



  if(!tof_time)
    tof_time = (TH2*)f->Get("tof_time");  //this is a 2d: xaxis runtime, yaxis TOF.

  fTofTime = (TH2*)tof_time->Clone(Form("%s_clone",tof_time->GetName()));

  FitTOF(fTofTime);

  TDirectory *current =gDirectory;
 
  gSystem->mkdir("tof",true);
  std::string correctionFilename;
  if(subrun >= 0) {
    correctionFilename = Form("tof/tof%04i-%02i.tof", run, subrun);
  } else {
    correctionFilename = Form("tof/tof%04i.tof", run);
  }
  
  TFile* ofile = TFile::Open(correctionFilename.c_str(), "recreate");
    this->Write();
    ofile->Close();
    current->cd();
  } 



double TOFCorrector::CommonDrift(double time) const {
  std::vector<double> drifts;
  for(std::size_t i = 0;i < fSplines.size();++i) {

    TSpline3* spline = fSplines[i];

    const double evaluationTime = std::clamp(time,spline->GetXmin(),spline->GetXmax());
    const double measuredCenter = spline->Eval(evaluationTime);
    const double reference = fExpectedValue.at(i);

    drifts.push_back(measuredCenter - reference);
  }

  if(drifts.empty())
    return 0.0;

  return Median(drifts);
}

double TOFCorrector::Correct(double tof,double time,int peak) const {
  // Default: median drift across every usable band.
  if(peak < 0)
    return tof - CommonDrift(time);

  TSpline3* spline = fSplines.at(peak);

  const double evaluationTime = std::clamp(time,spline->GetXmin(),spline->GetXmax());
  const double measuredCenter = spline->Eval(evaluationTime);
  const double reference = fExpectedValue.at(peak);
  const double drift = measuredCenter - reference;

  return tof - drift;
}

TH2* TOFCorrector::CorrectedTOF() const
{
  if(!fTofTime)
    return nullptr;

  TH2* corrected =
      static_cast<TH2*>(
          fTofTime->Clone("tofCorrected"));

  corrected->Reset();

  for(int x = 1;
      x <= fTofTime->GetNbinsX();
      ++x) {

    const double runtime =
        fTofTime->GetXaxis()->GetBinCenter(x);

    for(int y = 1;
        y <= fTofTime->GetNbinsY();
        ++y) {

      const double counts =
          fTofTime->GetBinContent(x, y);

      if(counts <= 0.0)
        continue;

      const double rawTOF =
          fTofTime->GetYaxis()->GetBinCenter(y);

      const double correctedTOF =
          Correct(rawTOF, runtime);

      corrected->Fill(
          runtime,
          correctedTOF,
          counts);
    }
  }

  return corrected;
}

void TOFCorrector::Draw() const {
  if(!fTofTime) return;

  TCanvas *g = new TCanvas();
  g->Divide(2,1);

  
  TVirtualPad *p1 = g->cd(1);
  p1->Divide(1,2);
  p1->cd(1);
  fTofTime->Draw("colz2"); 
  p1->cd(2);
  CorrectedTOF()->Draw("colz2");

  TVirtualPad *p2 = g->cd(2);
  p2->Divide(1,fNPeaks);
  for(int i=0;i<fNPeaks;i++) {
    p2->cd(i+1);
    int b = fExpectedBin.at(i);
    TProfile *prof = fTofTime->ProfileX(Form("fTofTime_%i",i+1),b-TOF_WIDTH/2,b+TOF_WIDTH/2);
    prof->Draw();
    double tof = fExpectedValue.at(i);
    prof->GetYaxis()->SetRangeUser(tof-5,tof+5);
    fSplines.at(i)->Draw("same");
  }
};



