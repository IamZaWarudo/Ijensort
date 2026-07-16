
#include <GBCS.h>
#include <TEnv.h>

void GBCS::Reset() {
  fPin1.Reset();
  fPin2.Reset();
  fPin3.Reset();

  fI2N.Reset();
  fI2S.Reset();
  fI2TAC.Reset();

  fLowGain.Reset();
  fHighGain.Reset();

  fSSSD.Reset();
}

bool GBCS::IsImplant() const {
  // Implant: low-gain DSSD fires front AND back, SSSD quiet (below t_h2).
  double th2 = gEnv->GetValue("BCS.SSSD.th2", 1e9);   // default: never quiet-fails
  bool dssd      = fLowGain.Triggered();
  bool sssdQuiet = fSSSD.MaximumEnergy() < th2;
  return dssd && sssdQuiet;
}

bool GBCS::IsDecay() const {
  // Decay: high-gain DSSD fires front AND back, no PIN (anti-coincidence),
  //        SSSD in noise region (below t_h1).
  double th1 = gEnv->GetValue("BCS.SSSD.th1", 1e9);
  bool dssd      = fHighGain.Triggered();
  bool noPin     = !(fPin1.Time() > 0 || fPin2.Time() > 0 || fPin3.Time() > 0);
  bool sssdNoise = fSSSD.MaximumEnergy() < th1;
  return dssd && noPin && sssdNoise;
}