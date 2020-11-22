# SMA-Emeter to InfluxDB 
A C++ executable to grab measurements from an SMA-Emeter(TM) and pass them to InfluxDB(TM)

SMA Emeters(TM) send out udp packets including electrical power and energy measurements at intervals of 1000ms. Each udp packet is 600 bytes long and its format is specified in an openly available specification document (search the web and you will find it).

I am using it to monitor the electrical energy consumption at the grid connection point of my house. Measurement data is persisted inside InfluxDB and visualized using Grafana.

This executable listens to the corresponding SMA Speedwire(TM) multicast group 239.12.255.254 at port 9522 and receives udp packets. You can configure a filter to extract just those measurements that you are interested in. Finally, it passes the filtered measurements to InfluxDB(TM). In order not to overwhelm the InfluxDB with too many measurements, it includes an averaging mechanism if needed. The code implements a full parser for the sma header and the emeter datagram structure, including obis filtering.

The software comes as is. No warrantees whatsoever are given and no responsibility is assumed in case of failure. There is neither a GUI nor a configuration file. Configurations must be tweaked by modifying main.cpp. A number of obis definitions are given, some of them are commented out, since I do not need them. InfluxDB specific configurations are found in ObisProcessor.cpp. You can use the code as is or as a starting point for your own project.

The code relies on the InfluxDB(TM) C++ client library written by Adam Wegrzynek https://github.com/awegrzyn/influxdb-cxx.

The code has been tested with the following environment:
- OS: CentOS 8(TM),   IDE: VSCode (TM)
- OS: Windows 10(TM), IDE: Visual Studio Community Edition 2017 (TM)

You will need to open your local firewall for udp packets on port 9522.

It comes with a simple makefile supporting the following commands:
- make clean
- make debug
- make release
