#ifndef DATABLOCK_H
#define DATABLOCK_H

#include <cstdint>
#include <vector>
#include <array>

#include <globals.h>

class dataBlock{

  public:
    dataBlock() { Clear(); }
    ~dataBlock() { }

    uint16_t                 ch;
    uint16_t               slot;
    uint16_t              crate;
    uint16_t       headerLength;  /// headerLength > 4, more data except tarce.
    uint16_t        eventLength;  /// eventLength = headerLength + trace 
    bool                 pileup;
    uint64_t               time;
    bool             cfd_forced;
    bool             cfd_source;
    uint16_t                cfd;
    uint16_t             energy;
    uint16_t       trace_length;
    bool     trace_out_of_range;

    int32_t            trailing;
    int32_t             leading;
    int32_t                 gap;
    int32_t            baseline;
    std::array<int32_t,8> QDCsum;

    uint64_t            eventID;

    //uint16_t  trace[MAX_TRACE_LENGTH];
    std::vector<uint16_t> trace;

    uint32_t address() const { uint32_t temp = crate; return (temp<<16) + (slot<<8) + ch; }

    void Clear();
    void ClearQDC();
    void ClearTrace();
    void Print(int opt = 0);

};

#endif