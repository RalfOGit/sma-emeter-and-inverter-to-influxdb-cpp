#include <ObisFilter.hpp>
#include <SpeedwireEmeter.hpp>


ObisFilter::ObisFilter(void) {
    // nothing to do
}

ObisFilter::~ObisFilter(void) {
    filterTable.clear();
    consumerTable.clear();
}

void ObisFilter::addFilter(const ObisFilterElement &entry) {
    filterTable.push_back(entry);
}

void ObisFilter::removeFilter(const ObisFilterElement &entry) {
    for (std::vector<ObisFilterElement>::iterator it = filterTable.begin(); it != filterTable.end(); it++) {
        if (it->equals(entry)) {
            it = filterTable.erase(it);
        }
    }
}

const std::vector<ObisFilterElement> &ObisFilter::getFilter(void) const {
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

bool ObisFilter::consume(const void *const obis, const __uint32_t timer) const {
    ObisElement element(SpeedwireEmeter::getObisChannel(obis),
                        SpeedwireEmeter::getObisIndex(obis),
                        SpeedwireEmeter::getObisType(obis),
                        SpeedwireEmeter::getObisTariff(obis));

    const ObisFilterElement *const filteredElement = filter(element);
    if (filteredElement != NULL && filteredElement->measurementValue != NULL) {
        MeasurementValue *mvalue = filteredElement->measurementValue;
        if (filteredElement->type == 4) {
            mvalue->setValue(SpeedwireEmeter::getObisValue4(obis), filteredElement->measurementType.divisor);
            mvalue->setTimer(timer);
            produce(*filteredElement);
        }
        else if (filteredElement->type == 8) {
            mvalue->setValue(SpeedwireEmeter::getObisValue8(obis), filteredElement->measurementType.divisor);
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

const ObisFilterElement *const ObisFilter::filter(const ObisElement &element) const {
    for (std::vector<ObisFilterElement>::const_iterator it = filterTable.begin(); it != filterTable.end(); it++) {
        if (it->equals(element)) {
            return &(*it);
        }
    }
    return NULL;
}

void ObisFilter::produce(const ObisFilterElement &element) const {
    for (std::vector<ObisConsumer*>::const_iterator it = consumerTable.begin(); it != consumerTable.end(); it++) {
        (*it)->consume(element);
    }
}
