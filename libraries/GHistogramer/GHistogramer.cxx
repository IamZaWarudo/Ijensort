
#include <GHistogramer.h>
#include <utils.h>
#include <TROOT.h>
#include <TList.h>

namespace {

void SortDirectoryTree(TDirectory* directory) {
  if(!directory)
    return;

  TList* objects = directory->GetList();
  if(!objects)
    return;

  objects->Sort();
  TIter next(objects);
  while(TObject* object = next()) {
    if(object->InheritsFrom(TDirectory::Class()))
      SortDirectoryTree(static_cast<TDirectory*>(object));
  }
}

} // namespace

struct GHistogramer::ThreadHistogramStore {
  explicit ThreadHistogramStore(uint64_t generation)
    : generation(generation) {}

  uint64_t generation;
  std::unordered_map<std::string, std::unique_ptr<TH1>> histograms;
};

GHistogramer& GHistogramer::Get() {
  static GHistogramer instance;
  return instance;
}

GHistogramer::GHistogramer() {
  fBaseDir = gROOT;
}

GHistogramer::~GHistogramer() = default;

void GHistogramer::Close() { 
  std::lock_guard<std::mutex> lock(fMutex);

  if(fClosed)
    return;

  MergeHistogramsLocked();

  if(fBaseDir && fBaseDir->InheritsFrom(TFile::Class())) {
    TFile *file = static_cast<TFile*>(fBaseDir);
    SortDirectoryTree(file);
    file->Write();
    file->Close();
  }

  fClosed = true;
  printf("histogramer closed.\n");
}

void GHistogramer::SetBaseDirectory(TDirectory* dir) { 
  std::lock_guard<std::mutex> lock(fMutex);
  fBaseDir = dir ? dir : gROOT;
  ResetHistogramsLocked();
}

void GHistogramer::SetOutFile(const std::string& fname,const std::string& option) {
  TFile *outfile = new TFile(fname.c_str(),option.c_str());
  SetBaseDirectory(outfile);
}


TDirectory* GHistogramer::GetDirectory(const std::string& path) {
  if(!fBaseDir || path.empty()) return fBaseDir ? fBaseDir : gROOT;

  auto it = fDirCache.find(path);
  if(it!=fDirCache.end()) return it->second;

  TDirectory *dir = fBaseDir;
  auto parts = tokenizeString(path);
 
  for(const auto&p : parts) {
    TDirectory* sub = dir->GetDirectory(p.c_str());
    if(!sub) { 
      sub = dir->mkdir(p.c_str());
    }
    dir = sub;
  }
  fDirCache[path] = dir;
  return dir;
}

TH1* GHistogramer::Create(TDirectory* dir, const std::string& name,
                           int xbins,double xlow,double xhigh,
                           int ybins,double ylow,double yhigh) { 
  TDirectory* oldDir = gDirectory;
  if(dir) dir->cd();
  TH1* hist = 0;
  if(ybins>0) 
    hist = new TH2D(name.c_str(),name.c_str(),xbins,xlow,xhigh,ybins,ylow,yhigh);
  else
    hist = new TH1D(name.c_str(),name.c_str(),xbins,xlow,xhigh);
  hist->SetDirectory(dir);
  if(oldDir) oldDir->cd();
  return hist;
}

void GHistogramer::Fill(const std::string& pathName,        
                        int xbins,double xlow,double xhigh,double xvalue,       
                        int ybins,double ylow,double yhigh,double yvalue) {
  auto store = GetThreadHistogramStore();
  auto it = store->histograms.find(pathName);

  if(it == store->histograms.end()) {
    const HistogramDefinition definition{xbins, xlow, xhigh, ybins, ylow, yhigh};
    std::lock_guard<std::mutex> lock(fMutex);

    auto [definitionIt, inserted] = fDefinitions.emplace(pathName, definition);
    if(!inserted) {
      const auto& existing = definitionIt->second;
      if(existing.xbins != definition.xbins ||
         existing.xlow != definition.xlow ||
         existing.xhigh != definition.xhigh ||
         existing.ybins != definition.ybins ||
         existing.ylow != definition.ylow ||
         existing.yhigh != definition.yhigh) {
        printf("Histogram '%s' was requested with different binning; using its original definition.\n",
               pathName.c_str());
      }
    }

    auto histogram = std::unique_ptr<TH1>(CreateHistogram(pathName, definitionIt->second));
    it = store->histograms.emplace(pathName, std::move(histogram)).first;
  }

  TH1* hist = it->second.get();
  if(ybins>0) 
    hist->Fill(xvalue,yvalue);
  else 
    hist->Fill(xvalue);
}

std::shared_ptr<GHistogramer::ThreadHistogramStore>
GHistogramer::GetThreadHistogramStore() {
  static thread_local std::unordered_map<GHistogramer*,
                                         std::shared_ptr<ThreadHistogramStore>> stores;

  const uint64_t generation = fGeneration.load(std::memory_order_acquire);
  auto it = stores.find(this);
  if(it != stores.end() && it->second->generation == generation)
    return it->second;

  auto store = std::make_shared<ThreadHistogramStore>(generation);
  {
    std::lock_guard<std::mutex> lock(fMutex);
    fThreadStores.push_back(store);
  }
  stores[this] = store;
  return store;
}

TH1* GHistogramer::CreateHistogram(const std::string& pathName,
                                   const HistogramDefinition& definition) {
  const auto parts = tokenizeString(pathName);
  return Create(nullptr, parts.back(),
                definition.xbins, definition.xlow, definition.xhigh,
                definition.ybins, definition.ylow, definition.yhigh);
}

void GHistogramer::ResetHistogramsLocked() {
  fH1.clear();
  fDirCache.clear();
  fDefinitions.clear();
  fThreadStores.clear();
  fClosed = false;
  fGeneration.fetch_add(1, std::memory_order_release);
}

void GHistogramer::MergeHistogramsLocked() {
  fH1.clear();

  for(const auto& [pathName, definition] : fDefinitions) {
    std::string path;
    const auto parts = tokenizeString(pathName);
    const std::string& name = parts.back();
    if(parts.size() > 1) {
      path = parts.front();
      for(size_t i = 1; i + 1 < parts.size(); ++i)
        path += "/" + parts[i];
    }

    TH1* merged = nullptr;
    for(const auto& store : fThreadStores) {
      auto it = store->histograms.find(pathName);
      if(it == store->histograms.end())
        continue;

      if(!merged) {
        merged = it->second.release();
        merged->SetDirectory(GetDirectory(path));
      } else {
        merged->Add(it->second.get());
      }
    }

    if(!merged) {
      merged = Create(GetDirectory(path), name,
                      definition.xbins, definition.xlow, definition.xhigh,
                      definition.ybins, definition.ylow, definition.yhigh);
    }
    fH1[pathName] = merged;
  }
}


void GHistogramer::Print(Option_t *opt) const {
  int counter =0;
  for(const auto& item : fDefinitions) {
    printf("%i:    {%s}\n",counter++,item.first.c_str());
  }

}


