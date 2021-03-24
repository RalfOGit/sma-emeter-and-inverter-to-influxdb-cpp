#ifndef __OBISPROCESSOR_HPP__
#define __OBISPROCESSOR_HPP__

#include <cstdint>
#include <memory>               // for std::unique_ptr
#include <string_view>          // for std::string_view (forgotten in <InfluxDB.h>)
#include <ObisData.hpp>
#include <SpeedwireData.hpp>
#include <Measurement.hpp>
#include <ObisFilter.hpp>


class Producer;


class DataProcessor : public ObisConsumer, SpeedwireConsumer {

protected:
    unsigned long averagingTime;
    unsigned long obis_remainder;
    uint32_t      obis_currentTimestamp;
    bool          obis_currentTimestampIsValid;
    bool          obis_averagingTimeReached;
    unsigned long speedwire_remainder;
    uint32_t      speedwire_currentTimestamp;
    bool          speedwire_currentTimestampIsValid;
    bool          speedwire_averagingTimeReached;
    Producer&     producer;

public:

    DataProcessor(const unsigned long averagingTime, Producer& producer);
    ~DataProcessor(void);

    virtual void consume(const ObisData &element);
    virtual void consume(const SpeedwireData& element);
};

#endif
