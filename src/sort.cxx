#include <cstdio>
#include <vector>
#include <string>

#include <chrono>
#include <iostream>
#include <string>
#include <algorithm>

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

  GChannel::ReadDetmap("cals/newdetmap3.tsv");
  GHistogramer::Get().SetOutFile("calhist.root");

  evtLoop  reader(files, 500000, true);
  ddasLoop converter(reader, 200, 1);
  reader.Start();
  converter.Start();

  int64_t  lastPos    = 0;
  uint64_t lastBlocks = 0;
  uint64_t lastHits   = 0;
  auto lastTime  = std::chrono::steady_clock::now();
  auto lastPrint = lastTime;

  std::cout << HIDE_CURSOR << std::flush;

  GBCS bcs;
  std::vector<ddasHit> event;
  while(!converter.Finished() || !converter.Empty()) {
    if(converter.TryPop(event)) {
      ProcessEvent(bcs, event);
      event.clear();
    }

  auto now = std::chrono::steady_clock::now();
  if(now - lastPrint > std::chrono::milliseconds(500)) {
    auto e = reader.GetStats();
    auto d = converter.GetStats();

    constexpr int barWidth = 30;
    double percent = e.Percent();
    int filled = std::clamp(static_cast<int>((percent / 100.0) * barWidth), 0, barWidth);
    std::string bar(filled, '#');
    bar.append(barWidth - filled, '-');

    double dt = std::chrono::duration<double>(now - lastTime).count();
    double mbps = 0, blockRate = 0, hitRate = 0;
    if(dt > 0) {
      mbps      = (e.filePos     - lastPos)    / dt / 1024.0 / 1024.0;
      blockRate = (e.blocksRead  - lastBlocks) / dt;
      hitRate   = (d.hitsBuilt   - lastHits)   / dt;
    }

    printf(CLEAR_LINE "[%s] %6.2f%%  file %llu/%llu  %.1f/%.1f MB  %7.1f MB/s\n",
        bar.c_str(), percent,
        (unsigned long long)e.currentFile, (unsigned long long)e.totalFiles,
        e.filePos / 1024.0 / 1024.0, e.fileSize / 1024.0 / 1024.0, mbps);
    printf(CLEAR_LINE "blocks=%llu (%6.0f/s)  hits=%llu (%6.0f/s)",
        (unsigned long long)e.blocksRead, blockRate,
        (unsigned long long)d.hitsBuilt,  hitRate);
    fflush(stdout);
    printf(CURSOR_UP);
    fflush(stdout);

    lastPos = e.filePos; lastBlocks = e.blocksRead; lastHits = d.hitsBuilt;
    lastTime = now; lastPrint = now;
  }

  }

  printf(CURSOR_DOWN "\n" SHOW_CURSOR);

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


// Fill histograms for each hit in the event
  for(auto hit : event) {
    GHistogramer::Get().Fill("All_Channel/ecal",16000,0,64000,hit.GetEcal(),
                                       300,0,300,hit.GetId());
    GHistogramer::Get().Fill("All_Channel/raw",16000,0,64000,hit.GetCharge(),
                                       300,0,300,hit.GetId());}


  // The one histogram that matters first: the SSSD spectrum that tells us t_h1/t_h2.
  double sssdMax = bcs.fSSSD.MaximumEnergy();
  if(sssdMax > 0)
    GHistogramer::Get().Fill("sssd/max_energy", 4000, 0, 32000, sssdMax);


  //Position Plots
  if(bcs.fHighGain.HasPosition()){     // High-gain -> Decay (car crash down the street)
    GHistogramer::Get().Fill("dssd/high_gain_position", 40, 0, 40, bcs.fHighGain.X(),
                                                        40, 0, 40, bcs.fHighGain.Y());}
  if(bcs.fLowGain.HasPosition()){     // Low-gain -> Implant (meteor crash down the street)
    GHistogramer::Get().Fill("dssd/low_gain_position", 40, 0, 40, bcs.fLowGain.X(),
                                                       40, 0, 40, bcs.fLowGain.Y());}

}
