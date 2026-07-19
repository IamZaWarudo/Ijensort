#ifndef GCLOVER_H
#define GCLOVER_H

#include <ddasHit.h>

#include <TObject.h>
#include <TRandom3.h>

#include <map>
#include <string>
#include <vector>

// Container for one calibrated crystal hit
class GCloverHit : public TObject {
    public:
      GCloverHit();
      GCloverHit(const GCloverHit&);
      ~GCloverHit();

      void Copy(GCloverHit &other) const;
      void Reset();

    //private:
    int    fId;
    double fTime;
    double fCfdTime;
    double fEnergy;
    double fEcal;

    ClassDef(GCloverHit,4)
};


// Container for a summed clover hit (reconstructed from 4 crystals) + fMult 
class GAddbackHit : public GCloverHit {
  public:
    GAddbackHit();
    GAddbackHit(const GCloverHit& hit);
    ~GAddbackHit();

    void Add(const GCloverHit& hit);
    void Reset();

    bool CheckTime(const GCloverHit &other) const;

    int Mult() const { return fMult;}

  private:
    int fMult;

  ClassDef(GAddbackHit,0)
};


// Container to hold both vector 
class GClover : public TObject {
  public:
    GClover();
    ~GClover();

    void Copy(GClover &other) const;
    void Reset();
    void Unpack(const ddasHit &chanhit, int det);
    void BuildAddback();

//  private:
    std::vector<GCloverHit> hits;
    std::vector<GAddbackHit> addbackHits; //!
    int hit;

  ClassDef(GClover,4);
};

#endif 