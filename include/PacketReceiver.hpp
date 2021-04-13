#ifndef __PACKETRECEIVER_HPP__
#define __PACKETRECEIVER_HPP__

#include <LocalHost.hpp>
#include <ObisFilter.hpp>
#include <DataProcessor.hpp>
#include <SpeedwireCommand.hpp>
#include <SpeedwireData.hpp>
#include <SpeedwireHeader.hpp>
#include <SpeedwireReceiveDispatcher.hpp>

/**
 *  Derived classes for speedwire packet receivers
 *  Each derived class is intended to receive speedwire packets belonging to a a single protocol as
 *  defined by its protocolID setting.
 */

 /**
  *  Speedwire packet receiver class for sma emeter packets
  */
class EmeterPacketReceiver : public EmeterPacketReceiverBase {
protected:
    ObisFilter& filter;

public:
    EmeterPacketReceiver(LocalHost& host, ObisFilter& filter);
    virtual void receive(SpeedwireHeader& packet, struct sockaddr& src);
};


/**
 *  Speedwire packet receiver class for sma inverter packets
 */
class InverterPacketReceiver : public InverterPacketReceiverBase {
protected:
    SpeedwireCommand& command;
    DataProcessor&    processor;
    SpeedwireDataMap& data_map;

public:
    InverterPacketReceiver(LocalHost& host, SpeedwireCommand& command, DataProcessor& processor, SpeedwireDataMap& data_map);
    virtual void receive(SpeedwireHeader& packet, struct sockaddr& src);
};

#endif
