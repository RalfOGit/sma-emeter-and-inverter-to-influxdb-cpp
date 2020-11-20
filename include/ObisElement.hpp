#ifndef __OBISELEMENT_HPP__
#define __OBISELEMENT_HPP__

#include <cstdint>
#include <stdio.h>
#include <string>
#include <MeasurementElement.hpp>

class ObisElement {
public:
    uint8_t channel;
    uint8_t index;
    uint8_t type;
    uint8_t tariff;
 
    ObisElement(uint8_t channel, uint8_t index, uint8_t type, uint8_t tariff);

    bool equals(const ObisElement &other) const;

    std::string toString(void) const;
    void print(uint32_t value, FILE *file) const;
    void print(uint64_t value, FILE *file) const;
};


class ObisFilterElement : public ObisElement {
public:
    MeasurementType   measurementType;
    MeasurementValue *measurementValue;
    Line              line;
    std::string       description;

    ObisFilterElement(uint8_t channel, uint8_t index, uint8_t type, uint8_t tariff, 
                      const MeasurementType &measurementType, const Line line);
    ObisFilterElement(const ObisFilterElement &rhs);
    ObisFilterElement &operator=(const ObisFilterElement &rhs);
    
    ~ObisFilterElement(void);

    bool equals(const ObisElement &other) const;

    void print(FILE *file) const;
};

#endif
