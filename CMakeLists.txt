project(emeter-grabber)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)

# TODO: make InfluxDB optional
find_package(InfluxDB REQUIRED)

set(COMMON_SOURCES
    src/DataProcessor.cpp
    src/InfluxDBProducer.cpp
    src/LocalHost.cpp
    src/Measurement.cpp
    src/ObisData.cpp
    src/ObisFilter.cpp
    src/SpeedwireByteEncoding.cpp
    src/SpeedwireCommand.cpp
    src/SpeedwireData.cpp
    src/SpeedwireDiscovery.cpp
    src/SpeedwireEmeterProtocol.cpp
    src/SpeedwireHeader.cpp
    src/SpeedwireInverterProtocol.cpp
    src/SpeedwireSocket.cpp
    src/SpeedwireSocketFactory.cpp
    src/SpeedwireSocketSimple.cpp
    src/main.cpp
)

add_executable(${PROJECT_NAME}
    ${COMMON_SOURCES}
)

if (InfluxDB_FOUND)
    target_link_libraries(${PROJECT_NAME} InfluxData::InfluxDB)
endif (InfluxDB_FOUND)

target_include_directories(${PROJECT_NAME}
PRIVATE
    include
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
