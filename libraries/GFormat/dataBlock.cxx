#include <dataBlock.h>
#include <cstdio>
#include <cinttypes>

void dataBlock::Clear(){
  ch                 = 0;
  slot               = 0;
  crate              = 0;
  headerLength       = 0;
  eventLength        = 0;
  pileup             = false;
  time               = 0;
  cfd_forced         = false;
  cfd_source         = false;
  cfd                = 0;
  energy             = 0;
  trace_length       = 0;
  trace_out_of_range = false;
  eventID            = 0;
  ClearQDC();
  ClearTrace();
}

void dataBlock::ClearQDC(){
  trailing  = 0;
  leading   = 0;
  gap       = 0;
  baseline  = 0;
  QDCsum.fill(-1);
}

void dataBlock::ClearTrace(){
  //for( int i = 0 ; i < MAX_TRACE_LENGTH; i++) trace[i] = 0;
  trace_length=0;
  trace.clear();
}


void dataBlock::Print(int opt){
  printf("============== eventID : %" PRIu64 "\n", eventID);
  printf("Crate: %d, Slot: %d, Ch: %d \n", crate, slot, ch);
  printf("HeaderLength: %d, Event Length: %d, energy: %d, timeStamp: %" PRIu64 "n", headerLength, eventLength, energy, time);
  printf("trace_length: %d, pile-up:%d\n", trace_length, pileup);
  printf("CFD : %d , Forced : %d , Source : %d\n", cfd, cfd_forced, cfd_source);
  if( headerLength > 4 ){
    if( headerLength > 12 ){
      printf(" trailing : %d\n", trailing);
      printf(" leading  : %d\n", leading);
      printf(" gap      : %d\n", gap);
      printf(" baseLine : %d\n", baseline);
    }
    if( opt >= 1 ){
      printf(" QDCsum : \n");
      for( int i = 0; i < 8; i++) printf("    %-10d\n", QDCsum[i]);
    }
  }
  if( opt >= 2 && eventLength > headerLength ){
    printf(" trace:\n");
    for( int i = 0 ; i < trace_length ; i++)printf("%3d|     %-10d\n",i, trace[i]);
  }
}