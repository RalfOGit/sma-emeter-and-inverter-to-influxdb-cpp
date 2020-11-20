#include <SpeedwireSocket.hpp>
#include <SpeedwireProtocol.hpp>
#include <SpeedwireEmeter.hpp>
#include <MeasurementElement.hpp>
#include <ObisFilter.hpp>
#include <ObisProcessor.hpp>


int main(int argc, char **argv) {

    SpeedwireSocket *socket = SpeedwireSocket::getInstance();

    MeasurementType positive_power (Direction::POSITIVE, Type::ACTIVE, Quantity::POWER,  "W",        10);
    MeasurementType positive_energy(Direction::POSITIVE, Type::ACTIVE, Quantity::ENERGY, "kWh", 3600000);
    MeasurementType negative_power (Direction::NEGATIVE, Type::ACTIVE, Quantity::POWER,  "W",        10);
    MeasurementType negative_energy(Direction::NEGATIVE, Type::ACTIVE, Quantity::ENERGY, "kWh", 3600000);

    ObisFilter filter;
    filter.addFilter(ObisFilterElement(0, 1, 4, 0, positive_power, TOTAL));
    filter.addFilter(ObisFilterElement(0,21, 4, 0, positive_power, L1));
    filter.addFilter(ObisFilterElement(0,41, 4, 0, positive_power, L2)); 
    filter.addFilter(ObisFilterElement(0,61, 4, 0, positive_power, L3)); 
    filter.addFilter(ObisFilterElement(0, 1, 8, 0, positive_energy, TOTAL));
    filter.addFilter(ObisFilterElement(0,21, 8, 0, positive_energy, L1)); 
    filter.addFilter(ObisFilterElement(0,41, 8, 0, positive_energy, L2));  
    filter.addFilter(ObisFilterElement(0,61, 8, 0, positive_energy, L3));  
    //filter.addFilter(ObisFilterElement(0, 2, 4, 0, negative_power, TOTAL));
    //filter.addFilter(ObisFilterElement(0,22, 4, 0, negative_power, L1));
    //filter.addFilter(ObisFilterElement(0,42, 4, 0, negative_power, L2));  
    //filter.addFilter(ObisFilterElement(0,62, 4, 0, negative_power, L3));  
    //filter.addFilter(ObisFilterElement(0, 2, 8, 0, negative_energy, TOTAL));
    //filter.addFilter(ObisFilterElement(0,22, 8, 0, negative_energy, L1));
    //filter.addFilter(ObisFilterElement(0,42, 8, 0, negative_energy, L2)); 
    //filter.addFilter(ObisFilterElement(0,62, 8, 0, negative_energy, L3)); 

    ObisProcessor processor(60000);
    filter.addConsumer(&processor);

    unsigned char multicast_packet[1024];
    while(1) {
        int nbytes = socket->recv(multicast_packet, sizeof(multicast_packet));
        SpeedwireProtocol protocol(multicast_packet, nbytes);
        bool valid     = protocol.checkHeader();
        int group      = protocol.getGroup();
        int length     = protocol.getLength();
        int protocolID = protocol.getProtocolID();
        int offset     = protocol.getPayloadOffset();

        SpeedwireEmeter emeter(multicast_packet + offset, nbytes - offset);
        __uint16_t susyid = emeter.getSusyID();
        __uint32_t serial = emeter.getSerialNumber();
        __uint32_t timer  = emeter.getTime();

        void *obis = emeter.getFirstObisElement();
        while (obis != NULL) {
            //emeter.printObisElement(obis, stderr);
            filter.consume(obis, timer);
            obis = emeter.getNextObisElement(obis);
        }

        valid = valid;
    }

    //SpeedwireDiscovery::sendRequest();
    //SpeedwireDiscovery::waitForResponse();

    return 0;
}