//
// Simple program to perform SMA speedwire device discovery
// https://www.sma.de/fileadmin/content/global/Partner/Documents/sma_developer/SpeedwireDD-TI-de-10.pdf
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifdef _WIN32
#include <Winsock2.h>
#include <Ws2tcpip.h>
#define poll(a, b, c)  WSAPoll((a), (b), (c))
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SpeedwireByteEncoding.hpp>
#include <SpeedwireProtocol.hpp>
#include <SpeedwireEmeter.hpp>
#include <SpeedwireInverter.hpp>
#include <SpeedwireSocket.hpp>
#include <SpeedwireDiscovery.hpp>


// multicast device discovery request packet, according to SMA documentation
// however, this does not seem to be supported anymore with version 3.x devices
const unsigned char  SpeedwireDiscovery::multicast_request[] = {
    0x53, 0x4d, 0x41, 0x00, 0x00, 0x04, 0x02, 0xa0,     // sma signature, tag0
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x20,     // 0xffffffff group, 0x0000 length, 0x0020 "SMA Net ?", Version ?
    0x00, 0x00, 0x00, 0x00                              // 0x0000 protocol, 0x00 #long words, 0x00 ctrl
};


/**
 *  Constructor
 */
SpeedwireDiscovery::SpeedwireDiscovery(LocalHost& host) :
    localhost(host)
{
    // assemble multicast device discovery packet
    //unsigned char mreq[20];
    //SpeedwireProtocol mcast(mreq, sizeof(mreq));
    //mcast.setDefaultHeader(0xffffffff, 0, 0x0000);
    //mcast.setNetworkVersion(0x0020);
    //if (memcmp(mreq, multicast_request, sizeof(mreq) != 0)) {
    //    perror("diff");
    //}
}


/**
 *  Destructor - close all open sockets before clearing the device list
 */
SpeedwireDiscovery::~SpeedwireDiscovery(void) {
    speedwireDevices.clear();
}


/**
 *  Pre-register a device, i.e. just provide the ip address of the device
 */
bool SpeedwireDiscovery::preRegisterDevice(const std::string peer_ip_address) {
    SpeedwireInfo info(localhost);
    info.peer_ip_address = peer_ip_address;
    bool new_device = true;
    for (auto& device : speedwireDevices) {
        if (info.peer_ip_address == device.peer_ip_address) {
            new_device = false;
        }
    }
    if (new_device) {
        speedwireDevices.push_back(info);
    }
    return new_device;
}


/**
 *  Fully register a device, i.e. provide a full information data set
 */
bool SpeedwireDiscovery::registerDevice(const SpeedwireInfo& info) {
    bool new_device = true;
    for (auto& device : speedwireDevices) {
        if (device.isPreRegistered() && info.peer_ip_address == device.peer_ip_address) {
            device = info;
            new_device = false;
        }
        else if (device.isFullyRegistered() && info == device) {
            new_device = false;
        }
    }
    if (new_device) {
        speedwireDevices.push_back(info);
    }
    return new_device;
}


/**
 *  Unregister a device, i.e. delete it from the device list
 */
void SpeedwireDiscovery::unregisterDevice(const SpeedwireInfo& info) {
    for (std::vector<SpeedwireInfo>::iterator it = speedwireDevices.begin(); it != speedwireDevices.end(); ) {
        if (*it == info) {
            it = speedwireDevices.erase(it);
        } else {
            it++;
        }
    }
}


/**
 *  Get a vector of all devices
 */
const std::vector<SpeedwireInfo>& SpeedwireDiscovery::getDevices(void) const {
    return speedwireDevices;
}


/**
 *  Try to find SMA devices on the networks connected to this host
 */
int SpeedwireDiscovery::discoverDevices(void) {

    //int fd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    //struct sockaddr_in addr;
    //memset(&addr, 0, sizeof(addr));
    //addr.sin_family = AF_INET;
    //addr.sin_addr.s_addr = inet_addr("239.12.255.254");
    //addr.sin_port = htons(9522);
    //int nbytes = ::sendto(fd, (char*)multicast_request, 20, 0, (struct sockaddr*)&addr, sizeof(addr));
    //closesocket(fd);

    // get a list of all local ip addresses and open a socket for each of them
    const std::vector<std::string>& localIPs = localhost.getLocalIPAddresses();
    std::vector<SpeedwireSocket> sockets;
    for (auto &local_ip : localIPs) {

        // ignore ipv6 addresses, as speedwire is ipv4 only
        if (local_ip.find(':') != std::string::npos) continue;

        // open socket for local ip address
        SpeedwireSocket socket(localhost);
        if (socket.openSocket(local_ip) < 0) {
            perror("cannot open recv socket instance");
        } else {
            sockets.push_back(socket);
        }
    }

    // allocate a pollfd structure for each local ip address and populate it with the socket fds
    struct pollfd* fds = (struct pollfd*)malloc(sizeof(struct pollfd) * sockets.size());
    if (fds == NULL) {
        perror("malloc failure");
        return 0;
    }
    int j = 0;
    for (auto &socket : sockets) {
        fds[j++].fd = socket.getSocketFd();
    }

    // wait for incoming multicast packets
    const uint64_t maxWaitTimeInMillis = 2000u;
    uint64_t startTimeInMillis = localhost.getTickCount();
    size_t broadcast_counter = 0;
    size_t subnet_counter = 1;
    size_t socket_counter = 0;

    while ((localhost.getTickCount() - startTimeInMillis) < maxWaitTimeInMillis) {

        // prepare pollfd structure
        j = 0;
        for (auto& socket : sockets) {
            fds[j].events = POLLIN;
            fds[j++].revents = 0;
        }

        // send discovery request packet and update counters
        for (int i = 0; i < 10; ++i) {
            if (sendDiscoveryPackets(sockets, broadcast_counter, subnet_counter, socket_counter)) {
                startTimeInMillis = localhost.getTickCount();
            }
        }

        // wait for a packet on one of the configured sockets
        //fprintf(stdout, "poll() ...\n");
        if (poll(fds, (uint32_t)sockets.size(), 10) < 0) {     // timeout 10 ms
            perror("poll failed");
            break;
        }
        //fprintf(stdout, "... done\n");

        // determine the socket that received a packet
        j = 0;
        for (auto &socket : sockets) {
            if ((fds[j].revents & POLLIN) != 0) {

                // read packet data, analyze it and create a device information record
                recvDiscoveryPackets(sockets[j]);
            }
            j++;
        }
    }

    // close all sockets
    for (auto& socket : sockets) {
        socket.closeSocket();
    }
    free(fds);
    return 0;
}


