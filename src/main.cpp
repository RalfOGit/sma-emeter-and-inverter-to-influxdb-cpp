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
#include <DataProcessor.hpp>
#include <InfluxDBProducer.hpp>

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
    //discoverer.preRegisterDevice("192.168.182.18");
    discoverer.discoverDevices();

    // define measurement filters for sma emeter packet filtering
    ObisFilter filter;
    filter.addFilter(ObisData::PositiveActivePowerTotal);
    //filter.addFilter(ObisData::PositiveActivePowerL1);
    //filter.addFilter(ObisData::PositiveActivePowerL2);
    //filter.addFilter(ObisData::PositiveActivePowerL3);
    //filter.addFilter(ObisData::PositiveActiveEnergyTotal);
    //filter.addFilter(ObisData::PositiveActiveEnergyL1);
    //filter.addFilter(ObisData::PositiveActiveEnergyL2);
    //filter.addFilter(ObisData::PositiveActiveEnergyL3);
    filter.addFilter(ObisData::NegativeActivePowerTotal);
    //filter.addFilter(ObisData::NegativeActivePowerL1);
    //filter.addFilter(ObisData::NegativeActivePowerL2);
    //filter.addFilter(ObisData::NegativeActivePowerL3);
    //filter.addFilter(ObisData::NegativeActiveEnergyTotal);
    //filter.addFilter(ObisData::NegativeActiveEnergyL1);
    //filter.addFilter(ObisData::NegativeActiveEnergyL2); 
    //filter.addFilter(ObisData::NegativeActiveEnergyL3); 
    filter.addFilter(ObisData::PowerFactorTotal);
    filter.addFilter(ObisData::PowerFactorL1);
    filter.addFilter(ObisData::PowerFactorL2);
    filter.addFilter(ObisData::PowerFactorL3);
    //filter.addFilter(ObisData::CurrentL1);
    //filter.addFilter(ObisData::CurrentL2);
    //filter.addFilter(ObisData::CurrentL3);
    //filter.addFilter(ObisData::VoltageL1);
    //filter.addFilter(ObisData::VoltageL2);
    //filter.addFilter(ObisData::VoltageL3);
    filter.addFilter(ObisData::SignedActivePowerTotal);   // calculated value that is not provided by emeter
    filter.addFilter(ObisData::SignedActivePowerL1);      // calculated value that is not provided by emeter
    filter.addFilter(ObisData::SignedActivePowerL2);      // calculated value that is not provided by emeter
    filter.addFilter(ObisData::SignedActivePowerL3);      // calculated value that is not provided by emeter

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
    InfluxDBProducer producer;
    DataProcessor processor(60000, producer);
    filter.addConsumer(&processor);
    SpeedwireCommand command(localhost, discoverer.getDevices());

    // open socket(s) to receive sma emeter packets from any local interface
    const std::vector<SpeedwireSocket> sockets = SpeedwireSocketFactory::getInstance(localhost)->getRecvSockets(SpeedwireSocketFactory::ANYCAST, localhost.getLocalIPv4Addresses());

    // configure packet dispatcher
    EmeterPacketReceiver   emeter_packet_receiver(localhost, filter);
    InverterPacketReceiver inverter_packet_receiver(localhost, command, processor, query_map);
    SpeedwireReceiveDispatcher dispatcher(localhost);
    dispatcher.registerReceiver(&emeter_packet_receiver);
    dispatcher.registerReceiver(&inverter_packet_receiver);

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
                if (device.deviceType == "Inverter") {
                    logger.print(LogLevel::LOG_INFO_0, "login to inverter - susyid %u serial %lu time 0x%016llx", device.susyID, device.serialNumber, localhost.getUnixEpochTimeInMs());
                    command.logoff(device);
                    command.login(device, true, "9999");
                    int npackets = dispatcher.dispatch(sockets, poll_emeter_timeout_in_ms);
                    start_time = localhost.getTickCountInMs() - query_inverter_interval_in_ms - 1;
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
                if (device.deviceType == "Inverter") {
                    // query inverter for status and energy production data
                    logger.print(LogLevel::LOG_INFO_0, "query inverter  time %lu\n", (uint32_t)localhost.getUnixEpochTimeInMs());

                 // int32_t return_code_1 = command.sendQueryRequest(device, Command::COMMAND_DEVICE_QUERY, 0x00823400, 0x008234FF);    // query software version
                 // int32_t return_code_2 = command.sendQueryRequest(device, Command::COMMAND_DEVICE_QUERY, 0x00821E00, 0x008220FF);    // query device type
                    int32_t return_code_3 = command.sendQueryRequest(device, Command::COMMAND_DC_QUERY,     0x00251E00, 0x00251EFF);    // query dc power
                    int32_t return_code_4 = command.sendQueryRequest(device, Command::COMMAND_DC_QUERY,     0x00451F00, 0x004521FF);    // query dc voltage and current
                    int32_t return_code_5 = command.sendQueryRequest(device, Command::COMMAND_AC_QUERY,     0x00464000, 0x004642FF);    // query ac power
                 // int32_t return_code_6 = command.sendQueryRequest(device, Command::COMMAND_AC_QUERY,     0x00464800, 0x004655FF);    // query ac voltage and current
                    int32_t return_code_7 = command.sendQueryRequest(device, Command::COMMAND_STATUS_QUERY, 0x00214800, 0x002148FF);    // query device status
                    int32_t return_code_8 = command.sendQueryRequest(device, Command::COMMAND_STATUS_QUERY, 0x00416400, 0x004164FF);    // query grid relay status
                    //unsigned char buffer[2048];
                    //int32_t return_code_3 = command.query(device, Command::COMMAND_DC_QUERY,     0x00251E00, 0x00251EFF, buffer, sizeof(buffer));    // query dc power
                    //int32_t return_code_4 = command.query(device, Command::COMMAND_DC_QUERY,     0x00451F00, 0x004521FF, buffer, sizeof(buffer));    // query dc voltage and current
                    //int32_t return_code_5 = command.query(device, Command::COMMAND_AC_QUERY,     0x00464000, 0x004642FF, buffer, sizeof(buffer));    // query ac power
                    //int32_t return_code_7 = command.query(device, Command::COMMAND_STATUS_QUERY, 0x00214800, 0x002148FF, buffer, sizeof(buffer));    // query device status
                    //int32_t return_code_8 = command.query(device, Command::COMMAND_STATUS_QUERY, 0x00416400, 0x004164FF, buffer, sizeof(buffer));    // query grid relay status
                    inverter_query = true;
                }
            }
        }

        // dispatch inbound packets
        int npackets = dispatcher.dispatch(sockets, poll_emeter_timeout_in_ms);
        if (npackets > 0) {
            producer.flush();
        }

        // wait for inverter to respond with all data before deriving aggregated inverter values
        if (inverter_query == true) {
            if (command.getTokenRepository().size() == 0) {
                inverter_query = false;
                logger.print(LogLevel::LOG_INFO_0, "aggregate inverter data time %lu\n", (uint32_t)localhost.getUnixEpochTimeInMs());

                // derive values for inverter total dc and ac power, efficiency and power loss; these are not explicitly provided in the query data
                SpeedwireData dc_power(SpeedwireData::InverterPowerDCTotal);    dc_power.measurementValue->value = 0.0;
                SpeedwireData ac_power(SpeedwireData::InverterPowerACTotal);    ac_power.measurementValue->value = 0.0;
                SpeedwireData loss(SpeedwireData::InverterPowerLoss);
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
                efficiency.measurementValue->timer = dc_power.measurementValue->timer;
                efficiency.time = dc_power.time;
                if (dc_power.time != 0) processor.consume(dc_power);
                if (ac_power.time != 0) processor.consume(ac_power);
                if (loss.time != 0) processor.consume(loss);
                if (efficiency.time != 0) processor.consume(efficiency);
                producer.flush();

                // enable / disable night mode (it would be better to use dc_voltage, but it is not easily available here)
                night_mode = (dc_power.time != 0 && dc_power.measurementValue->value == 0);
            }
        }
    }

    return 0;
}

