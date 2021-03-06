#include <InfluxDBProducer.hpp>
#include <InfluxDBFactory.h>
using namespace libspeedwire;


InfluxDBProducer::InfluxDBProducer(const std::vector<SpeedwireInfo>& device_array) :
    devices(device_array),
  //influxDB(influxdb::InfluxDBFactory::Get("udp://localhost:8094/?db=test")),
    influxDB(influxdb::InfluxDBFactory::Get("http://localhost:8086/?db=test")),
  //influxDB(influxdb::InfluxDBFactory::Get("http://192.168.178.16:8086/?db=test")),
    influxPoint("sma_emeter") {
    influxDB->batchOf(100);
}

InfluxDBProducer::~InfluxDBProducer(void) {}


void InfluxDBProducer::flush(void) {
    influxDB.get()->flushBuffer();
}


void InfluxDBProducer::produce(const uint32_t serial_number, const MeasurementType &type, const Wire line, const double value) {

    if (serial_number == 0xcafebabe) {
        fprintf(stderr, "house_%s  %lf\n", type.getFullName(line).c_str(), value);
        influxPoint.addTag("device", "house");
    }
    else {
        fprintf(stderr, "%s  %lf\n", type.getFullName(line).c_str(), value);
        for (size_t i = 0; i < devices.size(); ++i) {
            if (devices[i].serialNumber == serial_number) {
                if (devices[i].deviceClass == "Emeter") {
                    influxPoint.addTag("device", "meter"); 
                }
                else if (devices[i].deviceClass == "Inverter") {
                    influxPoint.addTag("device", "inverter"); 
                }
                break;
            }
        }
    }

    char serial[32];
    snprintf(serial, sizeof(serial), "%lu", serial_number);
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

    std::string name = influxPoint.getName();
    influxDB.get()->write(std::move(influxPoint));
    influxPoint = influxdb::Point(name);
}