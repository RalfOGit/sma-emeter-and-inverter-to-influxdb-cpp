#include <LocalHost.hpp>
#include <AddressConversion.hpp>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireReceiveDispatcher.hpp>
#include <SpeedwireCommand.hpp>
#include <ObisData.hpp>
#include <Logger.hpp>
#include <PacketReceiver.hpp>
using namespace libspeedwire;

static Logger emeter_logger   = Logger("EmeterPacketReceiver");
static Logger inverter_logger = Logger("InverterPacketReceiver");


/**
 *  Speedwire packet receiver class for sma emeter packets
 */

 /**
  *  Constructor.
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
        uint32_t time   = emeter_packet.getTime();
        emeter_logger.print(LogLevel::LOG_INFO_1, "received emeter packet from %s susyid %u serial %lu time %lu\n", AddressConversion::toString(src).c_str(), susyid, serial, time);

        // extract obis data from the emeter packet and pass each obis data element to the obis filter
        int32_t signed_power_total = 0, signed_power_l1 = 0, signed_power_l2 = 0, signed_power_l3 = 0;
        for (void* obis = emeter_packet.getFirstObisElement(); obis != NULL; obis = emeter_packet.getNextObisElement(obis)) {
            //emeter.printObisElement(obis, stderr);
            // send the obis value to the obis filter before proceeding with then next obis element
            filter.consume(serial, obis, time);
        }

        // signal end of obis data to the obis filter
        filter.endOfObisData(serial, time);
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
        std::vector<SpeedwireRawData> raw_data_vector = inverter_packet.getRawDataElements();
        //printf("%s\n", inverter_packet.toString().c_str());

        // convert raw data into inverter values and pass them to the data processor
        for (auto &raw_data : raw_data_vector) {
            raw_data.command = (Command)token.command;
            auto iterator = data_map.find(raw_data.toKey());
            if (iterator != data_map.end()) {
                iterator->second.consume(raw_data);
                //iterator->second.print(stdout);
                processor.consume(serial, iterator->second);
            }
        }

        // remove query token
        command.getTokenRepository().remove(token_index);
    }
}
