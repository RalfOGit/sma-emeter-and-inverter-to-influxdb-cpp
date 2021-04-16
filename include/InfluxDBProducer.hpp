#ifndef __INFLUXDBEXPORT_HPP__
#define __INFLUXDBEXPORT_HPP__

#include <memory>               // for std::unique_ptr
#include <Measurement.hpp>
#include <InfluxDB.h>
#include <Producer.hpp>


class InfluxDBProducer : public Producer {

protected:
    std::unique_ptr<influxdb::InfluxDB> influxDB;
    influxdb::Point                     influxPoint;

public:

    InfluxDBProducer(void);
    ~InfluxDBProducer(void);

    void flush(void) override;
    void produce(const std::string &device, const MeasurementType &type, const Wire, const double value) override;

};

#endif
