#include <InfluxDBProducer.hpp>
#include <InfluxDBFactory.h>
#include <SpeedwireTime.hpp>
using namespace libspeedwire;
using namespace std::chrono;

#define PRINT_STYLE_NONE (0)
#define PRINT_STYLE_SHORT (1)
#define PRINT_STYLE_LINEPROTOCOL (2)
#define PRINT_STYLE PRINT_STYLE_SHORT

typedef struct {
    uint32_t sma_time;
    system_clock::time_point system_time;
} TimeMapping;

static TimeMapping lastEmeterTime = { 0 };
static TimeMapping lastInverterTime = { 0 };
static TimeMapping lastBatteryTime = { 0 };


InfluxDBProducer::InfluxDBProducer(void) :
  //influxDB(influxdb::InfluxDBFactory::Get("udp://localhost:8094/?db=test")),
  influxDB(influxdb::InfluxDBFactory::Get("http://localhost:8086/?db=test")),
  //  influxDB(influxdb::InfluxDBFactory::Get("http://192.168.178.16:8086/?db=test")),
    influxPoint("sma_emeter") {
    influxDB->batchOf(100);
}

InfluxDBProducer::~InfluxDBProducer(void) {}


void InfluxDBProducer::flush(void) {
    influxDB.get()->flushBuffer();
}


void InfluxDBProducer::produce(const SpeedwireDevice &device, const MeasurementType &type, const Wire line, const double value, const uint32_t time) {
    system_clock::time_point system_time(system_clock::duration::zero());   // initialize to 0

    // fake serial number for household consumption measurements
    if (device.deviceAddress.serialNumber == 0xcafebabe) {
        influxPoint.addTag("device", "house");
        if (time != 0) {
            if (lastEmeterTime.sma_time != time) {
                lastEmeterTime.sma_time = time;
                milliseconds millis(SpeedwireTime::convertEmeterTimeToUnixEpochTime(time));
                lastEmeterTime.system_time = system_clock::time_point(duration_cast<system_clock::duration>(millis));
            }
            system_time = lastEmeterTime.system_time;
        }
#if PRINT_STYLE == PRINT_STYLE_SHORT
        fprintf(stderr, "%llu  house_%-16s  %lf\n", system_time.time_since_epoch().count(), type.getFullName(line).c_str(), value);
#endif
    }
    // fake serial number for experimental emeter measurements
    else if (device.deviceAddress.serialNumber == 1234567890) {
        influxPoint.addTag("device", "meter");
        if (time != 0) {
            milliseconds millis(SpeedwireTime::convertEmeterTimeToUnixEpochTime(time));
            system_time = system_clock::time_point(duration_cast<system_clock::duration>(millis));
        }
#if PRINT_STYLE == PRINT_STYLE_SHORT
        fprintf(stderr, "%llu  experimental_%-16s  %lf\n", system_time.time_since_epoch().count(), type.getFullName(line).c_str(), value);
#endif
    }
    // determine device type by its serial number
    else if (device.deviceClass == "Emeter") {
        influxPoint.addTag("device", "meter");
        if (time != 0) {
            if (lastEmeterTime.sma_time != time) {
                lastEmeterTime.sma_time = time;
                milliseconds millis(SpeedwireTime::convertEmeterTimeToUnixEpochTime(time));
                lastEmeterTime.system_time = system_clock::time_point(duration_cast<system_clock::duration>(millis));
            }
            system_time = lastEmeterTime.system_time;
        }
    }
    else if (device.deviceClass == "Inverter" || device.deviceClass == "PV-Inverter") {
        influxPoint.addTag("device", "inverter"); 
        if (time != 0) {
            if (lastInverterTime.sma_time != time) {
                lastInverterTime.sma_time = time;
                milliseconds millis(SpeedwireTime::convertInverterTimeToUnixEpochTime(time));
                lastInverterTime.system_time = system_clock::time_point(duration_cast<system_clock::duration>(millis));
            }
            system_time = lastInverterTime.system_time;
        }
    }
    else if (device.deviceClass == "Battery-Inverter") {
        influxPoint.addTag("device", "battery");
        if (time != 0) {
            if (lastBatteryTime.sma_time != time) {
                lastBatteryTime.sma_time = time;
                milliseconds millis(SpeedwireTime::convertInverterTimeToUnixEpochTime(time));
                lastBatteryTime.system_time = system_clock::time_point(duration_cast<system_clock::duration>(millis));
            }
            system_time = lastBatteryTime.system_time;
        }
    }
#if PRINT_STYLE == PRINT_STYLE_SHORT
    fprintf(stderr, "%llu  %-22s  %lf     %s\n", system_time.time_since_epoch().count(), type.getFullName(line).c_str(), value, influxPoint.getTags().c_str());
#endif
    char serial[32];
    snprintf(serial, sizeof(serial), "%lu", device.deviceAddress.serialNumber);
    influxPoint.addTag("serial", std::string(serial));

    std::string str_direction(toString(type.direction));
    std::string str_quantity(toString(type.quantity));
    std::string str_type(toString(type.type));
    std::string str_line(toString(line));
    if (str_direction.length() > 0) {
        influxPoint.addTag("direction", str_direction);
    }
    if (str_quantity.length() > 0) {
        influxPoint.addTag("quantity", str_quantity);
    }
    if (str_type.length() > 0) {
        influxPoint.addTag("type", str_type);
    }
    if (str_line.length() > 0) {
        influxPoint.addTag("line", str_line);
    }

    influxPoint = influxPoint.addField("value", value);

    // if a measurement timestamp is given, use it as InfluxDB timestamp
    if (system_time.time_since_epoch().count() != 0) {
        influxPoint.setTimestamp(system_time);
    }

    std::string name = influxPoint.getName();
    influxDB.get()->write(std::move(influxPoint));

#if PRINT_STYLE == PRINT_STYLE_LINEPROTOCOL
    std::string influx_protocol = influxPoint.toLineProtocol();
    fprintf(stderr, "%s  %llu\n", influx_protocol.substr(0, influx_protocol.length()-6).c_str(), LocalHost::getUnixEpochTimeInMs());  // cut off the ns part and just display ms.
#endif

    influxPoint = influxdb::Point(name);
}