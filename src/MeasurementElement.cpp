#include <MeasurementElement.hpp>


MeasurementType::MeasurementType(const Direction direction, const Type type, const Quantity quantity, 
                                 const std::string &unit, const unsigned long divisor) {
    this->direction = direction;
    this->type = type;
    this->quantity = quantity;
    this->unit = unit;
    this->divisor = divisor;
    this->instaneous = isInstantaneous(quantity);
    this->name = toString(direction).append("_").append(toString(type)).append("_").append(toString(quantity));
}

std::string MeasurementType::getFullName(const Line line) const {
    if (line != Line::TOTAL) {
        return std::string(name).append("_").append(toString(line));
    }
    return name;
}

/*******************************/

MeasurementValue::MeasurementValue(void) {
    value = 0.0;
    timer = 0;
    elapsed = 0;
    sumValue = 0;
    counter = 0;
    initial = true;
}

void MeasurementValue::setValue(__uint32_t raw_value, unsigned long divisor) {
    value = (double)raw_value / (double)divisor;
}

void MeasurementValue::setValue(__uint64_t raw_value, unsigned long divisor) {
    value = (double)raw_value / (double)divisor;
}

void MeasurementValue::setTimer(__uint32_t time) {
    if (initial) {
        initial = false;
        timer = time;
        elapsed = 1000;
    } else {
        elapsed = time - timer;
        timer = time;
    }
}

/*******************************/

std::string toString(const Direction direction) {
    if (direction == Direction::POSITIVE) return "positive";
    if (direction == Direction::NEGATIVE) return "negative";
    return "undefined direction";
}

std::string toString(const Line line) {
    switch (line) {
        case TOTAL: return "total";
        case L1:    return "l1";
        case L2:    return "l2";
        case L3:    return "l3";
    }
    return "undefined line";
}

std::string toString(const Quantity quantity) {
    switch (quantity) {
        case POWER:     return "power";
        case ENERGY:    return "energy";
        case FREQUENCY: return "frequency";
        case PHASE:     return "phase";
        case VOLTAGE:   return "voltage";
        case CURRENT:   return "current";
    }
    return "undefined quantity";
}
bool isInstantaneous(const Quantity quantity) {
    return (quantity != ENERGY);
}

std::string toString(const Type type) {
    switch(type) {
        case ACTIVE:    return "active";
        case REACTIVE:  return "reactive";
        case APPARENT:  return "apparent";
    }
    return "undefined type";
}
