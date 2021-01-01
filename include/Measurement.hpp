#ifndef __MEASUREMENT_HPP__
#define __MEASUREMENT_HPP__

#include <cstdint>
#include <string>


enum class Direction {
    POSITIVE,
    NEGATIVE,
    NO_DIRECTION
};
std::string toString(const Direction direction);

enum class Line {
    TOTAL,
    L1,
    L2,
    L3,
    MPP1,
    MPP2,
    DEVICE_OK,
    RELAY_ON,
    NO_LINE
};
std::string toString(const Line line);

enum class Quantity {
    POWER,
    ENERGY,
    POWER_FACTOR,
    CURRENT,
    VOLTAGE,
    STATUS,
    NO_QUANTITY
};
std::string toString(const Quantity quantity);
bool isInstantaneous(const Quantity quantity);

enum class Type {
    ACTIVE,
    REACTIVE,
    APPARENT,
    NO_TYPE
};
std::string toString(const Type type);


class MeasurementType {
public:
    std::string name;
    std::string unit;       // measurement unit after applying the divisor, e.g. W, kWh
    unsigned long divisor;  // divide value by divisor to obtain floating point measurements in the given unit
    bool instaneous;        // true for power measurements, false for energy measurements
    Direction direction;    // true for consumed from grid, false for provided to the grid
    Quantity quantity;
    Type type;

    MeasurementType(const Direction direction, const Type type, const Quantity quantity, 
                    const std::string &unit, const unsigned long divisor);

    std::string getFullName(const Line line) const;
};


class MeasurementValue {
public:
    double       value;         // the current measurement
    uint32_t     timer;         // the current timestamp
    uint32_t     elapsed;       // time elapsed from previous timestamp to current timestamp
    double       sumValue;      // the sum of previous and current measurements
    unsigned int counter;       // the number of measurements included in sumValue
    bool         initial;

    MeasurementValue(void);
    void setValue(uint32_t raw_value, unsigned long divisor);
    void setValue(uint64_t raw_value, unsigned long divisor);
    void setTimer(uint32_t timer);
};

#endif