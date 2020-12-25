#ifndef __SPEEDWIREINVERTER_HPP__
#define __SPEEDWIREINVERTER_HPP__

#include <cstdint>
#include <SpeedwireProtocol.hpp>


class SpeedwireInverter {

protected:

    uint8_t* udp;
    unsigned long size;

public:
    SpeedwireInverter(const void* const udp_packet, const unsigned long udp_packet_size);
    SpeedwireInverter(SpeedwireProtocol &prot);
    ~SpeedwireInverter(void);


};
#endif
