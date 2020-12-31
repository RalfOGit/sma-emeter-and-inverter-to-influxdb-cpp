#include <memory.h>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireData.hpp>


/**
 *  Constructor
 */
SpeedwireData::SpeedwireData(const uint32_t _command, const uint32_t _id, const uint8_t _conn, const uint8_t _type, const time_t _time, const void *const _data, const size_t _data_size) :
    command(_command),
    id(_id),
    conn(_conn),
    type(_type),
    time(_time) {
    data_size = (_data_size < sizeof(data) ? _data_size : sizeof(data));
    memset(data, 0, sizeof(data));
    if (data != NULL) {
        memcpy(data, _data, data_size);
    }
}


/**
 *  Compare two instances of SpeedwireData with each other
 */
bool SpeedwireData::equals(const SpeedwireData& other) const {
    return (command == other.command && id == other.id && conn == other.conn && type == other.type && time == other.time && data_size == other.data_size && memcmp(data, other.data, data_size) == 0);
}


/**
 *  Compare two instance signatures of SpeedwireData with each other
 */
bool SpeedwireData::isSameSignature(const SpeedwireData& other) const {
    return (command == other.command && id == other.id && conn == other.conn && type == other.type);
}



/**
 *  Convert SpeedwireData into a std::string represenation
 */
std::string SpeedwireData::toString(void) const {
    char buff[256];
    snprintf(buff, sizeof(buff), "id 0x%08lx conn 0x%02x type 0x%02x  time 0x%08lx data 0x", (unsigned)id, (unsigned)conn, (unsigned)type, (uint32_t)time);
    std::string result(buff);
    for (int i = 0; i < data_size; ++i) {
        char byte[4];
        snprintf(byte, sizeof(byte), "%02x", (unsigned)data[i]);
        result.append(byte);
    }
    return result;
}

void SpeedwireData::print(uint32_t value, FILE* file) const {
    fprintf(file, "%s 0x%08lx %lu\n", toString().c_str(), value, value);
}

void SpeedwireData::print(uint64_t value, FILE* file) const {
    fprintf(file, "%s 0x%016llx %llu\n", toString().c_str(), value, value);
}


/*******************************/


/**
 *  Constructor
 */
SpeedwireFilterData::SpeedwireFilterData(const uint32_t command, const uint32_t id, const uint8_t conn, const uint8_t type, const time_t time, const void *const data, const size_t data_size,
                                         const MeasurementType& mType, const Line _line) :
    SpeedwireData(command, id, conn, type, time, data, data_size),
    measurementType(mType),
    line(_line),
    description(mType.getFullName(_line)) {
    measurementValue = new MeasurementValue();
}


/**
 *  Copy constructor
 */
SpeedwireFilterData::SpeedwireFilterData(const SpeedwireFilterData& rhs) :
    SpeedwireFilterData(rhs.command, rhs.id, rhs.conn, rhs.type, rhs.time, &rhs.data, rhs.data_size, rhs.measurementType, rhs.line) {
    line = rhs.line;
    description = rhs.description;
    *measurementValue = *rhs.measurementValue;  // the constructor call above already allocated a new MeasurementValue instance
}


/**
 *  Default constructor, not very useful, but needed for std::map
 */
SpeedwireFilterData::SpeedwireFilterData(void) :
    SpeedwireData(0, 0, 0, 0, 0, NULL, 0),
    measurementType(Direction::NO_DIRECTION, Type::NO_TYPE, Quantity::NO_QUANTITY, "", 0),
    line(Line::NO_LINE),
    description() {
    measurementValue = new MeasurementValue();
}


/**
 *  Assignment operator
 */
SpeedwireFilterData& SpeedwireFilterData::operator=(const SpeedwireFilterData& rhs) {
    if (this != &rhs) {
        this->SpeedwireData::operator=(rhs);
        this->measurementType = rhs.measurementType;
        this->line = rhs.line;
        this->description = rhs.description;
        *this->measurementValue = *rhs.measurementValue;
    }
    return *this;
}


