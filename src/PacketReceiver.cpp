#include <LocalHost.hpp>
#include <AddressConversion.hpp>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireReceiveDispatcher.hpp>
#include <SpeedwireCommand.hpp>
#include <ObisData.hpp>
#include <Logger.hpp>
#include <PacketReceiver.hpp>

static Logger emeter_logger   = Logger("EmeterPacketReceiver");
static Logger inverter_logger = Logger("InverterPacketReceiver");


/**
 *  Speedwire packet receiver class for sma emeter packets
 */

 /**
  *  Constructor, std::vector<SpeedwirePacketSender&> &senders, BounceDetector &bounceDetector, PacketPatcher &packetPatcher);
  */
EmeterPacketReceiver::EmeterPacketReceiver(LocalHost& host, ObisFilter& obis_filter)
    : EmeterPacketReceiverBase(host),
    filter(obis_filter) {
    protocolID = SpeedwireHeader::sma_emeter_protocol_id;
}

/**
 *  Receive method - can be called with arbitrary speedwire packets
 */
void EmeterPacketReceiver::receive(SpeedwireHeader& speedwire_packet, struct sockaddr& src) {
    uint32_t group = speedwire_packet.getGroup();
    uint16_t length = speedwire_packet.getLength();
    uint16_t protocolID = speedwire_packet.getProtocolID();
    int      offset = speedwire_packet.getPayloadOffset();

    // check if it is an emeter packet
    if (speedwire_packet.isEmeterProtocolID() == true) {
        const SpeedwireEmeterProtocol emeter_packet(speedwire_packet);
        uint16_t susyid = emeter_packet.getSusyID();
        uint32_t serial = emeter_packet.getSerialNumber();
        uint32_t timer  = emeter_packet.getTime();
        emeter_logger.print(LogLevel::LOG_INFO_1, "received emeter packet from %s susyid %u serial %lu time %lu\n", AddressConversion::toString(src).c_str(), susyid, serial, timer);

        // extract obis data from the emeter packet and pass each obis data element to the obis filter
        int32_t signed_power_total = 0, signed_power_l1 = 0, signed_power_l2 = 0, signed_power_l3 = 0;
        for (void* obis = emeter_packet.getFirstObisElement(); obis != NULL; obis = emeter_packet.getNextObisElement(obis)) {
            //emeter.printObisElement(obis, stderr);
            // ugly hack to calculate the signed power value
            if (SpeedwireEmeterProtocol::getObisType(obis) == 4) {
                uint32_t value = SpeedwireEmeterProtocol::getObisValue4(obis);
                switch (SpeedwireEmeterProtocol::getObisIndex(obis)) {
                case  1: signed_power_total += value;  break;
                case  2: signed_power_total -= value;  break;
                case 21: signed_power_l1    += value;  break;
                case 22: signed_power_l1    -= value;  break;
                case 41: signed_power_l2    += value;  break;
                case 42: signed_power_l2    -= value;  break;
                case 61: signed_power_l3    += value;  break;
                case 62: signed_power_l3    -= value;  break;
                }
            }
            // send the obis value to the obis filter before proceeding with then next obis element
            filter.consume(serial, obis, timer);
        }
        // send the calculated signed power values to the obis filter
        std::array<uint8_t, 12> obis_signed_power_total = ObisData::SignedActivePowerTotal.toByteArray();
        std::array<uint8_t, 12> obis_signed_power_L1    = ObisData::SignedActivePowerL1.toByteArray();
        std::array<uint8_t, 12> obis_signed_power_L2    = ObisData::SignedActivePowerL2.toByteArray();
        std::array<uint8_t, 12> obis_signed_power_L3    = ObisData::SignedActivePowerL3.toByteArray();
        SpeedwireByteEncoding::setUint32BigEndian(&obis_signed_power_total[4], signed_power_total);
        SpeedwireByteEncoding::setUint32BigEndian(&obis_signed_power_L1[4], signed_power_l1);
        SpeedwireByteEncoding::setUint32BigEndian(&obis_signed_power_L2[4], signed_power_l2);
        SpeedwireByteEncoding::setUint32BigEndian(&obis_signed_power_L3[4], signed_power_l3);
        filter.consume(serial, obis_signed_power_total.data(), timer);
        filter.consume(serial, obis_signed_power_L1.data(), timer);
        filter.consume(serial, obis_signed_power_L2.data(), timer);
        filter.consume(serial, obis_signed_power_L3.data(), timer);
    }
}

// ===================================================================================================

/**
 *  Constructor
 */
InverterPacketReceiver::InverterPacketReceiver(LocalHost& host, SpeedwireCommand& _command, AveragingProcessor& data_processor, SpeedwireDataMap& map)
  : InverterPacketReceiverBase(host),
    command(_command),
    processor(data_processor),
    data_map(map) {
    protocolID = SpeedwireHeader::sma_inverter_protocol_id;
}