/**
 *  Send discovery packets
 *  State machine implementing the following sequence of packets:
 *  - multicast speedwire discovery requests to all interfaces
 *  - unicast speedwire discovery requests to all hosts on the network
 */
bool SpeedwireDiscovery::sendDiscoveryPackets(const std::vector<SpeedwireSocket>& sockets, size_t& broadcast_counter, size_t& subnet_counter, size_t& socket_counter) {

    // sequentially first send multicast speedwire discovery requests
    if (broadcast_counter < sockets.size()) {
        //sockaddr_in sockaddr;
        //sockaddr.sin_family = AF_INET;
        //sockaddr.sin_addr = localhost.toInAddress("239.12.255.254");
        //sockaddr.sin_port = htons(9522);
        //int nbytes = sockets[broadcast_counter].sendto(multicast_request, sizeof(multicast_request), sockaddr);
        int nbytes = sockets[broadcast_counter].send(multicast_request, sizeof(multicast_request));
        //fprintf(stdout, "send broadcast discovery request to %s\n", localhost.toString(sockets[broadcast_counter].getSpeedwireMulticastIn4Address()).c_str());
        broadcast_counter++;
        return true;
    }
    return false;
}


/**
 *  Receive a discovery packet, analyze it and create a device information record
 */
bool SpeedwireDiscovery::recvDiscoveryPackets(const SpeedwireSocket& socket) {
    bool result = false;

    std::string peer_ip_address;
    char udp_packet[1600];
    int nbytes = 0;
    if (socket.isIpv4()) {
        struct sockaddr_in addr;
        nbytes = socket.recvfrom(udp_packet, sizeof(udp_packet), addr);
        addr.sin_port = 0;
        peer_ip_address = localhost.toString(addr);
    }
    else if (socket.isIpv6()) {
        struct sockaddr_in6 addr;
        nbytes = socket.recvfrom(udp_packet, sizeof(udp_packet), addr);
        addr.sin6_port = 0;
        peer_ip_address = localhost.toString(addr);
    }
    if (nbytes > 0) {
        SpeedwireProtocol protocol(udp_packet, nbytes);
        if (protocol.checkHeader()) {
            unsigned long payload_offset = protocol.getPayloadOffset();
            // check for emeter protocol
            if (protocol.getProtocolID() == 0x0001) {
                printf("received speedwire discovery response packet\n");
            }
            else if (protocol.isEmeterProtocolID()) {
                //SpeedwireSocket::hexdump(udp_packet, nbytes);
                SpeedwireEmeter emeter(udp_packet + payload_offset, nbytes - payload_offset);
                SpeedwireInfo info(localhost);
                info.susyID = emeter.getSusyID();
                info.serialNumber = emeter.getSerialNumber();
                info.deviceClass = "Emeter";
                info.deviceType = "Emeter";
                info.peer_ip_address = peer_ip_address;
                info.interface_ip_address = localhost.getMatchingLocalIPAddress(peer_ip_address);
                if (registerDevice(info)) {
                    printf("%s\n", info.toString().c_str());
                    result = true;
                }
            }
        }
    }
    return result;
}


/*====================================*/

/**
 *  Constructor
 *  Just initialize all member variables to a defined state; set susyId and serialNumber to 0
 */
SpeedwireInfo::SpeedwireInfo(const LocalHost& localhost) : susyID(0), serialNumber(0) {}


/**
 *  Convert speedwire information to a single line string
 */
std::string SpeedwireInfo::toString(void) const {
    char buffer[256] = { 0 };
    snprintf(buffer, sizeof(buffer), "SusyID %u  Serial %u  Class %s  Type %s  SWVersion %s  IP %s  IF %s", 
             susyID, serialNumber, deviceClass.c_str(), deviceType.c_str(), softwareVersion.c_str(), peer_ip_address.c_str(), interface_ip_address.c_str());
    return std::string(buffer);
}


/**
 *  Compare two instances; assume that if SusyID, Serial and IP is the same, it is the same device
 */
bool SpeedwireInfo::operator==(const SpeedwireInfo& rhs) const {
    return (susyID == rhs.susyID && serialNumber == rhs.serialNumber && peer_ip_address == rhs.peer_ip_address);
}


/*
 *  Check if this instance is just pre-registered, i.e a device ip address is given
 */
bool SpeedwireInfo::isPreRegistered(void) const {
    return (peer_ip_address.length() > 0 && susyID == 0 && serialNumber == 0);
}


/*
 *  Check if this instance is fully registered, i.e all device information is given
 */
bool SpeedwireInfo::isFullyRegistered(void) const {
    return (susyID != 0 && serialNumber != 0 && deviceClass.length() > 0 && deviceType.length() > 0 && 
            /*softwareVersion.length() > 0 &&*/ peer_ip_address.length() > 0 && interface_ip_address.length() > 0);
}
