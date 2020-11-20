#ifdef _WIN32
    #include <Winsock2.h> // before Windows.h, else Winsock 1 conflict
#else
    #include <netinet/in.h>    // for ntohl(), ...
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SpeedwireProtocol.hpp>


const uint8_t  SpeedwireProtocol::sma_signature[] = {
    0x53, 0x4d, 0x41, 0x00      // "SMA\0"
};

const uint8_t SpeedwireProtocol::sma_tag0[] = {
    0x00, 0x04, 0x02, 0xa0      // length: 0x0004  tag: 0x02a0
};

const uint8_t SpeedwireProtocol::sma_net_v2[] = {
    0x00, 0x10
};

const unsigned long SpeedwireProtocol::sma_signature_offset = 0;
const unsigned long SpeedwireProtocol::sma_signature_size = sizeof(SpeedwireProtocol::sma_signature);
const unsigned long SpeedwireProtocol::sma_tag0_offset = SpeedwireProtocol::sma_signature_size;
const unsigned long SpeedwireProtocol::sma_tag0_size = sizeof(SpeedwireProtocol::sma_tag0);
const unsigned long SpeedwireProtocol::sma_group_offset = SpeedwireProtocol::sma_tag0_offset + SpeedwireProtocol::sma_tag0_size;
const unsigned long SpeedwireProtocol::sma_group_size = 4;
const unsigned long SpeedwireProtocol::sma_length_offset = SpeedwireProtocol::sma_group_offset + SpeedwireProtocol::sma_group_size;
const unsigned long SpeedwireProtocol::sma_length_size = 2;
const unsigned long SpeedwireProtocol::sma_netversion_offset = SpeedwireProtocol::sma_length_offset + SpeedwireProtocol::sma_length_size;
const unsigned long SpeedwireProtocol::sma_netversion_size = sizeof(SpeedwireProtocol::sma_net_v2);
const unsigned long SpeedwireProtocol::sma_protocol_offset = SpeedwireProtocol::sma_netversion_offset + SpeedwireProtocol::sma_netversion_size;
const unsigned long SpeedwireProtocol::sma_protocol_size = 2;


SpeedwireProtocol::SpeedwireProtocol(const void *const udp_packet, const unsigned long udp_packet_len) {
    udp = (uint8_t *)udp_packet;
    size = udp_packet_len;
}

SpeedwireProtocol::~SpeedwireProtocol(void) {
    udp = NULL;
    size = 0;
}

// test validity of packet header
bool SpeedwireProtocol::checkHeader(void) {

    // test if udp packet is large enough to hold the header structure
    if (size < (sma_protocol_offset + sma_protocol_size)) {
        return false;
    }

    // test SMA signature
    if (memcmp(sma_signature, udp + sma_signature_offset, sizeof(sma_signature)) != 0) {
        return false;
    }

    // test SMA tag0
    if (memcmp(sma_tag0, udp + sma_tag0_offset, sizeof(sma_tag0)) != 0) {
        return false;
    }

    // test group field
    //__uint16_t group = getGroup();

    // test length field
    //__uint16_t length = getLength();

    // test SMA net version 2
    if (memcmp(sma_net_v2, udp + sma_netversion_offset, sizeof(sma_net_v2)) != 0) {
        return false;
    }

    return true;
}

// get SMA signature
uint32_t SpeedwireProtocol::getSignature(void) {
    return getUint32(udp + sma_signature_offset);
}

// get group
uint32_t SpeedwireProtocol::getGroup(void) {
    return getUint32(udp + sma_group_offset);
}

// get packet length
uint16_t SpeedwireProtocol::getLength(void) {
    return getUint16(udp + sma_length_offset);
}

// get packet length
uint16_t SpeedwireProtocol::getNetworkVersion(void) {
    return getUint16(udp + sma_netversion_offset);
}

// get protocol ID
uint16_t SpeedwireProtocol::getProtocolID(void) {
    return getUint16(udp + sma_protocol_offset);
}

// get payload offset in udp packet
unsigned long SpeedwireProtocol::getPayloadOffset(void) {
    return sma_protocol_offset + sma_protocol_size;
}


// methods to get and set field value from and to network byte order
uint16_t SpeedwireProtocol::getUint16(const void *const udp_ptr) {
    uint16_t value_in_nbo;
    memcpy(&value_in_nbo, udp_ptr, sizeof(value_in_nbo));
    uint16_t value = ntohs(value_in_nbo);
    return value;
}

uint32_t SpeedwireProtocol::getUint32(const void *const udp_ptr) {
    uint32_t value_in_nbo;
    memcpy(&value_in_nbo, udp_ptr, sizeof(value_in_nbo));
    uint32_t value = ntohl(value_in_nbo);
    return value;
}

uint64_t SpeedwireProtocol::getUint64(const void *const udp_ptr) {
    uint64_t hi_value = getUint32(udp_ptr);
    uint64_t lo_value = getUint32(((uint8_t*)udp_ptr) + sizeof(uint32_t));
    uint64_t value = (hi_value << (sizeof(uint32_t)*8)) | lo_value; 
   return value;
}

void SpeedwireProtocol::setUint16(void *udp_ptr, const uint16_t value) {
    uint16_t value_in_nbo = htons(value);
    memcpy(udp_ptr, &value_in_nbo, sizeof(value_in_nbo));
}

void SpeedwireProtocol::setUint32(void *udp_ptr, const uint32_t value) {
    uint32_t value_in_nbo = htonl(value);
    memcpy(udp_ptr, &value_in_nbo, sizeof(value_in_nbo));
}

void SpeedwireProtocol::setUint64(void *udp_ptr, const uint64_t value) {
    uint64_t hi_value = (value >> (sizeof(uint32_t)*8));
    uint64_t lo_value = (value &  (((uint64_t)1 << (sizeof(uint32_t)*8)) - 1));
    setUint32(udp_ptr, hi_value);
    setUint32(((uint8_t*)udp_ptr) + sizeof(uint32_t), lo_value);
}
