//
// Simple program to perform SMA speedwire device discovery
// https://www.sma.de/fileadmin/content/global/Partner/Documents/sma_developer/SpeedwireDD-TI-de-10.pdf
//

#ifdef _WIN32
    #include <Winsock2.h> // before Windows.h, else Winsock 1 conflict
    #include <Ws2tcpip.h> // needed for ip_mreq definition for multicast
    #include <Windows.h>
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h> // for sleep()
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <SpeedwireSocket.hpp>


const char      *SpeedwireSocket::multicast_group = "239.12.255.254";
const int        SpeedwireSocket::multicast_port  = 9522;
int              SpeedwireSocket::fd = -1;
SpeedwireSocket *SpeedwireSocket::instance = NULL;


SpeedwireSocket::SpeedwireSocket(void) {
    fd = -1;
}

SpeedwireSocket::~SpeedwireSocket(void) {
    if (fd >= 0) {
        close();
    }
}

// singleton
SpeedwireSocket *SpeedwireSocket::getInstance(void) {
    if (instance == NULL) {
        instance = new SpeedwireSocket();
        fd = -1;
    }
    if (fd < 0){
        fd = instance->open();
        if (fd < 0) {
            perror("cannot open socket");
            instance->close();
            delete instance;
            instance = NULL;
        }
    }
    return instance;
}

// open a socket to send and receive speedwire udp packets
int SpeedwireSocket::open(void) {

#ifdef _WIN32
    // initialize Windows Socket API with given VERSION.
    WSADATA wsaData;
    if (WSAStartup(0x0101, &wsaData)) {
        perror("WSAStartup failure");
        return -1;
    }
#endif

    // create what looks like an ordinary UDP socket
    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0) {
        perror("cannot create socket");
        return -1;
    }

    // allow multiple sockets to use the same PORT number
    u_int yes = 1;
    if ( setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0 ){
        perror("Reusing ADDR failed");
        return -1;
    }

    // allow packet info to be returned by recvmsg()
    u_int info = 1;
    if ( setsockopt(fd, SOL_SOCKET, IP_PKTINFO, &info, sizeof(info)) < 0 ){
        perror("Reusing ADDR failed");
        return -1;
    }

    // set up interface(s) to bind to
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(multicast_port);

    // bind to any available interface address
    if (bind(fd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        perror("bind");
        return -1;
    }

#if 1
    // use setsockopt() to request that the kernel joins a multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_group);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if ( setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0 ){
        perror("setsockopt");
        return -1;
    }

    // wait until multicast membership messages have been sent
    ::sleep(1);
#endif

    return fd;
}

// send udp packet to the speedwire multicast address
int SpeedwireSocket::send(const void *const buff, const size_t size) {

    // set up multicast destination address
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(multicast_group);
    dest_addr.sin_port = htons(multicast_port);

    // now just sendto() our destination
    int nbytes = sendto(buff, size, dest_addr);
    if (nbytes < 0) {
        perror("sendto failure");
    }
    return nbytes;
}

// send udp packet to the given destination address
int SpeedwireSocket::sendto(const void *const buff, const unsigned long size, const struct sockaddr_in &dest) {

#if 0
    // if there are multiple interfaces connected to this host, configure the outbound interface
    struct in_addr src_addr;
    memset(&src_addr, 0, sizeof(src_addr));
    //src_addr.s_addr = inet_addr("192.168.168.16");    // use specific interface
    src_addr.s_addr = htonl(INADDR_ANY);                // use default interface
    setsockopt(fd, IPPROTO_IP, IP_MULTICAST_IF, &src_addr, sizeof(src_addr));
#endif

    // now just sendto() our destination
    int nbytes = ::sendto(fd, buff, size, 0, (struct sockaddr*) &dest, sizeof(dest));
    if (nbytes < 0) {
        perror("sendto failure");
    }
    return nbytes;
}

// receive udp packet
int SpeedwireSocket::recv(void *buff, const unsigned long size) {

    // initialize the source address struct
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    // wait for packet data
    socklen_t addrlen = sizeof(addr);
    int nbytes = recvfrom(buff, size, addr);
    if (nbytes < 0) {
        perror("recvfrom failure");
    }

    return nbytes;
}

// receive udp packet and also provide the source address of the sender
int SpeedwireSocket::recvfrom(void *buff, const unsigned long size, struct sockaddr_in &src) {

    // wait for packet data
    socklen_t srclen = sizeof(src);
    int nbytes = ::recvfrom(fd, buff, size, 0, (struct sockaddr *) &src, &srclen);
    if (nbytes < 0) {
        perror("recvfrom failure");
    }

#if 0
    char str[256];
    strcpy(str, inet_ntoa(src.sin_addr));
    fprintf(stderr, "source address: %s", str);
#endif

    return nbytes;
}

// close the speedwire socket
int SpeedwireSocket::close(void) {
    int result = ::close(fd);
    fd = -1;

#ifdef _WIN32
    WSACleanup();
#endif
    return result;
}
