#include <cstdio>
#include <vector>
#include <string>

#include <evtFile.h>
#include <evtLoop.h>
#include <ddasLoop.h>
#include <dataBlock.h>
#include <ddasHit.h>
#include <GChannel.h>

// single-threaded: decode and print raw hits (Stage 2 path)
static void printRaw(const char* fname) {
  evtFile file(fname, true);
  dataBlock block;
  ddasHit hit;

  int n = 0;
  while(file.ReadBlock(block, 0) == 1 && n < 20) {
    hit.set(block);
    hit.print();
    n++;
  }
}

// threaded: run the pipeline, build events, print summary (Stage 3 path)
static void buildEvents(const std::vector<std::string>& files) {
  evtLoop  reader(files, 500000, true);
  ddasLoop converter(reader, 200, 1);

  reader.Start();
  converter.Start();

  uint64_t nEvents = 0, nHitsSeen = 0;
  std::vector<ddasHit> event;

  while(!converter.Finished() || !converter.Empty()) {
    if(converter.TryPop(event)) {
      nEvents++;
      nHitsSeen += event.size();
      if(nEvents <= 10)
        printf("event %llu : %zu hits\n",
               (unsigned long long)nEvents, event.size());
      event.clear();
    }
  }

  reader.Stop();
  converter.Stop();

  auto e = reader.GetStats();
  auto d = converter.GetStats();
  printf("\nblocks=%llu  hits=%llu  events=%llu  hitsSeen=%llu\n",
         (unsigned long long)e.blocksRead,
         (unsigned long long)d.hitsBuilt,
         (unsigned long long)nEvents,
         (unsigned long long)nHitsSeen);
}

int main(int argc, char** argv) {
  if(argc < 2) {
    printf("usage: printHits [-e] file.evt [file.evt ...]\n");
    printf("  (default) print first 20 raw hits, single-threaded\n");
    printf("  -e        build events via the threaded pipeline\n");
    return 1;
  }

  GChannel::ReadDetmap("cals/detmap3.tsv");

  std::string first = argv[1];
  if(first == "-e") {
    std::vector<std::string> files;
    for(int i = 2; i < argc; ++i) files.push_back(argv[i]);
    buildEvents(files);
  } else {
    printRaw(argv[1]);
  }

  return 0;
}
