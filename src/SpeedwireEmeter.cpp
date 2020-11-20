
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SpeedwireProtocol.hpp>
#include <SpeedwireEmeter.hpp>


const unsigned long SpeedwireEmeter::sma_susy_id_offset = 0;
const unsigned long SpeedwireEmeter::sma_susy_id_size   = 2;
const unsigned long SpeedwireEmeter::sma_serial_number_offset = SpeedwireEmeter::sma_susy_id_size;
const unsigned long SpeedwireEmeter::sma_serial_number_size   = 4;
const unsigned long SpeedwireEmeter::sma_time_offset = SpeedwireEmeter::sma_serial_number_offset + SpeedwireEmeter::sma_serial_number_size;
const unsigned long SpeedwireEmeter::sma_time_size   = 4;
const __uint8_t SpeedwireEmeter::sma_firmware_version_channel = 144;


SpeedwireEmeter::SpeedwireEmeter(const void *const udp_packet, const unsigned long udp_packet_len) {
    udp = (__uint8_t *)udp_packet;
    size = udp_packet_len;
}

SpeedwireEmeter::~SpeedwireEmeter(void) {
    udp = NULL;
    size = 0;
}

// get susy id
__uint16_t SpeedwireEmeter::getSusyID(void) {
    return SpeedwireProtocol::getUint16(udp + sma_susy_id_offset);
}

// get serial number
__uint32_t SpeedwireEmeter::getSerialNumber(void) {
    return SpeedwireProtocol::getUint32(udp + sma_serial_number_offset);
}

// get ticker
__uint32_t SpeedwireEmeter::getTime(void) {
    return SpeedwireProtocol::getUint32(udp + sma_time_offset);
}

// get pointer to first obis element in udp packet
void *const SpeedwireEmeter::getFirstObisElement(void) {
    __uint8_t *const first_element = ((__uint8_t *const)udp) + sma_time_offset + sma_time_size;
    if ((first_element - udp) > size) {
        return NULL;
    }
    return first_element;
}

// get pointer to next obis element starting from the given element
void *const SpeedwireEmeter::getNextObisElement(const void *const current_element) {
    __uint8_t *const next_element = ((__uint8_t *const)current_element) + getObisLength(current_element);
    // check if the next element including the 4-byte obis head is inside the udp packet
    if ((next_element + 4 - udp) > size) {
        return NULL;
    }
    // check if the entire next element is inside the udp packet
    if ((next_element + getObisLength(next_element) - udp) > size ) {
        return NULL;
    }
    return next_element;
}


// methods to get obis information with udp_ptr pointing to the first byte of the obis field
__uint8_t SpeedwireEmeter::getObisChannel(const void *const current_element) {
    return ((__uint8_t*)current_element)[0];
}

__uint8_t SpeedwireEmeter::getObisIndex(const void *const current_element) {
    return ((__uint8_t*)current_element)[1];
}

__uint8_t SpeedwireEmeter::getObisType(const void *const current_element) {
    return ((__uint8_t*)current_element)[2];
}

__uint8_t SpeedwireEmeter::getObisTariff(const void *const current_element) {
    return ((__uint8_t*)current_element)[3];
}

__uint32_t SpeedwireEmeter::getObisValue4(const void *const current_element) {
    return SpeedwireProtocol::getUint32(((__uint8_t*)current_element)+4);
}

__uint64_t SpeedwireEmeter::getObisValue8(const void *const current_element) {
    return SpeedwireProtocol::getUint64(((__uint8_t*)current_element)+4);
}

unsigned long SpeedwireEmeter::getObisLength(const void *const current_element) {
    unsigned long type = getObisType(current_element);
    if (getObisChannel(current_element) == sma_firmware_version_channel) {       // the software version has a type of 0, although it has a 4 byte payload
        return 8;
    }
    return 4 + type;
}

void SpeedwireEmeter::printObisElement(const void *const current_element, FILE *file) {
    __uint8_t type = getObisType(current_element);
    fprintf(file, "%d.%d.%d.%d ", getObisChannel(current_element), getObisIndex(current_element), type, getObisTariff(current_element));
    if (type == 4) {
        fprintf(file, "0x%08lx %lu\n", getObisValue4(current_element), getObisValue4(current_element));
    }
    else if (type == 8) {
        fprintf(file, "0x%016llx %llu\n", getObisValue8(current_element), getObisValue8(current_element));
    }
    else if (type == 0 && getObisChannel(current_element) == sma_firmware_version_channel) {
        __uint32_t version = getObisValue4(current_element);
        __uint8_t array[sizeof(__uint32_t)];
        memcpy(array, &version, sizeof(array));
        fprintf(file, "%u.%u.%u.%u\n", array[3], array[2], array[1], array[0]);
    }
    else {
        fprintf(file, "\n");
    }
}
