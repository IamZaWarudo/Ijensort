#include <GChannel.h>

#include <fstream>
#include <sstream>

#include <globals.h>

std::unordered_map<uint32_t,GChannel*> GChannel::fAddressMap; 
//std::map<uint32_t,GChannel*> GChannel::fNumberMap; 

GChannel::GChannel() { } 


GChannel *GChannel::ParseLine(std::string line) { 


  GChannel *ch = new GChannel;

  std::stringstream ss(line);

  ss >> std::hex >> ch->fAddress; 
  ss >> std::dec >> ch->fMHz >> ch->fCrate >> ch->fSlot >> ch->fChannel >> ch->fName >> ch->fNumber;

  double par;
  while(ss >> par) 
    ch->fCalPars.push_back(par);

  //printf("fAddress = 0x%0x\n",ch->fAddress);

  ch->fAddressMap[ch->fAddress] = ch;
  //ch->fNumberMap[ch->fNumber]   = ch;
  return ch;
} 


GChannel::~GChannel() { } 

void GChannel::ReadDetmap(std::string fname) {
  std::string line;
  std::ifstream infile;
  infile.open(fname.c_str());
  int counter=0;
  while(std::getline(infile,line)) {
    if(line.length()<1) continue;
    if(line[0]=='#') continue;
    ParseLine(line);
    counter++;
    //printf("line: %s\n",line.c_str());
  }
  printf(BLUE "\tloaded %i channels from %s" RESET_COLOR "\n",counter,fname.c_str());

}


void GChannel::Print() const { 

  printf("GChannel: 0x%08x\n",fAddress);
  printf("\tName:    %s\n",fName.c_str());
  printf("\tNumber:  %i\n",fNumber);
  printf("\tMHz:     %i\n",fMHz);
  printf("-----------------\n");


}
