#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>
#define poll(a, b, c)  WSAPoll((a), (b), (c))
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#endif

#include <LocalHost.hpp>
#include <Measurement.hpp>
#include <SpeedwireSocketFactory.hpp>
#include <SpeedwireSocketSimple.hpp>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireCommand.hpp>
#include <SpeedwireDiscovery.hpp>
#include <ObisFilter.hpp>
#include <DataProcessor.hpp>


static int poll_emeters(const std::vector<SpeedwireSocket>& sockets, struct pollfd* const fds, const int poll_emeter_timeout_in_ms, ObisFilter& filter);
static int query_inverter(const SpeedwireInfo& device, SpeedwireCommand& command, SpeedwireDataMap& query_map, DataProcessor& processor, bool& needs_login, bool& night_mode);


int main(int argc, char **argv) {

    // discover sma devices on the local network
    LocalHost localhost;
    SpeedwireDiscovery discoverer(localhost);
    discoverer.discoverDevices();

    // define measurement filters for sma emeter packet filtering
    ObisFilter filter;
    filter.addFilter(ObisData::PositiveActivePowerTotal);
    filter.addFilter(ObisData::PositiveActivePowerL1);
    filter.addFilter(ObisData::PositiveActivePowerL2);
    filter.addFilter(ObisData::PositiveActivePowerL3);
    //filter.addFilter(ObisData::PositiveActiveEnergyTotal);
    //filter.addFilter(ObisData::PositiveActiveEnergyL1);
    //filter.addFilter(ObisData::PositiveActiveEnergyL2);
    //filter.addFilter(ObisData::PositiveActiveEnergyL3);
    filter.addFilter(ObisData::NegativeActivePowerTotal);
    filter.addFilter(ObisData::NegativeActivePowerL1);
    filter.addFilter(ObisData::NegativeActivePowerL2);
    filter.addFilter(ObisData::NegativeActivePowerL3);
    //filter.addFilter(ObisData::NegativeActiveEnergyTotal);
    //filter.addFilter(ObisData::NegativeActiveEnergyL1);
    //filter.addFilter(ObisData::NegativeActiveEnergyL2); 
    //filter.addFilter(ObisData::NegativeActiveEnergyL3); 
    //filter.addFilter(ObisData::PowerFactorTotal);
    //filter.addFilter(ObisData::PowerFactorL1);
    //filter.addFilter(ObisData::PowerFactorL2);
    //filter.addFilter(ObisData::PowerFactorL3);
    //filter.addFilter(ObisData::CurrentL1);
    //filter.addFilter(ObisData::CurrentL2);
    //filter.addFilter(ObisData::CurrentL3);
    //filter.addFilter(ObisData::VoltageL1);
    //filter.addFilter(ObisData::VoltageL2);
    //filter.addFilter(ObisData::VoltageL3);
    filter.addFilter(ObisData::SignedActivePowerTotal);   // calculated value that is not provided by emeter

    // define measurement elements for sma inverter queries
    SpeedwireDataMap query_map;
    query_map.add(SpeedwireData::InverterPowerMPP1);
    query_map.add(SpeedwireData::InverterPowerMPP2);
    query_map.add(SpeedwireData::InverterVoltageMPP1);
    query_map.add(SpeedwireData::InverterVoltageMPP2);
    query_map.add(SpeedwireData::InverterCurrentMPP1);
    query_map.add(SpeedwireData::InverterCurrentMPP2);
    query_map.add(SpeedwireData::InverterPowerL1);
    query_map.add(SpeedwireData::InverterPowerL2);
    query_map.add(SpeedwireData::InverterPowerL3);
    //query_map.add(SpeedwireData::InverterVoltageL1);
    //query_map.add(SpeedwireData::InverterVoltageL2);
    //query_map.add(SpeedwireData::InverterVoltageL3);
    //query_map.add(SpeedwireData::InverterVoltageL1toL2);
    //query_map.add(SpeedwireData::InverterVoltageL2toL3);
    //query_map.add(SpeedwireData::InverterVoltageL3toL1);
    //query_map.add(SpeedwireData::InverterCurrentL1);
    //query_map.add(SpeedwireData::InverterCurrentL2);
    //query_map.add(SpeedwireData::InverterCurrentL3);
    query_map.add(SpeedwireData::InverterStatus);
    query_map.add(SpeedwireData::InverterRelay);

    // configure processing chain
    DataProcessor processor(60000);
    filter.addConsumer(&processor);
    SpeedwireCommand command(localhost, discoverer.getDevices());

    // open socket(s) to receive sma emeter packets from any local interface
    const std::vector<SpeedwireSocket> sockets = SpeedwireSocketFactory::getInstance(localhost)->getRecvSockets(SpeedwireSocketFactory::MULTICAST, localhost.getLocalIPv4Addresses());

    // allocate pollfd struct(s) for the socket(s)
    struct pollfd *const fds = (struct pollfd *const) malloc(sizeof(struct pollfd) * sockets.size());


    //
    // main loop
    //
    bool needs_login = true;
    bool night_mode  = false;
    uint64_t start_time = localhost.getTickCountInMs();

    while(true) {

        const unsigned long query_inverter_interval_in_ms = (night_mode ? 10*30000 : 30000);
        const unsigned long poll_emeter_timeout_in_ms     = (night_mode ?    10000 :  2000);
        night_mode = false;  // re-enabled later when handling inverter responses

        // login to all inverter devices
        if (needs_login == true) {
            needs_login = false;
            for (auto& device : discoverer.getDevices()) {
                if (device.deviceType == "Inverter") {
                    command.logoff(device);
                    command.login(device, true, "9999");
                }
            }
        }

        // poll sockets for inbound emeter packets
        int npackets = poll_emeters(sockets, fds, poll_emeter_timeout_in_ms, filter);
        if (npackets > 0) {
            processor.flush();
        }

        // if the query interval has elapsed for the inverters, start a query
        uint64_t elapsed_time = localhost.getTickCountInMs() - start_time;
        if (elapsed_time > query_inverter_interval_in_ms) {
            start_time += query_inverter_interval_in_ms;

            for (auto& device : discoverer.getDevices()) {
                if (device.deviceType == "Inverter") {
                    // query inverter for status and energy production data
                    printf("QUERY INVERTER  time 0x%016llx\n", localhost.getTickCountInMs());
                    int nreplies = query_inverter(device, command, query_map, processor, needs_login, night_mode);
                    if (nreplies > 0) {
                        processor.flush();
                    }
                }
            }
        }
    }
    free(fds);

    return 0;
}


