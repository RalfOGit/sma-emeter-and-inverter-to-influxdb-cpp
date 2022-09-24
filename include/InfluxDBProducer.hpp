#ifndef __INFLUXDBEXPORT_HPP__
#define __INFLUXDBEXPORT_HPP__

#include <memory>               // for std::unique_ptr
#include <Measurement.hpp>
#include <Producer.hpp>
#include <SpeedwireDiscovery.hpp>
#include <InfluxDB.h>



class InfluxDBProducer : public libspeedwire::Producer {

protected:
    const std::vector<libspeedwire::SpeedwireInfo>& devices;
    std::unique_ptr<influxdb::InfluxDB> influxDB;
    influxdb::Point                     influxPoint;

public:

    InfluxDBProducer(const std::vector<libspeedwire::SpeedwireInfo>& devices);
    ~InfluxDBProducer(void);

    void flush(void) override;
    void produce(const uint32_t serial_number, const libspeedwire::MeasurementType &type, const libspeedwire::Wire, const double value, const uint32_t time_in_ms = 0) override;

};

#endif
