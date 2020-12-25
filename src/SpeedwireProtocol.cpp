#include <memory.h>
#include <SpeedwireByteEncoding.hpp>
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

const uint16_t SpeedwireProtocol::sma_emeter_protocol_id    = 0x6069;
const uint16_t SpeedwireProtocol::sma_inverter_protocol_id  = 0x6065;
const uint16_t SpeedwireProtocol::sma_discovery_protocol_id = 0xffff;

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
const unsigned long SpeedwireProtocol::sma_long_words_offset = SpeedwireProtocol::sma_protocol_offset + SpeedwireProtocol::sma_protocol_size;
const unsigned long SpeedwireProtocol::sma_long_words_size = 1;
const unsigned long SpeedwireProtocol::sma_control_offset = SpeedwireProtocol::sma_long_words_offset + SpeedwireProtocol::sma_long_words_size;
const unsigned long SpeedwireProtocol::sma_control_size = 1;


SpeedwireProtocol::SpeedwireProtocol(const void *const udp_packet, const unsigned long udp_packet_len) {
    udp = (uint8_t *)udp_packet;
    size = udp_packet_len;
}

SpeedwireProtocol::~SpeedwireProtocol(void) {
    udp = NULL;
    size = 0;
}

// test validity of packet header
bool SpeedwireProtocol::checkHeader(void)  const {

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
uint32_t SpeedwireProtocol::getSignature(void) const {
    return SpeedwireByteEncoding::getUint32BigEndian(udp + sma_signature_offset);
}

// get tag0
uint32_t SpeedwireProtocol::getTag0(void) const {
    return SpeedwireByteEncoding::getUint32BigEndian(udp + sma_tag0_offset);
}

// get group
uint32_t SpeedwireProtocol::getGroup(void) const {
    return SpeedwireByteEncoding::getUint32BigEndian(udp + sma_group_offset);
}

// get packet length - starting to count from the the byte following protocolID, # of long words and control byte
uint16_t SpeedwireProtocol::getLength(void) const {
    return SpeedwireByteEncoding::getUint16BigEndian(udp + sma_length_offset);
}

// get network version
uint16_t SpeedwireProtocol::getNetworkVersion(void) const {
    return SpeedwireByteEncoding::getUint16BigEndian(udp + sma_netversion_offset);
}

// get protocol ID
uint16_t SpeedwireProtocol::getProtocolID(void) const {
    return SpeedwireByteEncoding::getUint16BigEndian(udp + sma_protocol_offset);
}

// get number of long words (1 long word = 4 bytes)
uint8_t SpeedwireProtocol::getLongWords(void) const {
    return *(udp + sma_long_words_offset);
}

// get control byte
uint8_t SpeedwireProtocol::getControl(void) const {
    return *(udp + sma_control_offset);
}

// check if protocolID is emeter
bool SpeedwireProtocol::isEmeterProtocolID(void) const {
    return (getProtocolID() == sma_emeter_protocol_id);
}

// check if protocolID is inverter
bool SpeedwireProtocol::isInverterProtocolID(void) const {
    return (getProtocolID() == sma_inverter_protocol_id);
}


// set header fields according to defaults
void SpeedwireProtocol::setDefaultHeader(void) {
    setDefaultHeader(1, 0, 0);
}
void SpeedwireProtocol::setDefaultHeader(uint32_t group, uint16_t length, uint16_t protocolID) {
    memcpy(udp + sma_signature_offset, sma_signature, sizeof(sma_signature));
    memcpy(udp + sma_tag0_offset,      sma_tag0,      sizeof(sma_tag0));
    setGroup(group);
    setLength(length);
    memcpy(udp + sma_netversion_offset, sma_net_v2, sizeof(sma_net_v2));
    setProtocolID(protocolID);
    setLongWords((uint8_t)(length / sizeof(uint32_t)));
    setControl(0);
}

// set SMA signature
void SpeedwireProtocol::setSignature(uint32_t value) {
    SpeedwireByteEncoding::setUint32BigEndian(udp + sma_signature_offset, value);
}

// set tag0
void SpeedwireProtocol::setTag0(uint32_t value) {
    SpeedwireByteEncoding::setUint32BigEndian(udp + sma_tag0_offset, value);
}

// set group
void SpeedwireProtocol::setGroup(uint32_t value) {
    SpeedwireByteEncoding::setUint32BigEndian(udp + sma_group_offset, value);
}

// set packet length
void SpeedwireProtocol::setLength(uint16_t value) {
    SpeedwireByteEncoding::setUint16BigEndian(udp + sma_length_offset, value);

}

// set network version
void SpeedwireProtocol::setNetworkVersion(uint16_t value) {
    SpeedwireByteEncoding::setUint16BigEndian(udp + sma_netversion_offset, value);
}

// set protocol ID
void SpeedwireProtocol::setProtocolID(uint16_t value) {
    SpeedwireByteEncoding::setUint16BigEndian(udp + sma_protocol_offset, value);
}

// set number of long words (1 long word = 4 bytes)
 void SpeedwireProtocol::setLongWords(uint8_t value) {
    SpeedwireByteEncoding::setUint8(udp + sma_long_words_offset, value);
}

// get control byte
void SpeedwireProtocol::setControl(uint8_t value)  {
    SpeedwireByteEncoding::setUint8(udp + sma_control_offset, value);
}


// get payload offset in udp packet
unsigned long SpeedwireProtocol::getPayloadOffset(void) const {
    if (getProtocolID() == sma_emeter_protocol_id) {    // emeter protocol data payload starts directly after the protocolID field
        return sma_protocol_offset + sma_protocol_size;
    }
    return sma_control_offset + sma_control_size;
}

// get pointer to udp packet
uint8_t* SpeedwireProtocol::getPacketPointer(void) const {
    return udp;
}

// get size of udp packet
unsigned long SpeedwireProtocol::getPacketSize(void) const {
    return size;
}