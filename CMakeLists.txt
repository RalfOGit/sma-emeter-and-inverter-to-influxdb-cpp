project(emeter-grabber)

cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)

find_package(InfluxDB)

set(COMMON_SOURCES
    src/DataProcessor.cpp
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
)

add_library(${PROJECT_NAME} STATIC
    ${COMMON_SOURCES}
)

target_include_directories(${PROJECT_NAME}
PUBLIC
    include
)

if (InfluxDB_FOUND)
    add_executable(${PROJECT_NAME}-bin
        src/InfluxDBProducer.cpp
        src/main.cpp
    )
    target_link_libraries(${PROJECT_NAME}-bin
        ${PROJECT_NAME}
        InfluxData::InfluxDB
    )
    set_target_properties(${PROJECT_NAME}-bin
        PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
    )
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
endif (InfluxDB_FOUND)
