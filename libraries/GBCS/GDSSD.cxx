
#include<GDSSD.h>

#include<cstdio>

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

void GDSSD::Build() {

  double sumFront    = 0;
  double sumBack     = 0;
  double weightFront = 0;
  double weightBack  = 0;

  for(const auto& front : fFront) {
    for(const auto& back : fBack) {
      if(abs(front.GetTime() - back.GetTime()) > 100) continue;

      sumFront    += front.GetEcal();
      sumBack     += back.GetEcal();
      weightFront += front.GetEcal() * GetStrip(front);
      weightBack  += back.GetEcal()  * GetStrip(back);   // was front.GetEcal()
    }
  }

  fX = sumFront > 0 ? weightFront / sumFront : -1;
  fY = sumBack  > 0 ? weightBack  / sumBack  : -1;
}


/*
  fX = -1;
  fY = -1;

  if(fFront.empty() || fBack.empty())
  return;

  double frontWeightedSum = 0.0;
  double frontEnergySum = 0.0;

  for(const auto& hit : fFront) {
    const int strip = FrontStrip(hit.GetId());
    const double energy = hit.GetEcal();

    if(strip < 0 || energy <= 0)
    continue;

    frontWeightedSum += strip * energy;
    frontEnergySum += energy;
  }

  double backWeightedSum = 0.0;
  double backEnergySum = 0.0;

  for(const auto& hit : fBack) {
    const int strip = BackStrip(hit.GetId());
    const double energy = hit.GetEcal();

    if(strip < 0 || energy <= 0)
    continue;

    backWeightedSum += strip * energy;
    backEnergySum += energy;
  }

  if(frontEnergySum <= 0 || backEnergySum <= 0)
  return;

  fX = frontWeightedSum / frontEnergySum;
  fY = backWeightedSum / backEnergySum;
}
*/

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