/**
 +  poll all configured sockets for emeter udp packets and pass emeter data to the obis filter 
 */
static int poll_emeters(const std::vector<SpeedwireSocket> &sockets, struct pollfd* const fds, const int poll_emeter_timeout_in_ms, ObisFilter &filter) {
    int npackets = 0;
    unsigned char multicast_packet[1024];

    // prepare the pollfd structure
    for (int j = 0; j < sockets.size(); ++j) {
        fds[j].fd      = sockets[j].getSocketFd();
        fds[j].events  = POLLIN;
        fds[j].revents = 0;
    }

    // wait for a packet on the configured socket
    if (poll(fds, sockets.size(), poll_emeter_timeout_in_ms) < 0) {
        perror("poll failure");
        return -1;
    }

    // determine if the socket received a packet
    for (int j = 0; j < sockets.size(); ++j) {
        auto& socket = sockets[j];

        if ((fds[j].revents & POLLIN) != 0) {
            int nbytes = -1;

            // read packet data
            if (socket.isIpv4()) {
                struct sockaddr_in src;
                nbytes = socket.recvfrom(multicast_packet, sizeof(multicast_packet), src);
            }
            else if (socket.isIpv6()) {
                struct sockaddr_in6 src;
                nbytes = socket.recvfrom(multicast_packet, sizeof(multicast_packet), src);
            }
            // check if it is an sma emeter packet
            SpeedwireHeader protocol(multicast_packet, nbytes);
            bool valid = protocol.checkHeader();
            if (valid) {
                uint32_t group = protocol.getGroup();
                uint16_t length = protocol.getLength();
                uint16_t protocolID = protocol.getProtocolID();
                int      offset = protocol.getPayloadOffset();

                if (protocolID == SpeedwireHeader::sma_emeter_protocol_id) {
                    printf("RECEIVED EMETER PACKET  time 0x%016llx\n", LocalHost::getTickCountInMs());
                    SpeedwireEmeterProtocol emeter(multicast_packet + offset, nbytes - offset);
                    uint16_t susyid = emeter.getSusyID();
                    uint32_t serial = emeter.getSerialNumber();
                    uint32_t timer = emeter.getTime();

                    // extract obis data from the emeter packet and pass each obis data element to the obis filter
                    uint8_t obis_signed_power[8] = { 0, 16, 7, 0, 0, 0, 0, 0 };
                    int32_t signed_power = 0;
                    void* obis = emeter.getFirstObisElement();
                    while (obis != NULL) {
                        //emeter.printObisElement(obis, stderr);
                        // ugly hack to calculate the signed power value
                        if (SpeedwireEmeterProtocol::getObisType(obis) == 4) {
                            switch (SpeedwireEmeterProtocol::getObisIndex(obis)) {
                            case 1: signed_power += SpeedwireEmeterProtocol::getObisValue4(obis);  break;
                            case 2: signed_power -= SpeedwireEmeterProtocol::getObisValue4(obis);  break;
                            }
                        }
                        // send the obis value to the obis filter before proceeding with then next obis element
                        filter.consume(obis, timer);
                        obis = emeter.getNextObisElement(obis);
                    }
                    // send the calculated signed power value to the obis filter
                    SpeedwireByteEncoding::setUint32BigEndian(&obis_signed_power[4], signed_power);
                    filter.consume(obis_signed_power, timer);
                    ++npackets;
                }
            }
        }
    }
    return npackets;
}


