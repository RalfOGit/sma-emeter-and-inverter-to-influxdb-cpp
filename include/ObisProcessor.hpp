#ifndef __OBISPROCESSOR_HPP__
#define __OBISPROCESSOR_HPP__

#include <stdio.h>
#include <ObisElement.hpp>
#include <MeasurementElement.hpp>
#include <ObisFilter.hpp>
#include <InfluxDB.h>


class ObisProcessor : public ObisConsumer {

protected:
    unsigned long averagingTime;
    unsigned long remainder;
    __uint32_t    currentTimestamp;
    bool          currentTimestampIsValid;
    bool          firstInBlock;
    bool          averagingTimeReached;
    std::unique_ptr<influxdb::InfluxDB> influxDB;
    influxdb::Point                     influxPoint;

public:

    ObisProcessor(const unsigned long averagingTime);
    ~ObisProcessor(void);

    virtual void consume(const ObisFilterElement &element);
    void produce(const bool firstInBlock, const MeasurementType &type, const Line, const double value);
};

#endif
