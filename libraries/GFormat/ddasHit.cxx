#include<ddasHit.h>

#include<cmath>
#include<limits>


#include<dataBlock.h>
#include<GChannel.h>
#include<globals.h>

#include <TRandom3.h>

ddasHit::ddasHit() { Clear(); }

ddasHit::~ddasHit() { } 


void ddasHit::Clear() {
  evId   = ULLONG_MAX;
  id     = INT_MAX;
  charge = 0.0;//sqrt(-1);
  ecal   = 0.0;
  ecal_labr = 0.0;
  //time   = ULLONG_MAX;
  time   = std::numeric_limits<double>::quiet_NaN();
  cfd    = INT_MAX;
  //for(int i=0;i<8;i++) qdc[i] = INT_MAX;
  qdc.fill(0);
  hasQDC = false;
  traceLength=0;
  trace.clear();
  forcedCFD = false;
}

void ddasHit::print() const {
  printf("ddas hit @ %f\n", GetTime()+GetCFD());
  printf("\tevId:    %llu\n",evId);
  printf("\tid:      %i\n",id);
  printf("\tcharge:  %.1f\n",charge);
  printf("\tecal:    %.1f\n",ecal);
  printf("\tecal_labr: %.1f\n",ecal_labr);
  printf("\thasQDC: %i\n", hasQDC);
  if(hasQDC) {
    printf("\t\t");
    for(size_t i=0;i<qdc.size();i++) 
      printf("[%lu]:%i ",i,qdc.at(i));
    printf("\n");
  }
  printf("\ttraceLength: %i\n",traceLength);
  if(traceLength) {
    printf("\t\t");
    for(int i=0;i<traceLength;i++) {
      if(i!=0 && (i%16==0))
        printf("\n\t\t");
      printf("%i  ",trace.at(i));
    }
    printf("\n");
  }
  printf("---------------------------------\n");
  fflush(stdout);
}

void ddasHit::Copy(ddasHit& lhs) const {

  lhs.evId = evId;
  lhs.id   = id;

  lhs.setEvId(evId);
  lhs.setId(id);
  lhs.setCharge(charge);
  lhs.setEcal(ecal);
  lhs.setEcal_labr(ecal_labr);
  lhs.setTime(time);
  lhs.setCFD(cfd);
  lhs.setQDC(qdc);
  lhs.setTraceLength(traceLength);
  lhs.setTrace(trace);



}

bool ddasHit::Calibrate(const GChannel *channel) const  { 
  if(!channel)
    channel = GChannel::Get(GetAddress());
  if(!channel)
    return false;
  
  static TRandom3 rng(0);
  double temp = charge + rng.Uniform();  
  
  //int counter=0; 
  //for(const auto par : channel->fCalPars) {
  //  ecal += par*std::pow(temp,counter);
  //  counter++;
  //}
  if(channel->fCalPars.size()==2) 
    ecal = channel->fCalPars[0] + temp*channel->fCalPars[1];


  return true;
}

bool ddasHit::Calibrate_LaBr(const GChannel *channel) const {
  if(!channel)
    channel = GChannel::Get(GetAddress());
  if(!channel)
    return false;

  //if(hasQDC && !forcedCFD) {
    double add = qdc[2] + qdc[3];
    double sub = qdc[1];
    double eqdc = add-sub*(140./100.);
    //if(channel->fCalPars.size()==2)
      ecal_labr = channel->fCalPars[0] + eqdc*channel->fCalPars[1];
  //} else {
    //ecal_labr = 0;
  //}


  return true;
}


void ddasHit::set(const dataBlock& data) { 

  //GChannel::Get(data.address())->Print();
  // check the digitizer 
  // set correctly depending on digitizer 
  evId = data.eventID;
  id   = data.crate*MAX_BOARDS_PER_CRATE*MAX_CHANNELS_PER_BOARD + 
        (data.slot-BOARD_START)*MAX_CHANNELS_PER_BOARD         + 
         data.ch;
  charge  = data.energy;
  address = data.address();

  time = static_cast<double>(data.time);
  cfd  = 0.0;
 
  auto *chan = GChannel::Get(data.address());
  Calibrate(chan);

  setForceCFD(data.cfd_forced);

  //printf(RED " crate slot channel  = %i %i %i" RESET_COLOR "\n",data.crate,data.slot,data.ch); fflush(stdout);
  //printf(RED " chan = %p " RESET_COLOR "\n",chan); fflush(stdout);

  switch(chan->fMHz) {
    case 100:
      if(data.cfd_forced==0) {
        cfd = static_cast<double>(data.cfd & 0x7fff)/32768.0;
      }
      break;
    case 250:
       if(data.cfd_forced == 0) {
        const double frac = static_cast<double>(data.cfd & 0x3fff) / 16384.0;
        const double src  = data.cfd_source ? 0.5 : 0.0;
        cfd = frac + src;
      }
      break;
    case 500:
     {
        const int source = ((data.cfd >> 13) & 0x7)-1;
        const int frac   = data.cfd & 0x1fff;

        //if(source >=0 && source <=4) {
        //  cfd = (static_cast<double>(source) +
        //         static_cast<double>(frac) / 8192.0) / 5.0;
        //}
        if(source==7) 
          setForceCFD(data.cfd_forced);
        if(source >=0 && source <=4) {
          cfd = (static_cast<double>(source) +
                 static_cast<double>(frac) / 8192.0);
        }
        //time = (time*5 + static_cast<double>(source)-1 static_cast<double>(frac)/8192.)*2
        //2 NS BINS?  compress correctly.

      }
      break;
    default:
      break;
  }

  //cfd corrected time values should be time-cfd.

  //if(data.QDCsum[0]>1) {
  //  hasQDC = true;
  //  for( int i = 0; i < 8; i++) {
  //    qdc[i] = data.QDCsum[i];
  //  }
  //}
hasQDC = (data.headerLength == 12 || data.headerLength == 16);

if(hasQDC) {
  for(int i = 0; i < 8; ++i)
    qdc[i] = data.QDCsum[i];
}

Calibrate_LaBr(chan);


  traceLength = data.trace_length;
  // std::vector<unsigned short> trace;
  if(traceLength > 0) {
    trace.assign(data.trace.begin(), data.trace.end());
  } else {
    trace.clear();
  }
}



//ddasHit ddasHit::operator=(ddasHit const& rhs) {
//  ddasHit hit;
//  rhs.Copy(hit);
//  return hit;
//}

bool ddasHit::operator==(ddasHit const& rhs) {
  if( evId==rhs.evId && id==rhs.id) 
    return true;
  return false;
}

bool ddasHit::operator<(ddasHit const & rhs) const {
  //double right = rhs.time + rhs.cfd/16384.;
  //double left  = time + cfd/16384.;
  //return left<right;
  if(GetId() == rhs.GetId()) {
    double right = rhs.time + rhs.cfd/16384.;
    double left  = time + cfd/16384.;
    return left<right;
  }
  return GetId() <= rhs.GetId();
}

