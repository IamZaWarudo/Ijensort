
#include<GDSSD.h>

#include<cstdio>

// For reference
// Low-gain -> Implant ; because it deposits more energy
// High-gain -> Decay ; because it deposits less energy 


void GDSSD::Reset() { 
  fX = -1;
  fY = -1;
  fTime = -1;
  fEnergy = -1;
  fFront.clear();
  fBack.clear();
}

void GDSSD::Print() const { 
  printf("front mult: %lu\t back mult: %lu\n",fFront.size(),fBack.size());
  for(const auto &hit : fFront) {
    printf("\t%i: %.1f\t%.1f\n",hit.GetId(),hit.GetEcal(),hit.GetTime());
  }
  printf("---------------------\n");
  for(const auto &hit : fBack) {
    printf("\t%i: %.1f\t%.1f\n",hit.GetId(),hit.GetEcal(),hit.GetTime());
  }
  printf("---------------------\n");
  printf("---------------------\n");

}

// Defining front and back strip from channel IDs

int GDSSD::GetStrip(const ddasHit &hit) const {
  int id = hit.GetId();
  if(id >= 0 && id <= 39)
  return id;                  // returning Front High Gain 

  if(id >= 40 && id <=79)
  return id -40;              // returning Front Low Gain
  
  if(id >= 80 && id <= 119)
  return id - 80;             // returning Back High gain

  if(id >= 120 && id <= 159)
  return id -120;             // returning Back Low gain

  return -1;
}


// Weighted average to get XY position == (front strip, back strip)

double GDSSD::Position(const std::vector<ddasHit>& hits) const {  
  double sum     = 0.0;
  double weight  = 0.0;

  for(const auto& hit : hits){
    const int strip = GetStrip(hit);
    const double energy = hit.GetEcal();

    if(strip<0 || energy<=0)
      continue;

    weight += energy*strip;
    sum    += energy;
  }

  return sum > 0.0 ? weight/sum : -1.0;
}

double GDSSD::MaxEnergyTime(const std::vector<ddasHit>& hits) const {
  double maxEnergy = -1;
  double time      = -1;

  for(const auto& hit : hits) {
    if(hit.GetEcal() > maxEnergy) {
      maxEnergy = hit.GetEcal();
      time      = hit.GetTime();
    }
  }

  return time;
}

void GDSSD::Build() {
  fX = -1;
  fY = -1;

  // front/back coincidence: gate on the time difference of the highest-energy hit on each side. Reject the event if out of window.
  const double dt = MaxEnergyTime(fFront) - MaxEnergyTime(fBack);
  if(dt < fTLow || dt > fTHigh)  // defined in GBCS.h
    return;

  fX = Position(fFront);
  fY = Position(fBack);
}

double GDSSD::FrontEnergySum() const {
  double sum = 0.0;
  for(const auto& hit : fFront) {
    if(hit.GetEcal() > 0.0)
      sum += hit.GetEcal();
  }
  return sum;
}


double GDSSD::BackEnergySum() const {
  double sum = 0.0;

  for(const auto& hit : fBack) {
    if(hit.GetEcal() > 0.0)
      sum += hit.GetEcal();
  }
  return sum;
}

bool GDSSD::HasEnergy() const {
  return FrontEnergySum() > 0.0 && BackEnergySum() > 0.0;
}

double GDSSD::Energy() const {
  if(!HasEnergy()) return 0.0;
  return 0.5 * (FrontEnergySum() + BackEnergySum()); // same ion goes thru both and deposits the same energy
  // so the total is basically a double of the actual Energy, that's why as a rough estimate i am doing *0.5
}