/**
 *  Destructor
 */
SpeedwireFilterData::~SpeedwireFilterData(void) {
    if (measurementValue != NULL) {
        delete measurementValue;
        measurementValue = NULL;
    }
}


/**
 *  Consume the given inverter data, i.e. interprete register id and convert to physical values
 */
bool SpeedwireFilterData::consume(const SpeedwireData& data) {
    if (!isSameSignature(data)) return false;
    if (data.data == NULL || data.data_size < 20) return false;

    MeasurementValue* mvalue = measurementValue;
    uint8_t  value1;
    uint32_t value4;
    uint64_t value8;

    switch (id) {
    case 0x00251e00:    // dc power
    case 0x00451f00:    // dc voltage
    case 0x00452100:    // dc current
    case 0x00464000:    // ac power
    case 0x00464100:    // ac power
    case 0x00464200:    // ac power
    case 0x00464800:    // ac voltage
    case 0x00464900:    // ac voltage
    case 0x00464a00:    // ac voltage
    case 0x00464b00:    // ac voltage
    case 0x00464c00:    // ac voltage
    case 0x00464d00:    // ac voltage
    case 0x00465300:    // ac current
    case 0x00465400:    // ac current
    case 0x00465500:    // ac current
        value4 = SpeedwireByteEncoding::getUint32LittleEndian(data.data);
        if (value4 == 0xffffffff || value4 == 0x80000000) value4 = 0;  // received during darkness
#if 0   // simulate some values
        if (id == 0x00251e00) value4 = 0x57;
        if (id == 0x00451f00) value4 = 0x6105;
        if (id == 0x00452100) value4 = 0x0160;
        if (id == 0x00464000 || id == 0x00464100 || id == 0x00464200) value4 = 0x0038;
        if (id == 0x00464800 || id == 0x00464900 || id == 0x00464a00) value4 = 0x59cf;
        if (id == 0x00464b00 || id == 0x00464c00 || id == 0x00464d00) value4 = 0x9b3c;
        if (id == 0x00465300 || id == 0x00465400 || id == 0x00465500) value4 = 0x011e;
#endif
        mvalue->setValue(value4, measurementType.divisor);
        mvalue->setTimer(data.time);
        time = data.time;
        break;

    case 0x00214800:    // device status
    case 0x00416400:    // grid relay status
        // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000980 00028051 00482100 ff482100 00000000 =>  query device status
        // Response 534d4100000402a000000001004e0010 606513a0 7d0042be283a00a1 7a01842a71b30001 000000000980 01028051 00000000 00000000 01482108 59c5e95f 33010001 feffff00 00000000 00000000 00000000 00000000 00000000 00000000 00000000
        // Request  534d4100000402a00000000100260010 606509a0 7a01842a71b30001 7d0042be283a0001 000000000a80 00028051 00644100 ff644100 00000000 =>  query grid relay status
        // Response 534d4100000402a000000001004e0010 606513a0 7d0042be283a00a1 7a01842a71b30001 000000000a80 01028051 07000000 07000000 01644108 59c5e95f 33000001 37010000 fdffff00 feffff00 00000000 00000000 00000000 00000000 00000000
        value4 = SpeedwireByteEncoding::getUint32LittleEndian(data.data);
        value1 = (value4 >> 24) & 0xff;
        mvalue->setValue((uint32_t)value1, measurementType.divisor);
        mvalue->setTimer(data.time);
        time = data.time;
        break;

    default:
        perror("unknown id");
        return false;
    }

    return true;
}

void SpeedwireFilterData::print(FILE* file) const {
    uint32_t timer = (measurementValue != NULL ? measurementValue->timer : 0xfffffffful);
    double   value = (measurementValue != NULL ? measurementValue->value : -999999.9999);
    fprintf(file, "%-16s  time %lu  %s  => %lf %s\n", description.c_str(), timer, SpeedwireData::toString().c_str(), value, measurementType.unit.c_str());
}
