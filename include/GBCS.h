#ifndef __GBCS_H__
#define __GBCS_H__

#include <GPIN.h>
#include <GDSSD.h>
#include <GSSSD.h>

#include <TObject.h>

class GBCS {
  public:  
    GBCS() = default;
    ~GBCS() = default;

    void Reset();

    //double TOF() const { return fI2N.Time() - fPin1.Time(); }
    double TOFS() const { return fI2S.Charge(); }
    double TOFN() const { return fI2N.Charge(); }
    double dE()  const { return fPin1.Ecal(); }

    bool Triggered() const { return fLowGain.Triggered() || fHighGain.Triggered(); }

    bool IsImplant() const;
    bool IsDecay()   const;

  //private:
    GPIN fI2N;   // this is a TAC!!
    GPIN fI2S;   // this is a TAC!!
    GPIN fI2TAC; // this is a TAC!!

    GPIN  fPin1;
    GPIN  fPin2;
    GPIN  fPin3;

    GDSSD fLowGain;
    GDSSD fHighGain;

    GSSSD fSSSD;

    //std::vector<GClover> fCloverHits; 

  ClassDef(GBCS,1)
};

#endif
