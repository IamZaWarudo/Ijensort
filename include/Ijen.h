#ifndef __IJEN_H__
#define __IJEN_H__

#ifndef __CINT__
#include <thread>
#endif

#include <TRint.h>

class Ijen : public TRint {
  private:
    Ijen(int argc, char **argv);
    static Ijen *fIjen;
  public:
    static Ijen *Get(int argc=0, char **argv=0);
    virtual ~Ijen();

    long ProcessLine(const char *line, bool sync=true, int *error=0) override;
    void Terminate(int status) override;
    void LoadStyle();

  private:
    bool fTabLock;
#ifndef __CINT__
    std::thread::id fMainThreadId;
#endif

  ClassDefOverride(Ijen,0)
};
#endif