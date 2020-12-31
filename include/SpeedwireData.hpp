#ifndef __SPEEDWIREDATA_HPP__
#define __SPEEDWIREDATA_HPP__

#include <cstdint>
#include <string>
#include <stdio.h>
#include <Measurement.hpp>


/**
 *  Class holding raw data from the speedwire inverter reply packet
 */
class SpeedwireData {
public:
    uint32_t command;                   // command code
    uint32_t id;                        // register id
    uint8_t  conn;                      // connector id (mpp #1, mpp #2, ac #1)
    uint8_t  type;                      // unknown type
    time_t   time;                      // timestamp
    uint8_t  data[40];                  // payload data
    size_t   data_size;                 // payload data size in bytes

    SpeedwireData(const uint32_t command, const uint32_t id, const uint8_t conn, const uint8_t type, const time_t time, const void* const data, const size_t data_size);

    bool equals(const SpeedwireData& other) const;
    bool isSameSignature(const SpeedwireData& other) const;

    std::string toString(void) const;
    void print(uint32_t value, FILE* file) const;
    void print(uint64_t value, FILE* file) const;
};


/**
 *  Class holding data from the speedwire inverter reply packet, enriched by measurement type information 
 *  and the interpreted measurement value
 */
class SpeedwireFilterData : public SpeedwireData {
public:
    MeasurementType   measurementType;
    MeasurementValue* measurementValue;
    Line              line;
    std::string       description;

    SpeedwireFilterData(const uint32_t command, const uint32_t id, const uint8_t conn, const uint8_t type, const time_t time, const void* data, const size_t data_size,
                        const MeasurementType& mType, const Line _line);
    SpeedwireFilterData(const SpeedwireFilterData& rhs);
    SpeedwireFilterData(void);
    SpeedwireFilterData& operator=(const SpeedwireFilterData& rhs);

    ~SpeedwireFilterData(void);

    bool consume(const SpeedwireData& data);

    void print(FILE* file) const;
};


/**
 *  Interface to be implemented by the consumer of speedwire inverter reply data
 */
class SpeedwireConsumer {
public:
    virtual void consume(const SpeedwireFilterData& element) = 0;
};

#endif
