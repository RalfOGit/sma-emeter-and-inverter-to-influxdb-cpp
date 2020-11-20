#ifndef __SPEEDWIRESOCKET_HPP__
#define __SPEEDWIRESOCKET_HPP__

class SpeedwireSocket {

protected:
    static const char *multicast_group;
    static const int   multicast_port;
    static int fd;
    static SpeedwireSocket *instance;

private:
    SpeedwireSocket(void);
    ~SpeedwireSocket(void);
    int open(void);     // call getInstance() instead

public:
    static SpeedwireSocket *getInstance(void);
    int send(const void *const buff, const unsigned long size);
    int sendto(const void *const buff, const unsigned long size, const struct sockaddr_in &dest);
    int recv(void *buff, const unsigned long size);
    int recvfrom(void *buff, const unsigned long size, struct sockaddr_in &src);
    int close(void);
};

#endif