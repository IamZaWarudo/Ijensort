#include <evtFile.h>
#include <dataBlock.h>
#include <ddasHit.h>
#include <GChannel.h>

int main(int argc, char** argv) {
  if(argc < 2) { printf("usage: printHits file.evt\n"); return 1; }

  GChannel::ReadDetmap("cals/detmap3.tsv");   // path TBD

  evtFile file(argv[1], true);               // true = NSCL
  dataBlock block;
  ddasHit hit;

  int n = 0;
  while(file.ReadBlock(block, 0) == 1 && n < 20) {
    hit.set(block);
    hit.print();
    n++;
  }
  return 0;
}
