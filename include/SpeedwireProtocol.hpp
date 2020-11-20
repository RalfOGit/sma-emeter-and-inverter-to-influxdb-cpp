#ifndef __SPEEDWIREPROTOCOL_H__
#define __SPEEDWIREPROTOCOL_H__

#include <bits/types.h>   // for __uint8_t


class SpeedwireProtocol {

protected:
    static const __uint8_t  sma_signature[4];
    static const __uint8_t  sma_tag0[4];
    static const __uint8_t  sma_net_v2[2];

    static const unsigned long sma_signature_offset;
    static const unsigned long sma_signature_size;
    static const unsigned long sma_tag0_offset;
    static const unsigned long sma_tag0_size;
    static const unsigned long sma_group_offset;
    static const unsigned long sma_group_size;
    static const unsigned long sma_length_offset;
    static const unsigned long sma_length_size;
    static const unsigned long sma_netversion_offset;
    static const unsigned long sma_netversion_size;
    static const unsigned long sma_protocol_offset;
    static const unsigned long sma_protocol_size;

    __uint8_t *udp;
    unsigned long size;

public:
    SpeedwireProtocol(const void *const udp_packet, const unsigned long udp_packet_size);
    ~SpeedwireProtocol(void);

    bool checkHeader(void);
    __uint32_t getSignature(void);
    __uint16_t getTag0(void);
    __uint32_t getGroup(void);
    __uint16_t getLength(void);
    __uint16_t getNetworkVersion(void);
    __uint16_t getProtocolID(void);
    unsigned long getPayloadOffset(void);

    // methods to get and set field value from and to network byte order
    static __uint16_t getUint16(const void *const udp_ptr);
    static __uint32_t getUint32(const void *const udp_ptr);
    static __uint64_t getUint64(const void *const udp_ptr);
    static void setUint16(void *udp_ptr, const __uint16_t value);
    static void setUint32(void *udp_ptr, const __uint32_t value);
    static void setUint64(void *udp_ptr, const __uint64_t value);
};

#endif