/**
 * query inverters for status and energy production data
 */
static int query_inverter(const SpeedwireInfo &device, SpeedwireCommand &command, SpeedwireDataMap &query_map, DataProcessor &processor, bool &needs_login, bool &night_mode) {

    // query information from the inverters and collect query results
    std::vector<SpeedwireRawData> reply_data;
    // int32_t return_code_1 = command.query(device, Command::COMMAND_DEVICE_QUERY, 0x00823400, 0x008234FF, reply_data);    // query software version
    // int32_t return_code_2 = command.query(device, Command::COMMAND_DEVICE_QUERY, 0x00821E00, 0x008220FF, reply_data);    // query device type
    int32_t return_code_3 = command.query(device, Command::COMMAND_DC_QUERY, 0x00251E00, 0x00251EFF, reply_data);           // query dc power
    int32_t return_code_4 = command.query(device, Command::COMMAND_DC_QUERY, 0x00451F00, 0x004521FF, reply_data);           // query dc voltage and current
    int32_t return_code_5 = command.query(device, Command::COMMAND_AC_QUERY, 0x00464000, 0x004642FF, reply_data);           // query ac power
    //int32_t return_code_6 = command.query(device, Command::COMMAND_AC_QUERY,     0x00464800, 0x004655FF, reply_data);     // query ac voltage and current
    int32_t return_code_7 = command.query(device, Command::COMMAND_STATUS_QUERY, 0x00214800, 0x002148FF, reply_data);       // query device status
    int32_t return_code_8 = command.query(device, Command::COMMAND_STATUS_QUERY, 0x00416400, 0x004164FF, reply_data);       // query grid relay status
    if ((return_code_8 & 0xffff) == 0x0017) {
        needs_login = true;
    }

    // if query results have been received
    if (reply_data.size() > 0) {

        // interprete query results
        for (auto& reply : reply_data) {
            auto iterator = query_map.find(reply.toKey());
            if (iterator != query_map.end()) {
                iterator->second.consume(reply);
                //it->second.print(stdout);
                processor.consume(iterator->second);
            }
        }

        // derive values for inverter total dc and ac power, efficiency and power loss; these are not explicitly provided in the query data
        SpeedwireData dc_power  (SpeedwireData::InverterPowerDCTotal);    dc_power.measurementValue->value = 0.0;
        SpeedwireData ac_power  (SpeedwireData::InverterPowerACTotal);    ac_power.measurementValue->value = 0.0;
        SpeedwireData loss      (SpeedwireData::InverterPowerLoss);
        SpeedwireData efficiency(SpeedwireData::InverterPowerEfficiency);
        query_map.addValueToTarget(SpeedwireData::InverterPowerMPP1.toKey(), dc_power);
        query_map.addValueToTarget(SpeedwireData::InverterPowerMPP2.toKey(), dc_power);
        query_map.addValueToTarget(SpeedwireData::InverterPowerL1.toKey(), ac_power);
        query_map.addValueToTarget(SpeedwireData::InverterPowerL2.toKey(), ac_power);
        query_map.addValueToTarget(SpeedwireData::InverterPowerL3.toKey(), ac_power);
        loss.measurementValue->value = dc_power.measurementValue->value - ac_power.measurementValue->value;
        loss.measurementValue->timer = dc_power.measurementValue->timer;
        loss.time = dc_power.time;
        efficiency.measurementValue->value = (dc_power.measurementValue->value > 0 ? (ac_power.measurementValue->value / dc_power.measurementValue->value) * 100.0 : 0.0);
        efficiency.measurementValue->timer =  dc_power.measurementValue->timer;
        efficiency.time = dc_power.time;
        if (dc_power.time   != 0) processor.consume(dc_power);
        if (ac_power.time   != 0) processor.consume(ac_power);
        if (loss.time       != 0) processor.consume(loss);
        if (efficiency.time != 0) processor.consume(efficiency);

        // enable / disable night mode (it would be better to use dc_voltage, but it is not easily available here)
        night_mode = (dc_power.time != 0 && dc_power.measurementValue->value == 0);
    }
    return reply_data.size();
}
