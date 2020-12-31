#include <stdio.h>
#include <DataProcessor.hpp>
#include <Measurement.hpp>
#include <InfluxDBFactory.h>


DataProcessor::DataProcessor(const unsigned long averagingTime) :  
  //influxDB(influxdb::InfluxDBFactory::Get("udp://localhost:8094/?db=test")),
    influxDB(influxdb::InfluxDBFactory::Get("http://localhost:8086/?db=test")),
  //influxDB(influxdb::InfluxDBFactory::Get("http://192.168.178.16:8086/?db=test")),
    influxPoint("sma_emeter") {
    influxDB->batchOf(75);
    this->averagingTime = averagingTime;
    this->obis_remainder = 0;
    this->obis_currentTimestamp = 0;
    this->obis_currentTimestampIsValid = false;
    this->obis_averagingTimeReached = false;
    this->speedwire_remainder = 0;
    this->speedwire_currentTimestamp = 0;
    this->speedwire_currentTimestampIsValid = false;
    this->speedwire_averagingTimeReached = false;
}

DataProcessor::~DataProcessor(void) {}

void DataProcessor::consume(const ObisFilterData &element) {
    //element.print(stdout);
    MeasurementValue *measurement = element.measurementValue;
    if (measurement != NULL) {
        bool firstInBlock = false;

        // initialize current timestamp
        if (obis_currentTimestampIsValid == false) {
            obis_currentTimestampIsValid = true;
        }
        // check if this is the first measurement of a new measurement block
        else if (measurement->timer != obis_currentTimestamp) {
            firstInBlock = true;
            obis_remainder += measurement->elapsed;
            obis_averagingTimeReached = (obis_remainder >= averagingTime);
            if  (obis_averagingTimeReached == true) {
                obis_remainder %= averagingTime;
            }
        }
        obis_currentTimestamp = measurement->timer;

        // if less than the defined averaging time has elapsed, sum up values
        if (obis_averagingTimeReached == false) {
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


void DataProcessor::consume(const SpeedwireFilterData& element) {
    //element.print(stdout);
    MeasurementValue* measurement = element.measurementValue;
    if (measurement != NULL) {
        bool firstInBlock = false;

        // initialize current timestamp
        if (speedwire_currentTimestampIsValid == false) {
            speedwire_currentTimestampIsValid = true;
        }
        // check if this is the first measurement of a new measurement block
        else if ((measurement->timer - speedwire_currentTimestamp) > 2) {   // inverter time may increase during a query block
            firstInBlock = true;
            speedwire_remainder += measurement->elapsed;
            speedwire_averagingTimeReached = (speedwire_remainder >= (averagingTime / 1000));   // inverter timestamps are in seconds
            if (speedwire_averagingTimeReached == true) {
                speedwire_remainder %= (averagingTime / 1000);
            }
        }
        speedwire_currentTimestamp = measurement->timer;

        // if less than the defined averaging time has elapsed, sum up values
        if (speedwire_averagingTimeReached == false) {
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
                }
                else {
                    value = measurement->value;
                }
                measurement->sumValue = 0;
                measurement->counter = 0;
            }
            else {
                value = measurement->value;
            }
            produce(firstInBlock, element.measurementType, element.line, value);
        }
    }
}


void DataProcessor::produce(const bool firstInBlock, const MeasurementType &type, const Line line, const double value) {
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