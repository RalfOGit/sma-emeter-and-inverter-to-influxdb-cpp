#ifndef __INFLUXDBEXPORT_HPP__
#define __INFLUXDBEXPORT_HPP__

#include <memory>               // for std::unique_ptr
#include <Measurement.hpp>
#include <Producer.hpp>
#include <SpeedwireDevice.hpp>
#include <InfluxDB.h>

class InfluxDBProducer : public libspeedwire::Producer {

protected:
    std::unique_ptr<influxdb::InfluxDB> influxDB;
    influxdb::Point                     influxPoint;

public:

    InfluxDBProducer(void);
    ~InfluxDBProducer(void);

    void flush(void) override;
    void produce(const libspeedwire::SpeedwireDevice& device, const libspeedwire::MeasurementType &type, const libspeedwire::Wire, const double value, const uint32_t time_in_ms = 0) override;

};

#endif
