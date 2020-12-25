#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireInverter.hpp>


SpeedwireInverter::SpeedwireInverter(const void* const udp_packet, const unsigned long udp_packet_len) {
    udp = (uint8_t*)udp_packet;
    size = udp_packet_len;
}

SpeedwireInverter::SpeedwireInverter(SpeedwireProtocol& prot) {
    udp = prot.getPacketPointer() + prot.getPayloadOffset();
    size = prot.getPacketSize() - prot.getPayloadOffset(); 
}

SpeedwireInverter::~SpeedwireInverter(void) {
    udp = NULL;
    size = 0;
}
