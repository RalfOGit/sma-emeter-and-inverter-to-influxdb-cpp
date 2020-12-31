#ifndef __OBISDATA_HPP__
#define __OBISDATA_HPP__

#include <cstdint>
#include <stdio.h>
#include <string>
#include <Measurement.hpp>

class ObisData {
public:
    uint8_t channel;
    uint8_t index;
    uint8_t type;
    uint8_t tariff;
 
    ObisData(uint8_t channel, uint8_t index, uint8_t type, uint8_t tariff);

    bool equals(const ObisData &other) const;

    std::string toString(void) const;
    void print(uint32_t value, FILE *file) const;
    void print(uint64_t value, FILE *file) const;
};


class ObisFilterData : public ObisData {
public:
    MeasurementType   measurementType;
    MeasurementValue *measurementValue;
    Line              line;
    std::string       description;

    ObisFilterData(uint8_t channel, uint8_t index, uint8_t type, uint8_t tariff, 
                   const MeasurementType &measurementType, const Line line);
    ObisFilterData(const ObisFilterData &rhs);
    ObisFilterData &operator=(const ObisFilterData &rhs);
    
    ~ObisFilterData(void);

    bool equals(const ObisData &other) const;

    void print(FILE *file) const;
};

#endif
