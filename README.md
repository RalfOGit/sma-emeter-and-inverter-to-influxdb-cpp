# SMA-Emeter and SMA-Inverter to InfluxDB 
A C++ executable to grab measurements from SMA-Emeters(TM) and SMA-Inverters(TM) and pass them to InfluxDB(TM).

SMA-Emeters(TM) send out udp packets including electrical power and energy measurements at intervals of 1000ms. Each udp packet is 600 bytes long and its format is specified in an publicly available specification document provided by the manufacturer.

SMA-Inverters(TM) provide electrical power production data on request. There is no official specification document provided by the manufactuer, but there are a number of projects on the internet describing what is publicly known today; please refer to the list given at the end of this document.

I am using it to monitor the electrical energy flow at the grid connection point of my house and also to measure the photovoltaic energy production of my small solar plant. Measurement data is persisted inside InfluxDB(TM) and visualized using Grafana(TM).

The executable starts by discovering all SMA-Emeter(TM) and SMA-Inverter(TM) devices on all ip networks connected to the host running the executable. For discovery, it listens to inbound datagrams from the SMA Speedwire(TM) multicast group 239.12.255.254 at port 9522 an all connected interfaces, and starts a unicast scan within all connected local subnets on port 9522. 

Afterwards it starts an infinite main loop. 

Within the main loop it listens to the above mentioned SMA Speedwire(TM) multicast group and receives udp packets. You can configure a filter to extract just those measurements that you are interested in. Finally, it passes the filtered measurements to InfluxDB(TM). In order not to overwhelm the InfluxDB with too many measurements, it includes an averaging mechanism if needed. 

Also within the main loop, it queries all discovered inverters at predefined regular intervals. This is done by sending udp unicast queries to each inverter and analyzing the corresponding udp unicast responses. It also passes these measurements to InfluxDB(TM). The same averaging mechanism can be applied if needed.

The software comes as is. No warrantees whatsoever are given and no responsibility is assumed in case of failure. There is neither a GUI nor a configuration file. Configurations must be tweaked by modifying main.cpp. A number of obis definitions are given, some of them are commented out, since I do not need them. A few inverter command definitions are given. InfluxDB specific configurations are found in DataProcessor.cpp. 

The code comes with a decent SMA Speedwire(TM) library implementation. It implements a full parser for the sma header and the emeter datagram structure, including obis filtering. In addition, it implements some parsing functionality for inverter query and response datagrams.

The code relies on the InfluxDB(TM) C++ client library written by Adam Wegrzynek https://github.com/awegrzyn/influxdb-cxx.

Further information regarding the SMA-Inverter(TM) datagrams can be found in various places on the internet:
- https://github.com/SBFspot/SBFspot
- https://github.com/Rincewind76/SMAInverter
- https://github.com/ardexa/sma-rs485-inverters
- https://github.com/dgibson/python-smadata2/blob/master/doc/protocol.txt
- https://github.com/peterbarker/python-smadata2

The code has been tested against the following environment:
- OS: CentOS 8(TM),   IDE: VSCode (TM)
- OS: Windows 10(TM), IDE: Visual Studio Community Edition 2017 (TM)

You will need to open your local firewall for udp packets on port 9522.

It comes with a simple makefile supporting the following commands:
- make clean
- make debug
- make release

![InfluxDB Grafana View #1](./screenshot#1.png?raw=true)
