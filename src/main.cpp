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
#include <HttpClient.hpp>
#include <Logger.hpp>
#include <Measurement.hpp>
#include <SpeedwireSocketFactory.hpp>
#include <SpeedwireSocketSimple.hpp>
#include <SpeedwireAuthentication.hpp>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireDevice.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireEmeterProtocol.hpp>
#include <SpeedwireCommand.hpp>
#include <SpeedwireDiscovery.hpp>
#include <SpeedwireReceiveDispatcher.hpp>
#include <PacketReceiver.hpp>
#include <ObisFilter.hpp>
#include <AveragingProcessor.hpp>
#include <InfluxDBProducer.hpp>
#include <TagDefinitions.hpp>
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


int main(int argc, char** argv) {

    // configure logger and logging levels
    ILogListener* log_listener = new LogListener();
    LogLevel log_level = LogLevel::LOG_ERROR | LogLevel::LOG_WARNING;
    log_level = log_level | LogLevel::LOG_INFO_0;
    log_level = log_level | LogLevel::LOG_INFO_1;
    //log_level = log_level | LogLevel::LOG_INFO_2;
    //log_level = log_level | LogLevel::LOG_INFO_3;
    Logger::setLogListener(log_listener, log_level);

    // read sbfspot tag list data from file (if the input file is available)
    TagDefinitions tagdefs;
    if (tagdefs.readFromFile("./TagListEN-US.txt") > 0) {
        TagDefinitions::insertIntoGlobalSpeedwireStatusMap(tagdefs.getTagMap());
        TagDefinitions::insertIntoGlobalSpeedwireDataMap(tagdefs.getTagMap());
    }

    // read credentials from file (if the input file is available)
    CredentialsMap credentials_map;
    credentials_map.readFromFile("./Credentials.txt");
    credentials_map.setDefaultUserName(UserName::INSTALLER);
    Credentials credentials = credentials_map.getDefaultCredentials();

    // register all required speedwire devices; this needs to be adapted for each site!!!
    LocalHost& localhost = LocalHost::getInstance();
    SpeedwireDiscovery discoverer(localhost);
    discoverer.preRegisterDevice("192.168.182.18");     // pv inverter
    discoverer.preRegisterDevice("192.168.178.22");     // battery inverter
    discoverer.requireDevice(1901431377);   // emeter at grid connection point
    discoverer.requireDevice(3010538116);   // pv inverter
    discoverer.requireDevice(1901026885);   // battery inverter

    // discover sma devices on the local network
    logger.print(LogLevel::LOG_INFO_0, "starting device discovery ...\n");
    int num_devices = discoverer.discoverDevices(false);    // set to false, skip full-subnet scan
    logger.print(LogLevel::LOG_INFO_0, "... finished device discovery; %lu devices found\n", discoverer.getNumberOfFullyRegisteredDevices());

    // check if there are still required but undiscovered devices; if so go for another discovery round
    if (discoverer.getNumberOfMissingDevices() > 0) {
        logger.print(LogLevel::LOG_INFO_0, "still missing %lu devices\n", discoverer.getNumberOfMissingDevices());
        std::vector<SpeedwireDevice> devices = discoverer.getDevices();
        for (const auto& device : devices) {
            // try to wakeup any device having a pre.registered ip address with an http get request, ping would also do
            if (device.hasIPAddressOnly()) {
                libralfogit::HttpClient http_client;
                std::string response;
                std::string content;
                std::string url = "http://" + device.deviceIpAddress + "/";
                logger.print(LogLevel::LOG_INFO_0, "send http get request to device %s ...\n", device.deviceIpAddress.c_str());
                int response_code = http_client.sendHttpGetRequest(url, response, content);
                logger.print(LogLevel::LOG_INFO_0, "... received response code %d\n", response_code);
            }
        }
        logger.print(LogLevel::LOG_INFO_0, "starting device discovery #2 ...\n");
        num_devices = discoverer.discoverDevices(true);    // set to true, perform full-subnet scan
        logger.print(LogLevel::LOG_INFO_0, "... finished device discovery #2\n");
    }
    if (num_devices == 0) {
        logger.print(LogLevel::LOG_WARNING, "... no speedwire device found\n");
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
    inverter_map.add(SpeedwireData::InverterOperationStatus);
    inverter_map.add(SpeedwireData::InverterRelay);
    inverter_map.add(SpeedwireData::BatteryStateOfCharge);
    inverter_map.add(SpeedwireData::BatteryTemperature);
    inverter_map.add(SpeedwireData::BatteryPowerACTotal);
    //inverter_map.add(SpeedwireData::BatteryPowerL1);
    //inverter_map.add(SpeedwireData::BatteryPowerL2);
    //inverter_map.add(SpeedwireData::BatteryPowerL3);
    inverter_map.add(SpeedwireData::BatteryOperationStatus);
    inverter_map.add(SpeedwireData::BatteryRelay);

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
    InfluxDBProducer producer;
    CalculatedValueProcessor calculator(filter.getFilter(), inverter_map, producer);
    AveragingProcessor averager(averagingTimeObisData, averagingTimeSpeedwireData);
    filter.addConsumer(averager);
    averager.addConsumer((ObisConsumer&)calculator);
    averager.addConsumer((SpeedwireConsumer&)calculator);
    SpeedwireCommand command(localhost, discoverer.getDevices());

    // open socket(s) to receive sma emeter packets from any local interface
    const std::vector<SpeedwireSocket> sockets = SpeedwireSocketFactory::getInstance(localhost)->getRecvSockets(SpeedwireSocketFactory::SocketType::ANYCAST, localhost.getLocalIPv4Addresses());

    // configure packet dispatcher
    EmeterPacketReceiver   emeter_packet_receiver(localhost, discoverer.getDevices(), filter);
    InverterPacketReceiver inverter_packet_receiver(localhost, discoverer.getDevices(), command, averager, inverter_map);
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
            SpeedwireAuthentication authenticator(localhost, discoverer.getDevices());
            authenticator.logoff();
            Credentials entry = credentials_map.getDefaultCredentials();
            authenticator.login(entry, 1000);
            int npackets = dispatcher.dispatch(sockets, poll_emeter_timeout_in_ms);
            start_time = localhost.getTickCountInMs() - query_inverter_interval_in_ms - 1;
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
#if 1
                  //int32_t return_code_1 = command.sendQueryRequest(device, Command::DEVICE_QUERY, 0x00823400, 0x008234FF);    // query software version
                  //int32_t return_code_2 = command.sendQueryRequest(device, Command::DEVICE_QUERY, 0x00821E00, 0x008220FF);    // query device type
                    int32_t return_code_3 = command.sendQueryRequest(device, Command::DC_QUERY,     0x00251E00, 0x00251EFF);    // query dc power
                    int32_t return_code_4 = command.sendQueryRequest(device, Command::DC_QUERY,     0x00451F00, 0x004521FF);    // query dc voltage and current
                    int32_t return_code_5 = command.sendQueryRequest(device, Command::AC_QUERY,     0x00464000, 0x004642FF);    // query ac power
                 // int32_t return_code_6 = command.sendQueryRequest(device, Command::AC_QUERY,     0x00464800, 0x004655FF);    // query ac voltage and current
                    int32_t return_code_7 = command.sendQueryRequest(device, Command::STATUS_QUERY, 0x00214800, 0x002148FF);    // query device status
                    int32_t return_code_8 = command.sendQueryRequest(device, Command::STATUS_QUERY, 0x00416400, 0x004164FF);    // query grid relay status
                    //unsigned char buffer[2048];
                    //int32_t return_code_3 = command.query(device, Command::DC_QUERY,     0x00251E00, 0x00251EFF, buffer, sizeof(buffer));    // query dc power
                    //int32_t return_code_4 = command.query(device, Command::DC_QUERY,     0x00451F00, 0x004521FF, buffer, sizeof(buffer));    // query dc voltage and current
                    //int32_t return_code_5 = command.query(device, Command::AC_QUERY,     0x00263F00, 0x004642FF, buffer, sizeof(buffer));    // query ac power
                    //int32_t return_code_7 = command.query(device, Command::STATUS_QUERY, 0x00214800, 0x002148FF, buffer, sizeof(buffer));    // query device status
                    //int32_t return_code_8 = command.query(device, Command::STATUS_QUERY, 0x00416400, 0x004164FF, buffer, sizeof(buffer));    // query grid relay status
#else
                    // query all available data
                    int32_t return_code_1 = command.sendQueryRequest(device, Command::DEVICE_QUERY,      0x00000000, 0xffffffff);    // query device information
                    int32_t return_code_2 = command.sendQueryRequest(device, Command::DEVICE_QUERY | Command::COMPONENT_1, 0x00000000, 0xffffffff);    // query device status (installer or bms?)
                    int32_t return_code_3 = command.sendQueryRequest(device, Command::ENERGY_QUERY,      0x00000000, 0xffffffff);    // query energy production
                    int32_t return_code_4 = command.sendQueryRequest(device, Command::TEMPERATURE_QUERY, 0x00000000, 0xffffffff);    // inverter temperature
                    int32_t return_code_5 = command.sendQueryRequest(device, Command::DC_QUERY,          0x00000000, 0xffffffff);    // query dc power
                    int32_t return_code_6 = command.sendQueryRequest(device, Command::AC_QUERY,          0x00000000, 0xffffffff);    // query ac voltage and current
                    int32_t return_code_7 = command.sendQueryRequest(device, Command::STATUS_QUERY,      0x00000000, 0xffffffff);    // query device status
#endif
                    inverter_query = true;
                }
                else if (device.deviceClass == "Battery-Inverter") {
#if 1
                    //int32_t return_code_1 = command.sendQueryRequest(device, Command::DEVICE_QUERY, 0x00823400, 0x008234FF);  // query software version
                    //int32_t return_code_2 = command.sendQueryRequest(device, Command::DEVICE_QUERY, 0x00821E00, 0x008220FF);  // query device type
                    //int32_t return_code_3 = command.sendQueryRequest(device, Command::ENERGY_QUERY, 0x00260100, 0x004657FF);  // query energy production
                    int32_t return_code_4 = command.sendQueryRequest(device, Command::STATUS_QUERY,                        0x00214800, 0x004164FF);  // query device status
                    int32_t return_code_5 = command.sendQueryRequest(device, Command::STATUS_QUERY,                        0x00416400, 0x004164FF);  // query grid relay status
                    int32_t return_code_6 = command.sendQueryRequest(device, Command::STATUS_QUERY | Command::COMPONENT_2, 0x00412a00, 0x00498aff);  // query battery status
                    int32_t return_code_7 = command.sendQueryRequest(device, Command::AC_QUERY,                            0x00263F00, 0x00495dff);  // query battery and grid measurements
                    //int32_t return_code_8 = command.sendQueryRequest(device, Command::DEVICE_QUERY | Command::COMPONENT_2, 0x00893600, 0x008973ff);  // query battery device settings
                    //int32_t return_code_9 = command.sendQueryRequest(device, Command::DEVICE_QUERY | Command::COMPONENT_2, 0x00921a00, 0x00950aff);  // query battery device settings
#elif 0
                    for (uint32_t i = 0; i <= 2; ++i) {
                        uint32_t offset = i << 16;  // device information is encoded in bit 16
                        if (i == 1) continue;       // this is all conn == 0x00
                        int32_t return_code_1 = command.sendQueryRequest(device, (Command)((uint32_t)Command::DEVICE_QUERY | offset), 0x00000000, 0xffffffff);    // query device information
                        int32_t return_code_2 = command.sendQueryRequest(device, (Command)((uint32_t)Command::ENERGY_QUERY | offset), 0x00000000, 0xffffffff);    // query energy production
                        int32_t return_code_3 = command.sendQueryRequest(device, (Command)((uint32_t)Command::AC_QUERY     | offset), 0x00000000, 0xffffffff);    // query battery and grid measurements
                        int32_t return_code_4 = command.sendQueryRequest(device, (Command)((uint32_t)Command::STATUS_QUERY | offset), 0x00000000, 0xffffffff);    // query device status
                    }
#else
                    // query all available data; DC_QUERY is not supported by SBS2.5
                    int32_t return_code_1 = command.sendQueryRequest(device, Command::DEVICE_QUERY,                        0x00000000, 0xffffffff);    // query device information
                    int32_t return_code_6 = command.sendQueryRequest(device, Command::DEVICE_QUERY | Command::COMPONENT_2, 0x00000000, 0xffffffff);    // query bms device
                    int32_t return_code_2 = command.sendQueryRequest(device, Command::ENERGY_QUERY,                        0x00000000, 0xffffffff);    // query energy production
                    int32_t return_code_3 = command.sendQueryRequest(device, Command::AC_QUERY,                            0x00000000, 0xffffffff);    // query battery and grid measurements
                    int32_t return_code_4 = command.sendQueryRequest(device, Command::STATUS_QUERY,                        0x00000000, 0xffffffff);    // query device status
                    int32_t return_code_5 = command.sendQueryRequest(device, Command::EVENT_QUERY,                         0x00000000, 0xffffffff);    // query device events
                    int32_t return_code_6 = command.sendQueryRequest(device, Command::YIELD_BY_DAY_QUERY,                  0x00000000, 0xffffffff);    // query energy yield per day
                    int32_t return_code_7 = command.sendQueryRequest(device, Command::YIELD_BY_MINUTE_QUERY, 0x00000000,   0xffffffff);                // query energy yield in 5 minute intervals
                    int32_t return_code_8 = command.sendQueryRequest(device, Command::ID_UNKNOWN | Command::COMPONENT_2,   0x00000000, 0xffffffff);    // query device status (installer or bms?)
#endif
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
                    if (device.deviceClass == "Inverter" || device.deviceClass == "PV-Inverter" || device.deviceClass == "Battery-Inverter") {
                        averager.endOfSpeedwireData(device, time);
                    }
                }
            }
        }
    }

    return 0;
}

