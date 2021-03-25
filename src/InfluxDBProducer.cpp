#include <InfluxDBProducer.hpp>
#include <InfluxDBFactory.h>


InfluxDBProducer::InfluxDBProducer(void) :
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


void InfluxDBProducer::produce(const std::string &device, const MeasurementType &type, const Line line, const double value) {
    fprintf(stderr, "%s  %lf\n", type.getFullName(line).c_str(), value);

    influxPoint.addTag("device", device);

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