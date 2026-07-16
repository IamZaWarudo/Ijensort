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
    void Build();  // convert fFront & fBack into pixels.
    bool HasPosition() const {return fX >= 0 && fY >= 0;}
    double X() const {return fX;}
    double Y() const {return fY;}  // channel id to strip calculation done in GDSSD.cxx   


    void Print() const;

    double FrontEnergySum() const;
    double BackEnergySum() const;
    double Energy() const;
    bool HasEnergy() const;

    int GetStrip(const ddasHit&) const; 

  //private:
    double fX;
    double fY;
   
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
