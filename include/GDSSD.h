#ifndef __GDSSD_H__
#define __GDSSD_H__

#include <ddasHit.h>

#include<vector>

/*
class pixel {
  
  //private:
    double fX;
    double fY;
    double energy;
    double time;
};
 */  // NOT needed - this is dumb, dont do it.


class GDSSD {
  public:
    GDSSD() = default;
    ~GDSSD() = default;

    void Reset();

    void AddFrontHit(const ddasHit &hit) { fFront.emplace_back(hit); }
    void AddBackHit(const ddasHit &hit)  { fBack.emplace_back(hit); }

// implementing DSSD front v/s back position plot
    double MaxEnergyTime(const std::vector<ddasHit>& hits) const;
    void Build();  // convert fFront & fBack into pixels.
    bool HasPosition() const {return fX >= 0 && fY >= 0;}  // To reject any pixel that weren't constructed properly
    double X() const {return fX;}
    double Y() const {return fY;}  // channel id to strip calculation done in GDSSD.cxx   


    void Print() const;

    double FrontEnergySum() const;
    double BackEnergySum() const;
    double Energy() const;
    bool HasEnergy() const;

    int GetStrip(const ddasHit&) const; 

    void SetTimingGate(double lo, double hi) { fTLow = lo; fTHigh = hi; }

  //private:
    double fX;
    double fY;
    double Position(const std::vector<ddasHit>& hits) const;

    // front-back time coincidence gates initialized to very wide value
    double fTLow   = -1e9;  
    double fTHigh  =  1e9;

    double fEnergy;
    double fTime;

    bool Triggered() const { return fFront.size() && fBack.size(); }

  //private:
    std::vector<ddasHit> fFront; //!
    std::vector<ddasHit> fBack;  //!

    //std::vector<pixel> fPixels;

  ClassDef(GDSSD,0);
};

#endif
