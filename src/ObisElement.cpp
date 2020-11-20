#include <ObisElement.hpp>


ObisElement::ObisElement(__uint8_t channel, __uint8_t index, __uint8_t type, __uint8_t tariff) {
    this->channel = channel;
    this->index = index;
    this->type = type;
    this->tariff = tariff;
}

bool ObisElement::equals(const ObisElement &other) const {
    return (channel == other.channel && index == other.index && type == other.type && tariff == other.tariff);
}

std::string ObisElement::toString(void) const {
    char str[16];
    sprintf(str, "%d.%02d.%d.%d", channel, index, type, tariff);
    return std::string(str);
}

void ObisElement::print(__uint32_t value, FILE *file) const {
    fprintf(file, "%s 0x%08lx %lu\n", toString().c_str(), value, value);
}

void ObisElement::print(__uint64_t value, FILE *file) const {
    fprintf(file, "%s 0x%016llx %llu\n", toString().c_str(), value, value);
}

/*******************************/

ObisFilterElement::ObisFilterElement(__uint8_t channel, __uint8_t index, __uint8_t type, __uint8_t tariff, 
                                     const MeasurementType &mType, const Line lin) : 
    ObisElement(channel, index, type, tariff),
    measurementType(mType),
    line(lin),
    description(mType.getFullName(lin)) {
    measurementValue = new MeasurementValue();
}

ObisFilterElement::ObisFilterElement(const ObisFilterElement &rhs) :
    ObisFilterElement(rhs.channel, rhs.index, rhs.type, rhs.tariff, rhs.measurementType, TOTAL) {
    line = rhs.line;
    description = rhs.description;
    *measurementValue = *rhs.measurementValue;  // the constructor call above already allocated a new MeasurementValue instance
}

ObisFilterElement &ObisFilterElement::operator=(const ObisFilterElement &rhs) {
    if (this != &rhs) {
        this->ObisElement::operator=(rhs);
        this->measurementType = rhs.measurementType;
        this->line = rhs.line;
        this->description = rhs.description;
        *this->measurementValue = *rhs.measurementValue;
    }
    return *this;
}

ObisFilterElement::~ObisFilterElement(void) {
    if (measurementValue != NULL) {
        delete measurementValue;
        measurementValue = NULL;
    }
}

bool ObisFilterElement::equals(const ObisElement &other) const {
    return (channel == other.channel && index == other.index && type == other.type && tariff == other.tariff);
}

void ObisFilterElement::print(FILE *file) const {
    uint32_t timer = (measurementValue != NULL ? measurementValue->timer : 0xfffffffful);
    double   value = (measurementValue != NULL ? measurementValue->value : -999999.9999);
    fprintf(file, "%-25s  %lu  %s  => %lf %s\n", description.c_str(), timer, ObisElement::toString().c_str(), value, measurementType.unit.c_str());
}
