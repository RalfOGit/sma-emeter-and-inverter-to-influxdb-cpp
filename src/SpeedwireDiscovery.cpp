//
// Simple program to perform SMA speedwire device discovery
// https://www.sma.de/fileadmin/content/global/Partner/Documents/sma_developer/SpeedwireDD-TI-de-10.pdf
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SpeedwireDiscovery.hpp>
#include <SpeedwireSocket.hpp>


const char          *SpeedwireDiscovery::multicast_group     = "239.12.255.254";
const int            SpeedwireDiscovery::multicast_port      = 9522;
const unsigned char  SpeedwireDiscovery::multicast_request[] = {
    0x53, 0x4d, 0x41, 0x00, 0x00, 0x04, 0x02, 0xa0, 
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x20, 
    0x00, 0x00, 0x00, 0x00
};
unsigned char        SpeedwireDiscovery::multicast_response[];

bool SpeedwireDiscovery::sendRequest(void) {

    // get speedwire socket instance
    SpeedwireSocket *socket = SpeedwireSocket::getInstance();
    if (socket == NULL) {
        perror("cannot get socket instance");
        return false;
    }

    // now just sendto() our destination!
    int nbytes = socket->send(multicast_request, sizeof(multicast_request));
    if (nbytes < 0) {
        perror("sendto failure");
        return false;
    }
    return true;
}


int SpeedwireDiscovery::waitForResponse(void) {

    // get speedwire socket instance
    SpeedwireSocket *socket = SpeedwireSocket::getInstance();
    if (socket == NULL) {
        perror("cannot get socket instance");
        return false;
    }

    // now just wait for a response
    int nbytes = socket->recv(multicast_response, sizeof(multicast_response));
    if (nbytes < 0) {
        perror("recv failure");
        return -1;
    }
    return nbytes;
}