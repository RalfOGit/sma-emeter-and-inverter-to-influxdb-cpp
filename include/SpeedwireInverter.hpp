#ifndef __SPEEDWIREINVERTER_HPP__
#define __SPEEDWIREINVERTER_HPP__

#include <cstdint>
#include <SpeedwireProtocol.hpp>


class SpeedwireInverter {

protected:
    static const unsigned long sma_dst_susy_id_offset;
    static const unsigned long sma_dst_serial_number_offset;
    static const unsigned long sma_dst_control_offset;
    static const unsigned long sma_src_susy_id_offset;
    static const unsigned long sma_src_serial_number_offset;
    static const unsigned long sma_src_control_offset;
    static const unsigned long sma_error_code_offset;
    static const unsigned long sma_fragment_id_offset;
    static const unsigned long sma_packet_id_offset;
    static const unsigned long sma_command_id_offset;
    static const unsigned long sma_first_register_id_offset;
    static const unsigned long sma_last_register_id_offset;
    static const unsigned long sma_data_offset;


    uint8_t* udp;
    unsigned long size;

public:
    SpeedwireInverter(const void* const udp_packet, const unsigned long udp_packet_size);
    SpeedwireInverter(SpeedwireProtocol &prot);
    ~SpeedwireInverter(void);

    // accessor methods
    uint16_t getDstSusyID(void);
    uint32_t getDstSerialNumber(void);
    uint16_t getDstControl(void);
    uint16_t getSrcSusyID(void);
    uint32_t getSrcSerialNumber(void);
    uint16_t getSrcControl(void);
    uint16_t getErrorCode(void);
    uint16_t getFragmentID(void);
    uint16_t getPacketID(void);
    uint32_t getCommandID(void);
    uint32_t getFirstRegisterID(void);
    uint32_t getLastRegisterID(void);
    uint32_t getDataUint32(unsigned long byte_offset);   // offset 0 is the first byte after last register index
    uint64_t getDataUint64(unsigned long byte_offset);

    // setter methods
    void setDstSusyID(uint16_t value);
    void setDstSerialNumber(uint32_t value);
    void setDstControl(uint16_t value);
    void setSrcSusyID(uint16_t value);
    void setSrcSerialNumber(uint32_t value);
    void setSrcControl(uint16_t value);
    void setErrorCode(uint16_t value);
    void setFragmentID(uint16_t value);
    void setPacketID(uint16_t value);
    void setCommandID(uint32_t value);
    void setFirstRegisterID(uint32_t value);
    void setLastRegisterID(uint32_t value);
    void setDataUint32(unsigned long byte_offset, uint32_t value);   // offset 0 is the first byte after last register index
    void setDataUint64(unsigned long byte_offset, uint64_t value);
    void setTrailer(unsigned long offset);

};


#endif
