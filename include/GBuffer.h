#ifndef __GBUFFER_H__
#define __GBUFFER_H__

#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <queue>
#include <utility>
#include <vector>

#include "dataBlock.h"
#include "ddasHit.h"

class dataBlockBuffer {
  public:
    explicit dataBlockBuffer(uint64_t orderingWindow = 0,
                             std::size_t maxBufferedBlocks = 131072)
      : fOrderingWindow(orderingWindow),
        fMaxBufferedBlocks(maxBufferedBlocks) {}

    void Push(std::unique_ptr<dataBlock> block) {
      std::unique_lock<std::mutex> lock(fMutex);
      fNotFull.wait(lock, [this] { return HasSpace(); });

      const bool wasReady = HasReadyBlock();
      const uint64_t timestamp = block->time;
      if(timestamp > fLatestTimestamp)
        fLatestTimestamp = timestamp;
      fBuffer.emplace(timestamp, std::move(block));
      const bool becameReady = !wasReady && HasReadyBlock();
      lock.unlock();
      if(becameReady)
        fNotEmpty.notify_one();
    }

    bool TryPop(std::unique_ptr<dataBlock>& block) {
      std::unique_lock<std::mutex> lock(fMutex);

      if(fBuffer.empty())
        return false;
      if(!fFlushing && !HasReadyBlock())
        return false;

      const bool wasFull = !HasSpace();
      auto it = fBuffer.begin();
      block = std::move(it->second);
      fBuffer.erase(it);
      lock.unlock();
      if(wasFull)
        fNotFull.notify_one();
      return true;
    }

    bool WaitPop(std::unique_ptr<dataBlock>& block) {
      std::unique_lock<std::mutex> lock(fMutex);
      fNotEmpty.wait(lock, [this] { return fFlushing || HasReadyBlock(); });

      if(fBuffer.empty())
        return false;

      const bool wasFull = !HasSpace();
      auto it = fBuffer.begin();
      block = std::move(it->second);
      fBuffer.erase(it);
      lock.unlock();
      if(wasFull)
        fNotFull.notify_one();
      return true;
    }

    void Flush() {
      std::lock_guard<std::mutex> lock(fMutex);
      fFlushing = true;
      fNotEmpty.notify_all();
    }

    bool Empty() const {
      std::lock_guard<std::mutex> lock(fMutex);
      return fBuffer.empty();
    }

    std::size_t Size() const {
      std::lock_guard<std::mutex> lock(fMutex);
      return fBuffer.size();
    }

    std::size_t Capacity() const { return fMaxBufferedBlocks; }

  private:
    bool HasSpace() const {
      return fMaxBufferedBlocks == 0 || fBuffer.size() < fMaxBufferedBlocks;
    }

    bool HasReadyBlock() const {
      if(fBuffer.empty())
        return false;
      const auto oldestTimestamp = fBuffer.begin()->first;
      return oldestTimestamp + fOrderingWindow <= fLatestTimestamp;
    }
    mutable std::mutex fMutex;
    std::condition_variable fNotEmpty;
    std::condition_variable fNotFull;
    
    std::multimap<uint64_t, std::unique_ptr<dataBlock> > fBuffer;
    bool fFlushing{false};
    uint64_t fLatestTimestamp{0};
    uint64_t fOrderingWindow{0};
    std::size_t fMaxBufferedBlocks{0};
};

class ddasBuffer {
  public:
    explicit ddasBuffer(double buildWindow = 0.0,
                        std::size_t maxBufferedEvents = 4096)
      : fBuildWindow(buildWindow),
        fMaxBufferedEvents(maxBufferedEvents) {
      fCurrentEvent.reserve(256);
    }

    bool Push(ddasHit hit) {
      std::unique_lock<std::mutex> lock(fMutex);
      
      const double t = hit.GetTime();
      
      if(fCurrentEvent.empty()) {
        fCurrentStart = t;
        fCurrentEvent.emplace_back(std::move(hit));
        return false;
      }

      if(std::abs(t - fCurrentStart) <= fBuildWindow) { //GetTime is cfd corrected; abs for safety
        fCurrentEvent.emplace_back(std::move(hit));
        return false;
      }

      fNotFull.wait(lock, [this] { return HasSpace(); });
      const bool wasEmpty = fBuiltEvents.empty();
      fBuiltEvents.push(std::move(fCurrentEvent));
      fCurrentEvent.clear();
      fCurrentEvent.emplace_back(std::move(hit));
      fCurrentStart = t;

      lock.unlock();
      if(wasEmpty)
        fNotEmpty.notify_one();
      return true;
    }

    bool TryPop(std::vector<ddasHit>& event) {
      std::unique_lock<std::mutex> lock(fMutex);

      if(fBuiltEvents.empty())
        return false;
      const bool wasFull = !HasSpace();
      event = std::move(fBuiltEvents.front());
      fBuiltEvents.pop();
      lock.unlock();
      if(wasFull)
        fNotFull.notify_one();
      return true;
    }

    bool WaitPop(std::vector<ddasHit>& event) {
      std::unique_lock<std::mutex> lock(fMutex);
      fNotEmpty.wait(lock, [this] { return fFlushing || !fBuiltEvents.empty(); });

      if(fBuiltEvents.empty())
        return false;

      const bool wasFull = !HasSpace();
      event = std::move(fBuiltEvents.front());
      fBuiltEvents.pop();
      lock.unlock();
      if(wasFull)
        fNotFull.notify_one();
      return true;
    }

    void Flush() {
      std::unique_lock<std::mutex> lock(fMutex);

      if(!fCurrentEvent.empty()) {
        fNotFull.wait(lock, [this] { return HasSpace(); });
        fBuiltEvents.push(std::move(fCurrentEvent));
        fCurrentEvent.clear();
      }

      fFlushing = true;
      lock.unlock();
      fNotEmpty.notify_all();
    }

    bool Empty() const {
      std::lock_guard<std::mutex> lock(fMutex);
      return fBuiltEvents.empty() && fCurrentEvent.empty();
    }

    std::size_t Size() const {
      std::lock_guard<std::mutex> lock(fMutex);
      return fBuiltEvents.size();
    }

    std::size_t Capacity() const { return fMaxBufferedEvents; }

  private:
    bool HasSpace() const {
      return fMaxBufferedEvents == 0 || fBuiltEvents.size() < fMaxBufferedEvents;
    }

    double fBuildWindow{0.0};
    double fCurrentStart{0.0};

    std::vector<ddasHit> fCurrentEvent;
    std::queue<std::vector<ddasHit>> fBuiltEvents;

    mutable std::mutex fMutex;
    std::condition_variable fNotEmpty;
    std::condition_variable fNotFull;

    bool fFlushing{false};
    std::size_t fMaxBufferedEvents{0};
};

#endif
