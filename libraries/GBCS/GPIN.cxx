
#include <GPIN.h>
#include <ddasHit.h>

#include <TRandom.h>

ClassImp(GPIN);

//
// Reset PIN variables
//
void GPIN::Reset()
{
  fCharge = 0.;
  fTime   = 0.;
  fEcal   = 0.;
  //fHit    = 0;
}

GPIN::GPIN(const ddasHit& ddashit) {
  Unpack(ddashit);
}

void GPIN::Unpack(const ddasHit &ddashit) {

  fTime   = ddashit.GetTime();
  fCharge = ddashit.GetCharge();
  fEcal   = ddashit.GetEcal(); 

  // threshold check
  //if(ec > cal.thresh && ec < cal.uld) {
  //if(fEcal > 0 && fEcal < 64000) {
  //  fHit = 1;
  //}
}



GPIN::GPIN() { Reset(); }

GPIN::~GPIN() { } 

void GPIN::Copy(GPIN &other) const {
  other.fCharge = fCharge;
  other.fEcal   = fEcal;
  other.fTime   = fTime;
  //other.fHit    = fHit;
  return;
}


