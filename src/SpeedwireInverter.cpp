#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireInverter.hpp>


const unsigned long SpeedwireInverter::sma_dst_susy_id_offset       = 0;
const unsigned long SpeedwireInverter::sma_dst_serial_number_offset = SpeedwireInverter::sma_dst_susy_id_offset + 2;
const unsigned long SpeedwireInverter::sma_dst_control_offset       = SpeedwireInverter::sma_dst_serial_number_offset + 4;
const unsigned long SpeedwireInverter::sma_src_susy_id_offset       = SpeedwireInverter::sma_dst_control_offset + 2;
const unsigned long SpeedwireInverter::sma_src_serial_number_offset = SpeedwireInverter::sma_src_susy_id_offset + 2;
const unsigned long SpeedwireInverter::sma_src_control_offset       = SpeedwireInverter::sma_src_serial_number_offset + 4;
const unsigned long SpeedwireInverter::sma_error_code_offset        = SpeedwireInverter::sma_src_control_offset + 2;
const unsigned long SpeedwireInverter::sma_fragment_id_offset       = SpeedwireInverter::sma_error_code_offset + 2;
const unsigned long SpeedwireInverter::sma_packet_id_offset         = SpeedwireInverter::sma_fragment_id_offset + 2;
const unsigned long SpeedwireInverter::sma_command_id_offset        = SpeedwireInverter::sma_packet_id_offset + 2;
const unsigned long SpeedwireInverter::sma_first_register_id_offset = SpeedwireInverter::sma_command_id_offset + 4;
const unsigned long SpeedwireInverter::sma_last_register_id_offset  = SpeedwireInverter::sma_first_register_id_offset + 4;
const unsigned long SpeedwireInverter::sma_data_offset              = SpeedwireInverter::sma_last_register_id_offset + 4;


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

// get destination susy id
uint16_t SpeedwireInverter::getDstSusyID(void) {
    return SpeedwireByteEncoding::getUint16LittleEndian(udp + sma_dst_susy_id_offset);
}

// get destination serial number
uint32_t SpeedwireInverter::getDstSerialNumber(void) {
    return SpeedwireByteEncoding::getUint32LittleEndian(udp + sma_dst_serial_number_offset);
}

// get destination control word
uint16_t SpeedwireInverter::getDstControl(void) {
    return SpeedwireByteEncoding::getUint16LittleEndian(udp + sma_dst_control_offset);
}

// get source susy id
uint16_t SpeedwireInverter::getSrcSusyID(void) {
    return SpeedwireByteEncoding::getUint16LittleEndian(udp + sma_src_susy_id_offset);
}

// get source serial number
uint32_t SpeedwireInverter::getSrcSerialNumber(void) {
    return SpeedwireByteEncoding::getUint32LittleEndian(udp + sma_src_serial_number_offset);
}

// get source control word
uint16_t SpeedwireInverter::getSrcControl(void) {
    return SpeedwireByteEncoding::getUint16LittleEndian(udp + sma_src_control_offset);
}

// get error code
uint16_t SpeedwireInverter::getErrorCode(void) {
    return SpeedwireByteEncoding::getUint16LittleEndian(udp + sma_error_code_offset);
}

// get fragment id
uint16_t SpeedwireInverter::getFragmentID(void) {
    return SpeedwireByteEncoding::getUint16LittleEndian(udp + sma_fragment_id_offset);
}

// get packet id
uint16_t SpeedwireInverter::getPacketID(void) {
    return SpeedwireByteEncoding::getUint16LittleEndian(udp + sma_packet_id_offset);
}

// get command id0
uint32_t SpeedwireInverter::getCommandID(void) {
    return SpeedwireByteEncoding::getUint32LittleEndian(udp + sma_command_id_offset);
}

// get first register id
uint32_t SpeedwireInverter::getFirstRegisterID(void) {
    return SpeedwireByteEncoding::getUint32LittleEndian(udp + sma_first_register_id_offset);
}

