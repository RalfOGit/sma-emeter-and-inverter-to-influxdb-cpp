#ifndef __SPEEDWIREEMETER_HPP__
#define __SPEEDWIREEMETER_HPP__

#include <bits/types.h>   // for __uint8_t
#include <stdio.h>


class SpeedwireEmeter {

protected:
    static const unsigned long sma_susy_id_offset;
    static const unsigned long sma_susy_id_size;
    static const unsigned long sma_serial_number_offset;
    static const unsigned long sma_serial_number_size;
    static const unsigned long sma_time_offset;
    static const unsigned long sma_time_size;
    static const __uint8_t sma_firmware_version_channel;

    __uint8_t *udp;
    unsigned long size;

public:
    SpeedwireEmeter(const void *const udp_packet, const unsigned long udp_packet_size);
    ~SpeedwireEmeter(void);

    // accessor methods
    __uint16_t getSusyID(void);
    __uint32_t getSerialNumber(void);
    __uint32_t getTime(void);
    void *const getFirstObisElement(void);
    void *const getNextObisElement(const void *const current_element);

    // methods to get obis information with udp_ptr pointing to the first byte of the given obis field
    static __uint8_t getObisChannel(const void *const current_element);
    static __uint8_t getObisIndex(const void *const current_element);
    static __uint8_t getObisType(const void *const current_element);
    static __uint8_t getObisTariff(const void *const current_element);
    static __uint32_t getObisValue4(const void *const current_element);
    static __uint64_t getObisValue8(const void *const current_element);
    static unsigned long getObisLength(const void *const current_element);
    static void printObisElement(const void *const current_element, FILE *file);
};

#endif