#include <cstdio>
#include <vector>
#include <string>

#include <evtLoop.h>
#include <ddasLoop.h>
#include <ddasHit.h>
#include <GChannel.h>
#include <GHistogramer.h>
#include <GBCS.h>
#include <globals.h>

void ProcessEvent(GBCS& bcs, const std::vector<ddasHit>& event);

int main(int argc, char** argv) {
  if(argc < 2) { printf("usage: sort file.evt [file.evt ...]\n"); return 1; }

  std::vector<std::string> files;
  for(int i = 1; i < argc; ++i) files.push_back(argv[i]);

  GChannel::ReadDetmap("cals/detmap3.tsv");
  GHistogramer::Get().SetOutFile("hist.root");

  evtLoop  reader(files, 500000, true);
  ddasLoop converter(reader, 200, 1);
  reader.Start();
  converter.Start();

  GBCS bcs;
  std::vector<ddasHit> event;
  while(!converter.Finished() || !converter.Empty()) {
    if(converter.TryPop(event)) {
      ProcessEvent(bcs, event);
      event.clear();
    }
  }

  reader.Stop();
  converter.Stop();
  GHistogramer::Get().Close();
  return 0;
}

void ProcessEvent(GBCS& bcs, const std::vector<ddasHit>& event) {
  bcs.Reset();

for(const auto &hit : event) {
    switch(hit.GetId()) {
      case 0 ... 39:     // front High Gain
        bcs.fHighGain.AddFrontHit(hit);
        break;
      case 40 ... 79:    // front Low  Gain
        bcs.fLowGain.AddFrontHit(hit);
        break;
      case 80 ... 119:   // back  High Gain
        bcs.fHighGain.AddBackHit(hit);
        break;
      case 120 ... 159:  // back  Low  Gain
        bcs.fLowGain.AddBackHit(hit);
        break;
      case 176:
        bcs.fI2N.Unpack(hit);
        break;
      case 177:
        bcs.fI2S.Unpack(hit);
        break;
      case 180:
        bcs.fI2TAC.Unpack(hit);
        break;
      case 181:
        bcs.fPin1.Unpack(hit);
        break;
      case 182:
        bcs.fPin2.Unpack(hit);
        break;
      case 183:
        bcs.fPin3.Unpack(hit);
        break;
      case 272 ... 287:  // SSSD strips 0-15 (16 strips)
        bcs.fSSSD.AddHit(hit);
      break;

      default:
        break;
    }
  }

  bcs.fHighGain.Build();
  bcs.fLowGain.Build();

  // The one histogram that matters first: the SSSD spectrum that tells us t_h1/t_h2.
  double sssdMax = bcs.fSSSD.MaximumEnergy();
  if(sssdMax > 0)
    GHistogramer::Get().Fill("sssd/max_energy", 4000, 0, 32000, sssdMax);
}
