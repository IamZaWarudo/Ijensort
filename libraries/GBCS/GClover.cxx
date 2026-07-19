#include "GClover.h"
#include <ddasHit.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>


// Removed the TClover Cal map stuff, the detmap is goated.

GCloverHit::GCloverHit() {  }
GCloverHit::~GCloverHit() {  }
GCloverHit::GCloverHit(const GCloverHit &hit) {
  hit.Copy(*this);
}

void GCloverHit::Copy(GCloverHit &other) const {
  other.fId      = fId;
  other.fTime    = fTime;
  other.fCfdTime = fCfdTime;
  other.fEnergy  = fEnergy;
  other.fEcal    = fEcal;
}

void GCloverHit::Reset() {
  fId       = -1;
  fTime     = -1;
  fCfdTime  = -1;
  fEnergy   = -1;
  fEcal     = -1;   
}

GAddbackHit::GAddbackHit() : GCloverHit(),fMult(0) { }
GAddbackHit::~GAddbackHit() { }

bool GAddbackHit::CheckTime(const GCloverHit &other) const {
  return std::abs(other.fTime - fTime) < 200;   // gate — untuned, same as betasort
}


// Constructor for addback hit from a single crystal hit
GAddbackHit::GAddbackHit(const GCloverHit &hit) : GCloverHit(hit), fMult(1) {
  fId = hit.fId/4;      // crystal (0..63) -> detector (0..15)
  fEnergy = fEcal;
}

void GAddbackHit::Reset() {
  GCloverHit::Reset();
  fMult = 0;
}

void GAddbackHit::Add(const GCloverHit &hit) {
  fEcal += hit.fEcal;
  if(hit.fEcal > fEnergy) {          // adopt time/energy of largest crystal
    fEnergy  = hit.fEcal;
    fTime    = hit.fTime;
    fCfdTime = hit.fCfdTime;
  }
  fMult++;
}


GClover::GClover() { }
GClover::~GClover() { }

void GClover::Reset() {
  hits.clear();
  addbackHits.clear();
  hit = 0;
}

void GClover::Unpack(const ddasHit &ddashit, int det) {
  GCloverHit h;
  h.fId      = det;
  h.fTime    = ddashit.GetTime();
  h.fCfdTime = ddashit.GetCFD();             
  h.fEnergy  = ddashit.GetCharge();   
  h.fEcal    = ddashit.GetEcal();  // det-map calibrated energy
  //  if(h.fEcal<10. || h.fEcal>8192.) return;     // threshold + overflow reject
  if(h.fEcal<10.) return;
  hits.push_back(h);
}

void GClover::Copy(GClover &other) const {
  other.Reset();
  other.hit = hit;
  for(size_t i=0;i<hits.size();i++)
    other.hits.push_back(GCloverHit(hits[i]));
}

void GClover::BuildAddback() {
  addbackHits.clear();
  for(const auto& h : hits) {
    const int addbackId = h.fId/4;
    if(addbackId < 0 || addbackId >= 16) continue;
    bool added = false;
    for(auto& ab : addbackHits) {
      if(ab.fId != addbackId) continue;
      if(!ab.CheckTime(h))     continue;
      ab.Add(h);
      added = true;
      break;
    }
    if(!added) addbackHits.emplace_back(h);
  }
}
