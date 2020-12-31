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
#include <SpeedwireSocketFactory.hpp>
#include <SpeedwireSocketSimple.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireCommand.hpp>
#include <SpeedwireDiscovery.hpp>
#include <Measurement.hpp>
#include <ObisFilter.hpp>
#include <DataProcessor.hpp>


int main(int argc, char **argv) {

    // discover sma devices on the local network
    LocalHost localhost;
    SpeedwireDiscovery discoverer(localhost);
    discoverer.discoverDevices();

    // login to all inverter devices
    SpeedwireCommand command(localhost, discoverer.getDevices());
    for (auto& device : discoverer.getDevices()) {
        if (device.deviceType == "Inverter") {
            command.logoff(device);
        }
    }
    for (auto& device : discoverer.getDevices()) {
        if (device.deviceType == "Inverter") {
            std::vector<SpeedwireData> data;
            command.login(device, true, "9999");
        }
    }

    // define measurement types
    MeasurementType positive_power (Direction::POSITIVE,     Type::ACTIVE, Quantity::POWER,        "W",        10);
    MeasurementType positive_energy(Direction::POSITIVE,     Type::ACTIVE, Quantity::ENERGY,       "kWh", 3600000);
    MeasurementType negative_power (Direction::NEGATIVE,     Type::ACTIVE, Quantity::POWER,        "W",        10);
    MeasurementType negative_energy(Direction::NEGATIVE,     Type::ACTIVE, Quantity::ENERGY,       "kWh", 3600000);
    MeasurementType power_factor   (Direction::POSITIVE,     Type::ACTIVE, Quantity::POWER_FACTOR, "phi",    1000);
    MeasurementType voltage        (Direction::NO_DIRECTION, Type::ACTIVE, Quantity::VOLTAGE,      "V",      1000);
    MeasurementType current        (Direction::POSITIVE,     Type::ACTIVE, Quantity::CURRENT,      "A",      1000);
    MeasurementType status         (Direction::NO_DIRECTION, Type::ACTIVE, Quantity::STATUS,       "-",         1);

    // define measurement filters for sma emeter packet filtering
    ObisFilter filter;
    filter.addFilter(ObisFilterData(0, 1, 4, 0, positive_power, Line::TOTAL));
    filter.addFilter(ObisFilterData(0,21, 4, 0, positive_power, Line::L1));
    filter.addFilter(ObisFilterData(0,41, 4, 0, positive_power, Line::L2));
    filter.addFilter(ObisFilterData(0,61, 4, 0, positive_power, Line::L3));
    filter.addFilter(ObisFilterData(0, 1, 8, 0, positive_energy, Line::TOTAL));
    filter.addFilter(ObisFilterData(0,21, 8, 0, positive_energy, Line::L1));
    filter.addFilter(ObisFilterData(0,41, 8, 0, positive_energy, Line::L2));
    filter.addFilter(ObisFilterData(0,61, 8, 0, positive_energy, Line::L3));
    //filter.addFilter(ObisFilterElement(0, 2, 4, 0, negative_power, Line::TOTAL));
    //filter.addFilter(ObisFilterElement(0,22, 4, 0, negative_power, Line::L1));
    //filter.addFilter(ObisFilterElement(0,42, 4, 0, negative_power, Line::L2));  
    //filter.addFilter(ObisFilterElement(0,62, 4, 0, negative_power, Line::L3));  
    //filter.addFilter(ObisFilterElement(0, 2, 8, 0, negative_energy, Line::TOTAL));
    //filter.addFilter(ObisFilterElement(0,22, 8, 0, negative_energy, Line::L1));
    //filter.addFilter(ObisFilterElement(0,42, 8, 0, negative_energy, Line::L2)); 
    //filter.addFilter(ObisFilterElement(0,62, 8, 0, negative_energy, Line::L3)); 
    //filter.addFilter(ObisFilterElement(0, 13, 4, 0, power_factor, Line::TOTAL));
    //filter.addFilter(ObisFilterElement(0, 33, 4, 0, power_factor, Line::L1));
    //filter.addFilter(ObisFilterElement(0, 53, 4, 0, power_factor, Line::L2));
    //filter.addFilter(ObisFilterElement(0, 73, 4, 0, power_factor, Line::L3));
    //filter.addFilter(ObisFilterElement(0, 31, 4, 0, current, Line::L1));
    //filter.addFilter(ObisFilterElement(0, 51, 4, 0, current, Line::L2));
    //filter.addFilter(ObisFilterElement(0, 71, 4, 0, current, Line::L3));
    //filter.addFilter(ObisFilterElement(0, 32, 4, 0, voltage, Line::L1));
    //filter.addFilter(ObisFilterElement(0, 52, 4, 0, voltage, Line::L2));
    //filter.addFilter(ObisFilterElement(0, 72, 4, 0, voltage, Line::L3));

    // define measurement elements for sma inverter queries
    MeasurementType inverter_power  (Direction::NO_DIRECTION, Type::NO_TYPE, Quantity::POWER,   "W", 1);
    MeasurementType inverter_voltage(Direction::NO_DIRECTION, Type::NO_TYPE, Quantity::VOLTAGE, "V", 100);
    MeasurementType inverter_current(Direction::NO_DIRECTION, Type::NO_TYPE, Quantity::CURRENT, "A", 1000);
    MeasurementType inverter_status (Direction::NO_DIRECTION, Type::NO_TYPE, Quantity::STATUS,  "",  1);
    MeasurementType inverter_relay  (Direction::NO_DIRECTION, Type::NO_TYPE, Quantity::STATUS,  "",  1);

    std::map<uint32_t, SpeedwireFilterData> query_map;
    query_map[0x00251E01] = SpeedwireFilterData(COMMAND_DC_QUERY, 0x00251E00, 0x01, 0x40, 0, NULL, 0, inverter_power,   Line::MPP1);
    query_map[0x00251E02] = SpeedwireFilterData(COMMAND_DC_QUERY, 0x00251E00, 0x02, 0x40, 0, NULL, 0, inverter_power,   Line::MPP2);
    query_map[0x00451F01] = SpeedwireFilterData(COMMAND_DC_QUERY, 0x00451F00, 0x01, 0x40, 0, NULL, 0, inverter_voltage, Line::MPP1);
    query_map[0x00451F02] = SpeedwireFilterData(COMMAND_DC_QUERY, 0x00451F00, 0x02, 0x40, 0, NULL, 0, inverter_voltage, Line::MPP2);
    query_map[0x00452101] = SpeedwireFilterData(COMMAND_DC_QUERY, 0x00452100, 0x01, 0x40, 0, NULL, 0, inverter_current, Line::MPP1);
    query_map[0x00452102] = SpeedwireFilterData(COMMAND_DC_QUERY, 0x00452100, 0x02, 0x40, 0, NULL, 0, inverter_current, Line::MPP2);
    query_map[0x00464001] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00464000, 0x01, 0x40, 0, NULL, 0, inverter_power,   Line::L1);
    query_map[0x00464101] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00464100, 0x01, 0x40, 0, NULL, 0, inverter_power,   Line::L2);
    query_map[0x00464201] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00464200, 0x01, 0x40, 0, NULL, 0, inverter_power,   Line::L3);
    //query_map[0x00464801] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00464800, 0x01, 0x00, 0, NULL, 0, inverter_voltage, Line::L1);    // L1 -> N
    //query_map[0x00464901] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00464900, 0x01, 0x00, 0, NULL, 0, inverter_voltage, Line::L2);    // L2 -> N
    //query_map[0x00464a01] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00464a00, 0x01, 0x00, 0, NULL, 0, inverter_voltage, Line::L3);    // L3 -> N
    //query_map[0x00464b01] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00464b00, 0x01, 0x00, 0, NULL, 0, inverter_voltage, Line::L1);    // L1 -> L2
    //query_map[0x00464c01] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00464c00, 0x01, 0x00, 0, NULL, 0, inverter_voltage, Line::L2);    // L2 -> L3
    //query_map[0x00464d01] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00464d00, 0x01, 0x00, 0, NULL, 0, inverter_voltage, Line::L3);    // L3 -> L1
    //query_map[0x00465301] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00465300, 0x01, 0x40, 0, NULL, 0, inverter_current, Line::L1);
    //query_map[0x00465401] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00465400, 0x01, 0x40, 0, NULL, 0, inverter_current, Line::L2);
    //query_map[0x00465501] = SpeedwireFilterData(COMMAND_AC_QUERY, 0x00465500, 0x01, 0x40, 0, NULL, 0, inverter_current, Line::L3);
    query_map[0x00214801] = SpeedwireFilterData(COMMAND_STATUS_QUERY, 0x00214800, 0x01, 0x08, 0, NULL, 0, inverter_status, Line::DEVICE_OK);
    query_map[0x00416401] = SpeedwireFilterData(COMMAND_STATUS_QUERY, 0x00416400, 0x01, 0x08, 0, NULL, 0, inverter_relay,  Line::RELAY_ON);

    // configure processing chain
    DataProcessor processor(10000);
    filter.addConsumer(&processor);

    // open socket(s) to receive sma emeter packets from any local interface
    std::vector<SpeedwireSocket> sockets = SpeedwireSocketFactory::getInstance(localhost)->getRecvSockets(SpeedwireSocketFactory::MULTICAST, localhost.getLocalIPv4Addresses());
    struct pollfd* fds = (struct pollfd *) malloc(sizeof(struct pollfd) * sockets.size());

    const unsigned long query_inverter_interval_in_ms = 10000;
    const unsigned long poll_timeout_in_ms = 2000;
    unsigned char multicast_packet[1024];

    //
    // main loop
    //
    uint64_t start_time = localhost.getTickCountInMs();
    while(1) {

        // prepare the pollfd structure
        int j = 0;
        for (auto& socket : sockets) {
            fds[j].fd = socket.getSocketFd();
            fds[j].events = POLLIN;
            fds[j++].revents = 0;
        }

        // wait for a packet on configured socket
        if (poll(fds, j/*sizeof(sockets)*/, poll_timeout_in_ms) < 0) {
            perror("poll failure");
            return -1;
        }

        // determine if the socket received a packet
        j = 0;
        for (auto& socket : sockets) {

            if ((fds[j++].revents & POLLIN) != 0) {
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
                    int group = protocol.getGroup();
                    int length = protocol.getLength();
                    int protocolID = protocol.getProtocolID();
                    int offset = protocol.getPayloadOffset();

                    if (protocolID == SpeedwireHeader::sma_emeter_protocol_id) {
                        printf("RECEIVED EMETER PACKET  time 0x%016llx\n", localhost.getTickCountInMs());
                        SpeedwireEmeterProtocol emeter(multicast_packet + offset, nbytes - offset);
                        uint16_t susyid = emeter.getSusyID();
                        uint32_t serial = emeter.getSerialNumber();
                        uint32_t timer = emeter.getTime();

                        // extract obis data from the emeter packet and pass each obis data element to the obis filter
                        void* obis = emeter.getFirstObisElement();
                        while (obis != NULL) {
                            //emeter.printObisElement(obis, stderr);
                            filter.consume(obis, timer);
                            obis = emeter.getNextObisElement(obis);
                        }
                    }
                }
            }
        }

        // if the query interval has elapsed for the inverters, start a query
        uint64_t elapsed_time = localhost.getTickCountInMs() - start_time;
        if (elapsed_time > query_inverter_interval_in_ms) {
            start_time += query_inverter_interval_in_ms;

            for (auto& device : discoverer.getDevices()) {
                if (device.deviceType != "Inverter") continue;
                printf("QUERY INVERTER  time 0x%016llx\n", localhost.getTickCountInMs());

                // query information from the inverters and collect query results
                std::vector<SpeedwireData> reply_data;
                //command.query(device, Command::COMMAND_DEVICE_QUERY, 0x00823400, 0x008234FF, reply_data);     // query software version
                //command.query(device, Command::COMMAND_DEVICE_QUERY, 0x00821E00, 0x008220FF, reply_data);     // query device type
                command.query(device, Command::COMMAND_DC_QUERY,     0x00251E00, 0x00251EFF, reply_data);     // query dc power
                command.query(device, Command::COMMAND_DC_QUERY,     0x00451F00, 0x004521FF, reply_data);     // query dc voltage and current
                command.query(device, Command::COMMAND_AC_QUERY,     0x00464000, 0x004642FF, reply_data);     // query ac power
                //command.query(device, Command::COMMAND_AC_QUERY,     0x00464800, 0x004655FF, reply_data);     // query ac voltage and current
                command.query(device, Command::COMMAND_STATUS_QUERY, 0x00214800, 0x002148FF, reply_data);     // query device status
                command.query(device, Command::COMMAND_STATUS_QUERY, 0x00416400, 0x004164FF, reply_data);     // query grid relay status

                // interprete query results
                for (auto& reply : reply_data) {
                    std::map<uint32_t, SpeedwireFilterData>::iterator it = query_map.find(reply.id | reply.conn);
                    if (it != query_map.end()) {
                        it->second.consume(reply);
                        //it->second.print(stdout);
                        processor.consume(it->second);
                    }
                }
            }
        }
    }

    return 0;
}