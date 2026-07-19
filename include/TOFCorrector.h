#ifndef __TOFCORRECTOR_H__
#define __TOFCORRECTOR_H__

#include <TObject.h>
#include <vector>
#include <string>

#define EXPECTED_PEAKS 4
#define TOF_WIDTH 30
#define PEAK1 -160.0
#define PEAK2 -155.0
#define PEAK3 -152.0
#define PEAK4 -150.0

class TSpline3;
class TH2;

class TOFCorrector : public TObject {
  public:
    TOFCorrector();
    TOFCorrector(std::string fname);
    ~TOFCorrector();

    void Copy(TOFCorrector &other) const;

    void FitTOF(TH2 *tof_time=0); 
    void MakeCorrectionFile(std::string fname="",TH2 *tof_time=0); // 

    void Draw() const;

    double Correct(double tof,double time,int peak= -1) const; 

    TH2 *CorrectedTOF() const;
    
    TSpline3* GetSpline() const { return fSplines.size()>0 ? fSplines.at(0) : 0; }

  private:
    double CommonDrift(double time) const;

    int fNPeaks;
    std::vector<int>       fExpectedBin;
    std::vector<double>    fExpectedValue;
    //std::vector<double>    fCorrectedValue;
    std::vector<TSpline3*> fSplines;    

    TH2 *fTofTime;

  ClassDef(TOFCorrector,1)
};


#endif

