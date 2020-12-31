#include <ObisData.hpp>


ObisData::ObisData(uint8_t channel, uint8_t index, uint8_t type, uint8_t tariff) {
    this->channel = channel;
    this->index = index;
    this->type = type;
    this->tariff = tariff;
}

bool ObisData::equals(const ObisData &other) const {
    return (channel == other.channel && index == other.index && type == other.type && tariff == other.tariff);
}

std::string ObisData::toString(void) const {
    char str[16];
    sprintf(str, "%d.%02d.%d.%d", channel, index, type, tariff);
    return std::string(str);
}

void ObisData::print(uint32_t value, FILE *file) const {
    fprintf(file, "%s 0x%08lx %lu\n", toString().c_str(), value, value);
}

void ObisData::print(uint64_t value, FILE *file) const {
    fprintf(file, "%s 0x%016llx %llu\n", toString().c_str(), value, value);
}

/*******************************/

ObisFilterData::ObisFilterData(uint8_t channel, uint8_t index, uint8_t type, uint8_t tariff, 
                                     const MeasurementType &mType, const Line lin) : 
    ObisData(channel, index, type, tariff),
    measurementType(mType),
    line(lin),
    description(mType.getFullName(lin)) {
    measurementValue = new MeasurementValue();
}

ObisFilterData::ObisFilterData(const ObisFilterData &rhs) :
    ObisFilterData(rhs.channel, rhs.index, rhs.type, rhs.tariff, rhs.measurementType, Line::TOTAL) {
    line = rhs.line;
    description = rhs.description;
    *measurementValue = *rhs.measurementValue;  // the constructor call above already allocated a new MeasurementValue instance
}

ObisFilterData &ObisFilterData::operator=(const ObisFilterData &rhs) {
    if (this != &rhs) {
        this->ObisData::operator=(rhs);
        this->measurementType = rhs.measurementType;
        this->line = rhs.line;
        this->description = rhs.description;
        *this->measurementValue = *rhs.measurementValue;
    }
    return *this;
}

ObisFilterData::~ObisFilterData(void) {
    if (measurementValue != NULL) {
        delete measurementValue;
        measurementValue = NULL;
    }
}

bool ObisFilterData::equals(const ObisData &other) const {
    return ObisData::equals(other);
}

void ObisFilterData::print(FILE *file) const {
    uint32_t timer = (measurementValue != NULL ? measurementValue->timer : 0xfffffffful);
    double   value = (measurementValue != NULL ? measurementValue->value : -999999.9999);
    fprintf(file, "%-25s  %lu  %s  => %lf %s\n", description.c_str(), timer, ObisData::toString().c_str(), value, measurementType.unit.c_str());
}
