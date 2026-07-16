#ifndef GPIN_H
#define GPIN_H

//#include "DDASRootFitHit.h"

class ddasHit;

#include <TObject.h>

class GPIN {
  public:
    GPIN();
    GPIN(const ddasHit &hit);
    ~GPIN();

    void Reset();
    void Copy(GPIN &other) const;
    void Unpack(const ddasHit &hit);
    

    double Charge() const { return fCharge; }
    double Ecal()   const { return fEcal;   }
    double Time()   const { return fTime;   }
    //int    Hit()    const { return fHit;    }

  private:
    double fCharge;
    double fEcal;
    double fTime;
    //int fHit;
  
  ClassDef(GPIN,1);
};

#endif
