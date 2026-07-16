#ifndef __GHISTOGRAMER_H__
#define __GHISTOGRAMER_H__


#include <atomic>
#include <cmath>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>
#include <TH2.h>


class GHistogramer {
  public:
    static GHistogramer& Get();
    
  private:
    struct HistogramDefinition {
      int xbins;
      double xlow;
      double xhigh;
      int ybins;
      double ylow;
      double yhigh;
    };

    struct ThreadHistogramStore;

    GHistogramer();
    GHistogramer(const GHistogramer&)            = delete;
    GHistogramer& operator=(const GHistogramer&) = delete;
    ~GHistogramer();
    
  public: //private:
    void Close();
    void Print(Option_t *opt="") const;

    void Fill(const std::string& pathName,
                     int xbins,double xlow,double xhigh,double xvalue,
                     int ybins=-1,double ylow=sqrt(-1),double yhigh=sqrt(-1),double yvalue=sqrt(-1));

    TH1* Create(TDirectory* dir, const std::string& name, 
              int xbins,double xlow,double xhigh,
              int ybins=-1,double ylow=sqrt(-1),double yhigh=sqrt(-1));

    TDirectory* GetDirectory(const std::string& path);

    std::string makeKey(const std::string& path, const std::string& name) const {
      if (path.empty()) return name;
      return path + "/" + name;
    }

    void SetBaseDirectory(TDirectory *dir);
    void SetOutFile(const std::string& fname="output.root",const std::string& option="recreate");


  private:
    std::shared_ptr<ThreadHistogramStore> GetThreadHistogramStore();
    TH1* CreateHistogram(const std::string& pathName,
                         const HistogramDefinition& definition);
    void ResetHistogramsLocked();
    void MergeHistogramsLocked();

    std::mutex fMutex; 
    std::unordered_map<std::string,TH1*> fH1; 
    std::unordered_map<std::string, TDirectory*> fDirCache;
    std::unordered_map<std::string, HistogramDefinition> fDefinitions;
    std::vector<std::shared_ptr<ThreadHistogramStore>> fThreadStores;
    TDirectory* fBaseDir  = nullptr;
    std::atomic<uint64_t> fGeneration{0};
    bool fClosed{false};

  ClassDef(GHistogramer,0)  
};



#endif
