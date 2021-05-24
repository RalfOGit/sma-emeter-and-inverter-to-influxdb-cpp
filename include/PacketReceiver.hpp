#ifndef __PACKETRECEIVER_HPP__
#define __PACKETRECEIVER_HPP__

#include <LocalHost.hpp>
#include <ObisFilter.hpp>
#include <AveragingProcessor.hpp>
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
class EmeterPacketReceiver : public libspeedwire::EmeterPacketReceiverBase {
protected:
    libspeedwire::ObisFilter& filter;

public:
    EmeterPacketReceiver(libspeedwire::LocalHost& host, libspeedwire::ObisFilter& filter);
    virtual void receive(libspeedwire::SpeedwireHeader& packet, struct sockaddr& src);
};


/**
 *  Speedwire packet receiver class for sma inverter packets
 */
class InverterPacketReceiver : public libspeedwire::InverterPacketReceiverBase {
protected:
    libspeedwire::SpeedwireCommand&   command;
    libspeedwire::AveragingProcessor& processor;
    libspeedwire::SpeedwireDataMap&   data_map;

public:
    InverterPacketReceiver(libspeedwire::LocalHost& host, libspeedwire::SpeedwireCommand& command, libspeedwire::AveragingProcessor& processor, libspeedwire::SpeedwireDataMap& data_map);
    virtual void receive(libspeedwire::SpeedwireHeader& packet, struct sockaddr& src);
};

#endif
