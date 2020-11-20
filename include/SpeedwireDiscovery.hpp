#ifndef __SPEEDWIREDISCOVERY_HPP__

class SpeedwireDiscovery {

protected:
    static const char         *multicast_group;
    static const int           multicast_port;
    static const unsigned char multicast_request[20];
    static unsigned char       multicast_response[20];

public:
    static bool sendRequest(void);
    static int  waitForResponse(void);
};

#endif