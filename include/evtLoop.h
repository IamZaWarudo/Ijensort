#ifndef __EVTLOOP_H__
#define __EVTLOOP_H__

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <string>
#include <utility>
#include <memory>
#include <vector>

#include <GThread.h>
#include <GBuffer.h>
#include <evtFile.h>
#include <dataBlock.h>

struct evtStats {
  uint64_t blocksRead{0};
  uint64_t skippedItems{0};
  int64_t  filePos{0};
  int64_t  fileSize{0};
  uint64_t currentFile{0};
  uint64_t totalFiles{0};
  uint64_t bufferedBlocks{0};
  uint64_t maxBufferedBlocks{0};

  double Percent() const {
    if(fileSize <= 0) return 0.0;
    return 100.0 * static_cast<double>(filePos) / static_cast<double>(fileSize);
  }
};

class evtLoop : public GThread {
public:
  explicit evtLoop(const std::string& filename,
                   uint64_t orderingWindow = 0,
                   bool isNSCL = false,
                   std::size_t maxBufferedBlocks = 131072)
    : evtLoop(std::vector<std::string>{filename}, orderingWindow, isNSCL,
              maxBufferedBlocks) {}

  explicit evtLoop(std::vector<std::string> filenames,
                   uint64_t orderingWindow = 0,
                   bool isNSCL = false,
                   std::size_t maxBufferedBlocks = 131072)
    : fFiles(std::move(filenames)),
      fBuffer(orderingWindow, maxBufferedBlocks),
      fIsNSCL(isNSCL) {
    for(const auto& filename : fFiles) {
      std::error_code error;
      const auto fileSize = std::filesystem::file_size(filename, error);
      if(!error)
        fTotalFileBytes += fileSize;
    }

    OpenNextFile();
  }

  dataBlockBuffer& Buffer() { return fBuffer; }

  bool Finished() const { return fFinished; }
  bool Empty() const { return fBuffer.Empty(); }

  bool TryPop(std::unique_ptr<dataBlock>& block) {
    return fBuffer.TryPop(block);
  }

  evtStats GetStats() const {
    evtStats s;
    s.blocksRead  = fBlocksRead;
    s.skippedItems = fSkippedItems;
    s.filePos     = fCompletedFileBytes + fCurrentFilePos;
    s.fileSize    = fTotalFileBytes;
    s.currentFile = fCurrentFile;
    s.totalFiles  = fFiles.size();
    s.bufferedBlocks = fBuffer.Size();
    s.maxBufferedBlocks = fBuffer.Capacity();
    return s;
  }

protected:
  void Iteration() override {
    auto block = std::make_unique<dataBlock>();
    //fBlock.Clear();

    const int status = fFile.ReadBlock(*block, 0);
    fCurrentFilePos = fFile.GetFilePos();

    if(status == 1) {
      fBlocksRead++;
      fBuffer.Push(std::move(block));
      return;
    }

    if(status == -2) {
      fSkippedItems++;
      return; // skip NSCL non-physics item, keep reading
    }

    fCompletedFileBytes += fFile.GetFileSize();
    fCurrentFilePos = 0;

    if(OpenNextFile())
      return;

    // Final EOF or error: release remaining ordered blocks, mark finished, stop loop.
    Flush();
    RequestStop();
  }

  void Flush() override {
    fBuffer.Flush();   // tells buffer to stop holding back ordering-window tail
    fFinished = true;
  }

private:
  bool OpenNextFile() {
    fFile.CloseFile();

    while(fNextFile < fFiles.size()) {
      const auto& filename = fFiles[fNextFile++];
      fFile.OpenFile(filename, fIsNSCL);

      if(fFile.IsOpen()) {
        fCurrentFile = fNextFile;
        fCurrentFilePos = 0;
        return true;
      }
    }

    return false;
  }

  std::vector<std::string> fFiles;
  std::size_t fNextFile{0};
  std::atomic<uint64_t> fCurrentFile{0};
  uint64_t fTotalFileBytes{0};
  std::atomic<uint64_t> fCompletedFileBytes{0};
  std::atomic<int64_t> fCurrentFilePos{0};

  evtFile         fFile;
  dataBlockBuffer fBuffer;
  bool fIsNSCL{false};
  std::atomic<uint64_t> fBlocksRead{0};
  std::atomic<uint64_t> fSkippedItems{0};



  std::atomic<bool> fFinished{false};
};

#endif
