#ifndef __PIDGATES_H__
#define __PIDGATES_H__

#include <string>
#include <vector>

class TCutG;

class PIDGates {
  private:
    PIDGates() = default;
  public:
    static PIDGates& Get();

    void Load(const std::string& fname);

    // Isotope ID for a point on the PID plot, or "" if in no gate.
    std::string Identify(double tof, double de) const;

    int Size() const { return fCuts.size(); }

  private:
    std::vector<TCutG*> fCuts;
};

#endif
 