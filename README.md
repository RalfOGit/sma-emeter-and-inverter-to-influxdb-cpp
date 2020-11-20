# sma-emeter-grabber
A small executable to grab measurements from an SMA-Emeter(TM) and pass them to InfluxDB

SMA Emeters(TM) send out udp packets including electrical power and energy measurements at intervals of 1000m. Each udp packet is 600 bytes long and its format is specified in an openly available specification document (search the web and you will find it).

I am using it to monitor the electrical energy consumption at the grid connection point of my house.

This small executable listens to the corresponding SMA Speedwire(TM) multicast group 239.12.255.254 at port 9522 and receives udp packets. You can configure a filter to extract just those measurements that you are interested in. Finally, it passes the filtered measurements to InfluxDB. In order not to overwhelm the InfluxDB with too many measurements, it includes an averaging mechanism if needed.

The software comes as is. No warrantees whatsoever are given and no responsibility is assumed in case of failure. There is neither a GUI nor a configuration file. Configurations must be tweaked by modifying main.cpp. InfluxDB specific configurations are found in ObisProcessor.cpp.

The code relies on the InfluxDB C++ client library written by Adam Wegrzynek https://github.com/awegrzyn/influxdb-cxx.

It has been written on CentOS 8, using VSCode and it comes with a simple makefile supporting the following commands:
- make clean
- make debug
- make release
