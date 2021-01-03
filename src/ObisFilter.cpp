#include <ObisFilter.hpp>
#include <SpeedwireEmeterProtocol.hpp>


ObisFilter::ObisFilter(void) {
    // nothing to do
}

ObisFilter::~ObisFilter(void) {
    filterTable.clear();
    consumerTable.clear();
}

void ObisFilter::addFilter(const ObisFilterData &entry) {
    filterTable.push_back(entry);
}

void ObisFilter::removeFilter(const ObisFilterData &entry) {
    for (std::vector<ObisFilterData>::iterator it = filterTable.begin(); it != filterTable.end(); it++) {
        if (it->equals(entry)) {
            it = filterTable.erase(it);
        }
    }
}

const std::vector<ObisFilterData> &ObisFilter::getFilter(void) const {
    return filterTable;
}

void ObisFilter::addConsumer(ObisConsumer *obisConsumer) {
    consumerTable.push_back(obisConsumer);
}

void ObisFilter::removeConsumer(ObisConsumer *obisConsumer) {
   for (std::vector<ObisConsumer*>::iterator it = consumerTable.begin(); it != consumerTable.end(); it++) {
        if (*it == obisConsumer) {
            it = consumerTable.erase(it);
        }
    }
}

bool ObisFilter::consume(const void *const obis, const uint32_t timer) const {
    ObisData element(SpeedwireEmeterProtocol::getObisChannel(obis),
                     SpeedwireEmeterProtocol::getObisIndex(obis),
                     SpeedwireEmeterProtocol::getObisType(obis),
                     SpeedwireEmeterProtocol::getObisTariff(obis));

    const ObisFilterData *const filteredElement = filter(element);
    if (filteredElement != NULL && filteredElement->measurementValue != NULL) {
        MeasurementValue *mvalue = filteredElement->measurementValue;
        if (filteredElement->type == 4 || filteredElement->type == 7) {
            mvalue->setValue(SpeedwireEmeterProtocol::getObisValue4(obis), filteredElement->measurementType.divisor);
            mvalue->setTimer(timer);
            produce(*filteredElement);
        }
        else if (filteredElement->type == 8) {
            mvalue->setValue(SpeedwireEmeterProtocol::getObisValue8(obis), filteredElement->measurementType.divisor);
            mvalue->setTimer(timer);
            produce(*filteredElement);
        }
        else {
            perror("obis identifier not implemented");
        }
        return true;
    }
    return false;
}

const ObisFilterData *const ObisFilter::filter(const ObisData &element) const {
    for (std::vector<ObisFilterData>::const_iterator it = filterTable.begin(); it != filterTable.end(); it++) {
        if (it->equals(element)) {
            return &(*it);
        }
    }
    return NULL;
}

void ObisFilter::produce(const ObisFilterData &element) const {
    for (std::vector<ObisConsumer*>::const_iterator it = consumerTable.begin(); it != consumerTable.end(); it++) {
        (*it)->consume(element);
    }
}