/**
 *  Receive method - can be called with arbitrary speedwire packets
 */
void InverterPacketReceiver::receive(SpeedwireHeader& speedwire_packet, struct sockaddr& src) {
    uint32_t group      = speedwire_packet.getGroup();
    uint16_t length     = speedwire_packet.getLength();
    uint16_t protocolID = speedwire_packet.getProtocolID();
    int      offset     = speedwire_packet.getPayloadOffset();

    if (speedwire_packet.isInverterProtocolID() == true) {
        const SpeedwireInverterProtocol inverter_packet(speedwire_packet);
        uint16_t susyid   = inverter_packet.getSrcSusyID();
        uint32_t serial   = inverter_packet.getSrcSerialNumber();
        uint16_t packetid = inverter_packet.getPacketID();
        inverter_logger.print(LogLevel::LOG_INFO_1, "received inverter packet from %s susyid %u serial %lu packetid %u\n", AddressConversion::toString(src).c_str(), susyid, serial, packetid);

        // determine the query token belonging to this response packet
        int token_index = command.getTokenRepository().find(susyid, serial, packetid);
        if (token_index < 0) {
            inverter_logger.print(LogLevel::LOG_ERROR, "cannot find query token => DROPPED\n");
            return;
        }
        const SpeedwireCommandToken& token = command.getTokenRepository().at(token_index);

        // check if the inverter reply packet contains valid data
        bool valid = SpeedwireCommand::checkReply(speedwire_packet, src, token);
        if (valid == false) {
            inverter_logger.print(LogLevel::LOG_ERROR, "invalid reply data => DROPPED\n");
            return;
        }

        // check error code
        uint16_t result = inverter_packet.getErrorCode();
        if (result != 0x0000) {
            if (result == 0x0017) {
                inverter_logger.print(LogLevel::LOG_ERROR, "query error code 0x0017 received - lost connection - not authenticated");
                command.getTokenRepository().needs_login = true;
            }
            else if (token.command == 0xfffd040c) { // login command
                if (result == 0x0100) {
                    inverter_logger.print(LogLevel::LOG_ERROR, "invalid password - not authenticated");
                } else {
                    inverter_logger.print(LogLevel::LOG_ERROR, "login failure - not authenticated");
                }
                command.getTokenRepository().needs_login = true;
            }
            else {
                inverter_logger.print(LogLevel::LOG_ERROR, "query error code received");
            }
            command.getTokenRepository().remove(token_index);
            return;
        }

        // parse reply packet (this path is not taken for login replies)
        const int speedwire_header_length = speedwire_packet.getPayloadOffset();
        const int inverter_header_length  = 8 + 8 + 6 + 4 + 4 + 4;
        if (length > (speedwire_header_length + inverter_header_length + 4 + 8)) {
            uint32_t reply_command  = inverter_packet.getCommandID();
            uint32_t reply_first    = inverter_packet.getFirstRegisterID();
            uint32_t reply_last     = inverter_packet.getLastRegisterID();
            uint32_t payload_length = speedwire_packet.getLength() - inverter_header_length - 4 /*trailer*/;
            uint32_t record_length  = payload_length / (reply_last - reply_first + 1);
            uint32_t record_offset  = 0;
            //printf("=>     command %08lx first 0x%08lx last 0x%08lx rec_len %d\n", reply_command, reply_first, reply_last, record_length);

            // loop across all register ids in the reply packet
            for (uint32_t record = reply_first; record <= reply_last; ++record) {
                if ((record_offset + record_length) > payload_length) {
                    inverter_logger.print(LogLevel::LOG_ERROR, "payload error");
                    return;
                }
                // assemble raw data from reply packet
                SpeedwireRawData raw_data = {
                    (Command)token.command,                                                   // use token command, as at least the low byte will differ
                    (uint32_t)(inverter_packet.getDataUint32(record_offset) & 0x00ffff00),    // register id;
                    (uint8_t)(inverter_packet.getDataUint32(record_offset) & 0x000000ff),     // connector id (mpp #1, mpp #2, ac #1);
                    (uint8_t)(inverter_packet.getDataUint32(record_offset) >> 24),            // unknown type;
                    inverter_packet.getDataUint32(record_offset + 4),                         // time;
                    NULL,
                    (record_length - 8 < sizeof(SpeedwireRawData::data) ? record_length - 8 : sizeof(SpeedwireRawData::data))
                };
                inverter_packet.getDataUint8Array(record_offset + 8, raw_data.data, raw_data.data_size);
                //printf("=>     %s\n", token.toString().c_str());

                // convert raw data into inverter values and pass them to the data processor
                auto iterator = data_map.find(raw_data.toKey());
                if (iterator != data_map.end()) {
                    iterator->second.consume(raw_data);
                    //iterator->second.print(stdout);
                    processor.consume(serial, iterator->second);
                }

                record_offset += record_length;
            }
        }

        // remove query token
        command.getTokenRepository().remove(token_index);
    }
}
