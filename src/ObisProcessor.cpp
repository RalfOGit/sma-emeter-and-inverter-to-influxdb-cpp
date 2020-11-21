#include <stdio.h>
#include <ObisProcessor.hpp>
#include <MeasurementElement.hpp>
#include <InfluxDBFactory.h>


ObisProcessor::ObisProcessor(const unsigned long averagingTime) :  
  //influxDB(influxdb::InfluxDBFactory::Get("udp://localhost:8094/?db=test")),
    influxDB(influxdb::InfluxDBFactory::Get("http://localhost:8086/?db=test")),
    influxPoint("sma_emeter") {
    influxDB->batchOf(75);
    this->averagingTime = averagingTime;
    this->remainder = 0;
    this->currentTimestamp = 0;
    this->currentTimestampIsValid = false;
    this->firstInBlock = false;
    this->averagingTimeReached = false;
}

ObisProcessor::~ObisProcessor(void) {}

void ObisProcessor::consume(const ObisFilterElement &element) {
    //element.print(stderr);
    MeasurementValue *measurement = element.measurementValue;
    if (measurement != NULL) {
        firstInBlock = false;

        // initialize current timestamp
        if (currentTimestampIsValid == false) {
            currentTimestampIsValid = true;
        }
        // check if this is the first measurement of a new measurement block
        else if (measurement->timer != currentTimestamp) {
            firstInBlock = true;
            remainder += measurement->elapsed;
            averagingTimeReached = (remainder >= averagingTime);
            if  (averagingTimeReached == true) {
                averagingTimeReached = true;
                remainder %= averagingTime;
            }
        }
        currentTimestamp = measurement->timer;

        // if less than the defined averaging time has elapsed, sum up values
        if (averagingTimeReached == false) {
            if (isInstantaneous(element.measurementType.quantity) == false) {
                measurement->sumValue += measurement->value;
                measurement->counter++;
            }
        }
        // if the averaging time has elapsed, prepare a field for the influxdb consumer
        else {
            double value;
            if (isInstantaneous(element.measurementType.quantity) == false) {
                if (measurement->counter > 0) {
                    value = measurement->sumValue / measurement->counter;
                } else { 
                    value = measurement->value;
                }
                measurement->sumValue = 0;
                measurement->counter = 0;
            } else {
                value = measurement->value;
            }
            produce(firstInBlock, element.measurementType, element.line, value);
        }
    }
}

void ObisProcessor::produce(const bool firstInBlock, const MeasurementType &type, const Line line, const double value) {
    fprintf(stderr, "%d  %s  %lf\n", firstInBlock, type.getFullName(line).c_str(), value);

    if (firstInBlock) {
        influxDB.get()->flushBuffer();
    }
    influxPoint.addTag("direction", toString(type.direction));
    influxPoint.addTag("quantity", toString(type.quantity));
    influxPoint.addTag("type", toString(type.type));
    influxPoint.addTag("line", toString(line));

    influxPoint = influxPoint.addField("value", value);

    std::string name = influxPoint.getName();
    influxDB.get()->write(std::move(influxPoint));
    influxPoint = influxdb::Point(name);
}