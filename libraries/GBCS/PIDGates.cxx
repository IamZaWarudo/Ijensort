#include <PIDGates.h>

#include <TFile.h>
#include <TCutG.h>
#include <TKey.h>

#include <cstdio>

PIDGates& PIDGates::Get() {
  static PIDGates instance;
  return instance;
}

void PIDGates::Load(const std::string& fname) {
  TFile f(fname.c_str());
  if(f.IsZombie()) {
    fprintf(stderr, "PIDGates: cannot open %s\n", fname.c_str());
    return;
  }

  for(auto&& keyObj : *f.GetListOfKeys()) {
    TKey* key = static_cast<TKey*>(keyObj);
    TObject* obj = key->ReadObj();
    if(TCutG* cut = dynamic_cast<TCutG*>(obj)) {
      fCuts.push_back(static_cast<TCutG*>(cut->Clone()));
      printf("PIDGates: loaded gate '%s'\n", cut->GetName());
    }
  }
  f.Close();

  printf("PIDGates: %d gates loaded\n", Size());
}

std::string PIDGates::Identify(double tof, double de) const {
  for(const TCutG* cut : fCuts) {
    if(cut->IsInside(tof, de))
      return cut->GetName();
  }
  return "";
}
