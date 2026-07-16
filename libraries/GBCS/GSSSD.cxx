#include <GSSSD.h>

#include <cstdio>
#include <algorithm>

void GSSSD::Reset() {
  fHits.clear();
}

void GSSSD::AddHit(const ddasHit& hit) {
  fHits.push_back(hit);
}

double GSSSD::EnergySum() const {
  double sum = 0.0;
  for(const auto& hit : fHits) {
    if(hit.GetEcal() > 0.0)
      sum += hit.GetEcal();
  }
  return sum;
}

double GSSSD::MaximumEnergy() const {
  double maximum = 0.0;
  for(const auto& hit : fHits) {
    maximum = std::max(maximum, hit.GetEcal());
  }
  return maximum;
}






