#include <iostream>

#include <TROOT.h>
#include <TStyle.h>
#include <TEnv.h>
#include <TString.h>

#include <Ijen.h>

Ijen *Ijen::fIjen = nullptr;

Ijen::Ijen(int argc, char **argv) : TRint("ijen",0,0,0,0,true,false),
  fTabLock(false), fMainThreadId(std::this_thread::get_id()) {

  LoadStyle();
  SetPrompt("ijen [%d] ");
}

Ijen *Ijen::Get(int argc, char **argv) {
  if(!fIjen) {
    fIjen = new Ijen(argc,argv);
  }
  return fIjen;
}

Ijen::~Ijen() { }

void Ijen::Terminate(int status) {
  printf("\nbye,bye\n\n");
  SetPrompt("");
  TRint::Terminate(status);
}

void Ijen::LoadStyle() {
  gStyle->SetPalette(gEnv->GetValue("Ijen.Style",kVisibleSpectrum));
  gStyle->SetHistLineWidth(2);
  gStyle->SetHistFillStyle(0);

  gStyle->SetFrameBorderMode(1);
  gStyle->SetPadBorderMode(1);
  gStyle->SetPadBorderSize(5);

  gStyle->SetOptStat(gEnv->GetValue("Hists.Stats","nemri"));

  gROOT->ForceStyle();
}

long Ijen::ProcessLine(const char* line, bool sync, int* error) {
  if(fTabLock) {
    return TRint::ProcessLine(line,sync,error);
  }

  TString sline(line);
  if(!sline.Length()) {
    return 0;
  }

  if(std::this_thread::get_id() != fMainThreadId) {
    printf("Not the main thread...  ");
    fflush(stdout);
  }

  if(!sline.CompareTo("clear")) {
    return TRint::ProcessLine(".! clear");
  }

  return TRint::ProcessLine(sline.Data(),sync,error);
}
