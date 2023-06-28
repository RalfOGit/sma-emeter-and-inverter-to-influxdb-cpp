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
#include <AddressConversion.hpp>
#include <CalculatedValueProcessor.hpp>
#include <Logger.hpp>
#include <Measurement.hpp>
#include <SpeedwireSocketFactory.hpp>
#include <SpeedwireSocketSimple.hpp>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireCommand.hpp>
#include <SpeedwireDiscovery.hpp>
#include <SpeedwireReceiveDispatcher.hpp>
#include <PacketReceiver.hpp>
#include <ObisFilter.hpp>
#include <AveragingProcessor.hpp>
#include <InfluxDBProducer.hpp>
using namespace libspeedwire;

static Logger logger("main");

class LogListener : public ILogListener {
public:
    virtual ~LogListener() {}

    virtual void log_msg(const std::string& msg, const LogLevel& level) {
        fprintf(stdout, "%s", msg.c_str());
    }

    virtual void log_msg_w(const std::wstring& msg, const LogLevel& level) {
        fprintf(stdout, "%ls", msg.c_str());
    }
};


int main(int argc, char **argv) {

    // configure logger and logging levels
    ILogListener* log_listener = new LogListener();
    LogLevel log_level = LogLevel::LOG_ERROR | LogLevel::LOG_WARNING;
    log_level = log_level | LogLevel::LOG_INFO_0;
    log_level = log_level | LogLevel::LOG_INFO_1;
    //log_level = log_level | LogLevel::LOG_INFO_2;
    //log_level = log_level | LogLevel::LOG_INFO_3;
    Logger::setLogListener(log_listener, log_level);

    // discover sma devices on the local network
    LocalHost& localhost = LocalHost::getInstance();
    SpeedwireDiscovery discoverer(localhost);
    discoverer.preRegisterDevice("192.168.182.18");
    discoverer.discoverDevices();
    bool devices_found = false;
    for (auto& device : discoverer.getDevices()) {
        if (device.isFullyRegistered()) {
            devices_found = true;
        }
    }
    if (devices_found == false) {
        logger.print(LogLevel::LOG_ERROR, "no speedwire device found\n");
        return 0;
    }

    // define measurement filters for sma emeter packet filtering
    ObisDataMap emeter_map;
    emeter_map.add(ObisData::PositiveActivePowerTotal);
    //emeter_map.add(ObisData::PositiveActivePowerL1);
    //emeter_map.add(ObisData::PositiveActivePowerL2);
    //emeter_map.add(ObisData::PositiveActivePowerL3);
    //emeter_map.add(ObisData::PositiveActiveEnergyTotal);
    //emeter_map.add(ObisData::PositiveActiveEnergyL1);
    //emeter_map.add(ObisData::PositiveActiveEnergyL2);
    //emeter_map.add(ObisData::PositiveActiveEnergyL3);
    emeter_map.add(ObisData::NegativeActivePowerTotal);
    //emeter_map.add(ObisData::NegativeActivePowerL1);
    //emeter_map.add(ObisData::NegativeActivePowerL2);
    //emeter_map.add(ObisData::NegativeActivePowerL3);
    //emeter_map.add(ObisData::NegativeActiveEnergyTotal);
    //emeter_map.add(ObisData::NegativeActiveEnergyL1);
    //emeter_map.add(ObisData::NegativeActiveEnergyL2); 
    //emeter_map.add(ObisData::NegativeActiveEnergyL3); 
    emeter_map.add(ObisData::PowerFactorTotal);
    emeter_map.add(ObisData::PowerFactorL1);
    emeter_map.add(ObisData::PowerFactorL2);
    emeter_map.add(ObisData::PowerFactorL3);
    //emeter_map.add(ObisData::CurrentL1);
    //emeter_map.add(ObisData::CurrentL2);
    //emeter_map.add(ObisData::CurrentL3);
    //emeter_map.add(ObisData::VoltageL1);
    //emeter_map.add(ObisData::VoltageL2);
    //emeter_map.add(ObisData::VoltageL3);
    emeter_map.add(ObisData::SignedActivePowerTotal);   // calculated value that is not provided by emeter
    emeter_map.add(ObisData::SignedActivePowerL1);      // calculated value that is not provided by emeter
    emeter_map.add(ObisData::SignedActivePowerL2);      // calculated value that is not provided by emeter
    emeter_map.add(ObisData::SignedActivePowerL3);      // calculated value that is not provided by emeter

    // define measurement elements for sma inverter queries
    SpeedwireDataMap inverter_map;
    inverter_map.add(SpeedwireData::InverterPowerMPP1);
    inverter_map.add(SpeedwireData::InverterPowerMPP2);
    inverter_map.add(SpeedwireData::InverterVoltageMPP1);
    inverter_map.add(SpeedwireData::InverterVoltageMPP2);
    inverter_map.add(SpeedwireData::InverterCurrentMPP1);
    inverter_map.add(SpeedwireData::InverterCurrentMPP2);
    inverter_map.add(SpeedwireData::InverterPowerL1);
    inverter_map.add(SpeedwireData::InverterPowerL2);
    inverter_map.add(SpeedwireData::InverterPowerL3);
    //inverter_map.add(SpeedwireData::InverterVoltageL1);
    //inverter_map.add(SpeedwireData::InverterVoltageL2);
    //inverter_map.add(SpeedwireData::InverterVoltageL3);
    //inverter_map.add(SpeedwireData::InverterVoltageL1toL2);
    //inverter_map.add(SpeedwireData::InverterVoltageL2toL3);
    //inverter_map.add(SpeedwireData::InverterVoltageL3toL1);
    //inverter_map.add(SpeedwireData::InverterCurrentL1);
    //inverter_map.add(SpeedwireData::InverterCurrentL2);
    //inverter_map.add(SpeedwireData::InverterCurrentL3);
    inverter_map.add(SpeedwireData::InverterStatus);
    inverter_map.add(SpeedwireData::InverterRelay);

    // configure processing chain
    const unsigned long averagingTimeObisData = 60000;
    const unsigned long averagingTimeSpeedwireData = 0;

    for (auto& entry : emeter_map) {
        entry.second.measurementValues.setMaximumNumberOfElements(averagingTimeObisData / 1000);
    }
    for (auto& entry : inverter_map) {
        entry.second.measurementValues.setMaximumNumberOfElements(1);
    }

    ObisFilter filter;
    filter.addFilter(emeter_map);
    InfluxDBProducer producer(discoverer.getDevices());
    CalculatedValueProcessor calculator(filter.getFilter(), inverter_map, producer);
    AveragingProcessor averager(averagingTimeObisData, averagingTimeSpeedwireData);
    filter.addConsumer(averager);
    averager.addConsumer((ObisConsumer&)calculator);
    averager.addConsumer((SpeedwireConsumer&)calculator);
    SpeedwireCommand command(localhost, discoverer.getDevices());

    // open socket(s) to receive sma emeter packets from any local interface
    const std::vector<SpeedwireSocket> sockets = SpeedwireSocketFactory::getInstance(localhost)->getRecvSockets(SpeedwireSocketFactory::SocketType::ANYCAST, localhost.getLocalIPv4Addresses());

    // configure packet dispatcher
    EmeterPacketReceiver   emeter_packet_receiver(localhost, filter);
    InverterPacketReceiver inverter_packet_receiver(localhost, command, averager, inverter_map);
    SpeedwireReceiveDispatcher dispatcher(localhost);
    dispatcher.registerReceiver(emeter_packet_receiver);
    dispatcher.registerReceiver(inverter_packet_receiver);

    //
    // main loop
    //
    bool night_mode  = false;
    bool inverter_query = false;
    command.getTokenRepository().needs_login = true;
    uint64_t start_time = localhost.getTickCountInMs();

    while(true) {

        const unsigned long query_inverter_interval_in_ms = (night_mode ? 10*30000 : 30000);
        const unsigned long poll_emeter_timeout_in_ms     = (night_mode ?    10000 :  2000);
        night_mode = false;  // re-enabled later when handling inverter responses

        // login to all inverter devices
        if (command.getTokenRepository().needs_login == true) {
            command.getTokenRepository().needs_login = false;
            for (auto& device : discoverer.getDevices()) {
                if (device.deviceClass == "Inverter" || device.deviceClass == "PV-Inverter") {
                    logger.print(LogLevel::LOG_INFO_0, "login to inverter - susyid %u serial %lu time 0x%016llx", device.susyID, device.serialNumber, localhost.getUnixEpochTimeInMs());
                    command.logoff(device);
                    command.login(device, true, "9999");
                    int npackets = dispatcher.dispatch(sockets, poll_emeter_timeout_in_ms);
                    start_time = localhost.getTickCountInMs() - query_inverter_interval_in_ms - 1;
                    //command.queryDeviceType(device);
                }
            }
        }

        // if the query interval has elapsed for the inverters, start a query
        uint64_t elapsed_time = localhost.getTickCountInMs() - start_time;
        if (elapsed_time > query_inverter_interval_in_ms) {
            start_time += query_inverter_interval_in_ms;

            // clear any remaining command tokens, it is unlikely that they will still get answered
            command.getTokenRepository().clear();

            // query all inverter devices
            for (auto& device : discoverer.getDevices()) {
                if (device.deviceClass == "Inverter" || device.deviceClass == "PV-Inverter") {
                    // query inverter for status and energy production data
                    logger.print(LogLevel::LOG_INFO_0, "query inverter  time %lu\n", (uint32_t)localhost.getUnixEpochTimeInMs());

                  //int32_t return_code_1 = command.sendQueryRequest(device, Command::COMMAND_DEVICE_QUERY, 0x00823400, 0x008234FF);    // query software version
                  //int32_t return_code_2 = command.sendQueryRequest(device, Command::COMMAND_DEVICE_QUERY, 0x00821E00, 0x008220FF);    // query device type
                    int32_t return_code_3 = command.sendQueryRequest(device, Command::COMMAND_DC_QUERY,     0x00251E00, 0x00251EFF);    // query dc power
                    int32_t return_code_4 = command.sendQueryRequest(device, Command::COMMAND_DC_QUERY,     0x00451F00, 0x004521FF);    // query dc voltage and current
                    int32_t return_code_5 = command.sendQueryRequest(device, Command::COMMAND_AC_QUERY,     0x00464000, 0x004642FF);    // query ac power
                 // int32_t return_code_6 = command.sendQueryRequest(device, Command::COMMAND_AC_QUERY,     0x00464800, 0x004655FF);    // query ac voltage and current
                    int32_t return_code_7 = command.sendQueryRequest(device, Command::COMMAND_STATUS_QUERY, 0x00214800, 0x002148FF);    // query device status
                    int32_t return_code_8 = command.sendQueryRequest(device, Command::COMMAND_STATUS_QUERY, 0x00416400, 0x004164FF);    // query grid relay status
                    //unsigned char buffer[2048];
                    //int32_t return_code_3 = command.query(device, Command::COMMAND_DC_QUERY,     0x00251E00, 0x00251EFF, buffer, sizeof(buffer));    // query dc power
                    //int32_t return_code_4 = command.query(device, Command::COMMAND_DC_QUERY,     0x00451F00, 0x004521FF, buffer, sizeof(buffer));    // query dc voltage and current
                    //int32_t return_code_5 = command.query(device, Command::COMMAND_AC_QUERY,     0x00263F00, 0x004642FF, buffer, sizeof(buffer));    // query ac power
                    //int32_t return_code_7 = command.query(device, Command::COMMAND_STATUS_QUERY, 0x00214800, 0x002148FF, buffer, sizeof(buffer));    // query device status
                    //int32_t return_code_8 = command.query(device, Command::COMMAND_STATUS_QUERY, 0x00416400, 0x004164FF, buffer, sizeof(buffer));    // query grid relay status
                    inverter_query = true;
                }
            }
        }

        // dispatch inbound packets
        int npackets = dispatcher.dispatch(sockets, poll_emeter_timeout_in_ms);

        // wait for inverter to respond with all data before deriving aggregated inverter values
        if (inverter_query == true) {
            if (command.getTokenRepository().size() == 0) {
                inverter_query = false;
                uint32_t time = (uint32_t)localhost.getUnixEpochTimeInMs();
                logger.print(LogLevel::LOG_INFO_0, "aggregate inverter data time %lu\n", time);

                // enable / disable night mode based on the dc power on mpp1
                auto iterator = inverter_map.find(SpeedwireData::InverterPowerMPP1.toKey());
                if (iterator != inverter_map.end()) {
                    night_mode = (iterator->second.time != 0 && iterator->second.measurementValues.getNewestElement().value == 0);
                } else {
                    night_mode = false;
                }

                for (auto& device : discoverer.getDevices()) {
                    if (device.deviceClass == "Inverter" || device.deviceClass == "PV-Inverter") {
                        averager.endOfSpeedwireData(device.serialNumber, time);
                    }
                }
            }
        }
    }

    return 0;
}

