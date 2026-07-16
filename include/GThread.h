#ifndef __GTHREAD_H__
#define __GTHREAD_H__

#include <atomic>
#include <thread>

class GThread {
public:
  GThread() = default;

  virtual ~GThread() {
    Stop();
  }

  void Start() {
    fRunning = true;
    fWorker = std::thread(&GThread::Loop, this);
  }

  void Stop() {
    fRunning = false;

    if(fWorker.joinable() &&
       fWorker.get_id() != std::this_thread::get_id()) {
      fWorker.join();
    }
  }

protected:
  virtual void Iteration() = 0;
  virtual void Flush() {}

  bool Running() const { return fRunning; }

  void RequestStop() { fRunning = false; }

private:
  void Loop() {
    while(fRunning) {
      Iteration();
    }

    Flush();
  }

  std::atomic<bool> fRunning{false};
  std::thread fWorker;
};

#endif
