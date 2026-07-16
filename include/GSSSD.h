#ifndef __GSSSD_H__
#define __GSSSD_H__

#include <ddasHit.h>

#include <vector>
#include <cstddef>

class GSSSD {

public:
  GSSSD() = default;
  ~GSSSD() = default;

  void Reset();
  void AddHit(const ddasHit& hit);
  
  bool Empty() const { return fHits.empty(); }
  
  std::size_t Multiplicity() const {
    return fHits.size();
  }

  double EnergySum() const;
  double MaximumEnergy() const;

  const std::vector<ddasHit>& Hits() const {
    return fHits;
  }


private:
  std::vector<ddasHit> fHits; //!
  ClassDef(GSSSD, 1);
};

#endif
