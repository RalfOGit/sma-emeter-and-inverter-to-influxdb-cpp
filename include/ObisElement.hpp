#ifndef __OBISELEMENT_HPP__
#define __OBISELEMENT_HPP__

#include <bits/types.h>
#include <stdio.h>
#include <string>
#include <MeasurementElement.hpp>

class ObisElement {
public:
    __uint8_t channel;
    __uint8_t index;
    __uint8_t type;
    __uint8_t tariff;
 
    ObisElement(__uint8_t channel, __uint8_t index, __uint8_t type, __uint8_t tariff);

    bool equals(const ObisElement &other) const;

    std::string toString(void) const;
    void print(__uint32_t value, FILE *file) const;
    void print(__uint64_t value, FILE *file) const;
};


class ObisFilterElement : public ObisElement {
public:
    MeasurementType   measurementType;
    MeasurementValue *measurementValue;
    Line              line;
    std::string       description;

    ObisFilterElement(__uint8_t channel, __uint8_t index, __uint8_t type, __uint8_t tariff, 
                      const MeasurementType &measurementType, const Line line);
    ObisFilterElement(const ObisFilterElement &rhs);
    ObisFilterElement &operator=(const ObisFilterElement &rhs);
    
    ~ObisFilterElement(void);

    bool equals(const ObisElement &other) const;

    void print(FILE *file) const;
};

#endif
