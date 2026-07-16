#ifndef __DDASHIT_H__
#define __DDASHIT_H__

#include<vector>
#include<array>
#include<climits>


#include<TObject.h>

class dataBlock;
class GChannel;

class ddasHit { //: public TObject {
  public:
    ddasHit();
    ddasHit(const ddasHit&) = default;
    ddasHit(ddasHit&&) noexcept = default;
    virtual ~ddasHit();

   void setEvId(const unsigned long long EVID) { evId   = EVID; }  // evID
   void setId(const int ID)                    { id    = ID; }
   void setCharge(const double E)              { charge = E; }
   void setEcal(const double E)                { ecal = E; }
   void setEcal_labr(const double E)           { ecal_labr = E; }
   void setTime(const double T)                { time   = T; }
   void setCFD(const double CFD)                  { cfd = CFD; }
   //void setQDC(const std::vector<int> &QDC)     { qdc = QDC; }
   void setQDC(const std::array<int,8>& QDC)   { qdc = QDC; }
   void setTraceLength(const int TL)           { traceLength = TL; }
   void setTrace(const std::vector<unsigned short> &TRACE)  { trace = TRACE; }
   void setForceCFD(const bool forced)         { forcedCFD = forced; }

   void set(const dataBlock& data);


   //void SetCustomQDC(const val)  { cqdc = val; }

   void Clear();
   void Copy(ddasHit& lhs) const;


   ddasHit& operator=(const ddasHit&)     = default;
   ddasHit& operator=(ddasHit&&) noexcept = default;
   //ddasHit operator=(ddasHit const& rhs);
   bool    operator==(ddasHit const& rhs);
   bool    operator<(ddasHit const& rhs) const;

   bool Calibrate(const GChannel *c=0) const;
   bool Calibrate_LaBr(const GChannel *c=0) const;

  public:
    uint32_t GetAddress()  const { return address; }
    double GetCharge()     const { return charge; }
    double GetEcal()       const { return ecal; }
    double GetEcal_labr()  const { return ecal_labr; }
    //double GetTime()    const { return time; }//+ cfd/16384.; }
    double GetTime()       const { return time  +cfd; }//+ cfd/16384.; }
    double GetTimestamp()  const { return time; }//+ cfd/16384.; }
    double GetCFD()        const { return cfd; }
    int    GetId()         const { return id;  }
    bool   GetForcedCFD()  const { return forcedCFD; }

    const std::vector<unsigned short> &GetTrace() const { return trace; }
    //const std::vector<int> &GetQDCSums() const { return qdc; }
    const std::array<int,8>& GetQDC() const { return qdc; }
    bool hasQDC{false};


    void print() const;

  private:
    uint32_t address;
    unsigned long long evId;
    int id;
    double charge;
    mutable double ecal;
    mutable double ecal_labr;
    double time;
    double cfd;
    bool   forcedCFD;
    //int qdc[8];
    //std::vector<int> qdc;
    //double           cqdc
    std::array<int,8> qdc{};


    int traceLength;
    std::vector<unsigned short> trace;

  ClassDef(ddasHit,1);
};


#endif