// get last register id
uint32_t SpeedwireInverter::getLastRegisterID(void) {
    return SpeedwireByteEncoding::getUint32LittleEndian(udp + sma_last_register_id_offset);
}

// get 32-bit of data 
uint32_t SpeedwireInverter::getDataUint32(unsigned long byte_offset) {   // offset 0 is the first byte after last register index
    return SpeedwireByteEncoding::getUint32LittleEndian(udp + sma_data_offset + byte_offset);
}

// get 64-bit of data
uint64_t SpeedwireInverter::getDataUint64(unsigned long byte_offset) {
    return SpeedwireByteEncoding::getUint64LittleEndian(udp + sma_data_offset + byte_offset);
}


// set destination susy id
void SpeedwireInverter::setDstSusyID(uint16_t value) {
    SpeedwireByteEncoding::setUint16LittleEndian(udp + sma_dst_susy_id_offset, value);
}

// set destination serial number
void SpeedwireInverter::setDstSerialNumber(uint32_t value) {
    SpeedwireByteEncoding::setUint32LittleEndian(udp + sma_dst_serial_number_offset, value);
}

// set destination control word
void SpeedwireInverter::setDstControl(uint16_t value) {
    SpeedwireByteEncoding::setUint16LittleEndian(udp + sma_dst_control_offset, value);
}

// set source susy id
void SpeedwireInverter::setSrcSusyID(uint16_t value) {
    SpeedwireByteEncoding::setUint16LittleEndian(udp + sma_src_susy_id_offset, value);
}

// set source serial number
void SpeedwireInverter::setSrcSerialNumber(uint32_t value) {
    SpeedwireByteEncoding::setUint32LittleEndian(udp + sma_src_serial_number_offset, value);
}

// set source control word
void SpeedwireInverter::setSrcControl(uint16_t value) {
    SpeedwireByteEncoding::setUint16LittleEndian(udp + sma_src_control_offset, value);
}

// set error code
void SpeedwireInverter::setErrorCode(uint16_t value) {
    SpeedwireByteEncoding::setUint16LittleEndian(udp + sma_error_code_offset, value);
}

// set fragment id
void SpeedwireInverter::setFragmentID(uint16_t value) {
    SpeedwireByteEncoding::setUint16LittleEndian(udp + sma_fragment_id_offset, value);
}

// set packet id
void SpeedwireInverter::setPacketID(uint16_t value) {
    SpeedwireByteEncoding::setUint16LittleEndian(udp + sma_packet_id_offset, value);
}

// set command id
void SpeedwireInverter::setCommandID(uint32_t value) {
    SpeedwireByteEncoding::setUint32LittleEndian(udp + sma_command_id_offset, value);
}

// set first register id
void SpeedwireInverter::setFirstRegisterID(uint32_t value) {
    SpeedwireByteEncoding::setUint32LittleEndian(udp + sma_first_register_id_offset, value);
}

// set last register id
void SpeedwireInverter::setLastRegisterID(uint32_t value) {
    SpeedwireByteEncoding::setUint32LittleEndian(udp + sma_last_register_id_offset, value);
}

// set 32-bit of data
void SpeedwireInverter::setDataUint32(unsigned long byte_offset, uint32_t value) {  // offset 0 is the first byte after last register index
    SpeedwireByteEncoding::setUint32LittleEndian(udp + sma_data_offset + byte_offset, value);
}

// set 64-bit of data
void SpeedwireInverter::setDataUint64(unsigned long byte_offset, uint64_t value) {  // offset 0 is the first byte after last register index
    SpeedwireByteEncoding::setUint64LittleEndian(udp + sma_data_offset + byte_offset, value);
}

// set trailer long word at data offset
void SpeedwireInverter::setTrailer(unsigned long offset) {
    setDataUint32(offset, 0x00000000);
}
