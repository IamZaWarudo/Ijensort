
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

  fClover.Reset();
}

// threshold defined in GBCS.h

bool GBCS::IsImplant() const {
  // Implant is like a meteor crashing onto earth so low-gain trigger is enough to identify it.
  // SSSD should be below threshold th2 to avoid noise.
  return fLowGain.Triggered() && fSSSD.MaximumEnergy() < th2;
}

bool GBCS::IsDecay() const {
  // For Decay PIN and SSSD has to be silent.
  // Silent = below threshold th1 (noise)
  bool noPin = !(fPin1.Time() > 0 || fPin2.Time() > 0 || fPin3.Time() > 0);
  return fHighGain.Triggered() && noPin && fSSSD.MaximumEnergy() < th1;
}

// gating verified based on dt = front - back v/s count plot
GBCS::GBCS() {
  fHighGain.SetTimingGate(4,13);    // HG: 4-13 ticks
  fLowGain.SetTimingGate(2,7);      // LG: 2-7 ticks
}
