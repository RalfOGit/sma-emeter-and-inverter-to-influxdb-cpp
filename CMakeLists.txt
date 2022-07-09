cmake_minimum_required(VERSION 3.10)

project(emeter-grabber
  VERSION 0.1.0
  DESCRIPTION "SMA-Emeter to InfluxDB"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(USE_CURL OFF CACHE BOOL "Enable curl dependency")
set(USE_BOOST OFF CACHE BOOL "Enable boost dependency")

# This CMake configuration assumes the following directory layout
#
# - sma-emeter-grabber
#   - libspeedwire
#   - <relative build path>
# - libcurl (windows only; for linux it is installed in /usr/lib64)
#   - include
#   - lib
# - influxdb-cxx-fork
#   - include
#   - lib
#   - <relative install path>

# determine relative build and install paths
message("CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
message("CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR}")
message("CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")

string(REPLACE "${CMAKE_SOURCE_DIR}/" "" RELATIVE_BUILD_PATH "${CMAKE_BINARY_DIR}")
message("RELATIVE_BUILD_PATH ${RELATIVE_BUILD_PATH}")
string(REPLACE "${CMAKE_SOURCE_DIR}/" "" RELATIVE_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}")
message("RELATIVE_INSTALL_PATH ${RELATIVE_INSTALL_PATH}")

# path definitions for curl (adapt to your needs)
if (MSVC)
set(CURL_PROJECT_DIR "${CMAKE_SOURCE_DIR}/../libcurl")
set(CURL_LIBRARY_DIR "${CURL_PROJECT_DIR}/lib")
set(CURL_LIBRARY     "${CURL_LIBRARY_DIR}/libcurl.dll.a")
set(CURL_INCLUDE_DIR "${CURL_PROJECT_DIR}/include")
else()
set(CURL_LIBRARY_DIR "/usr/lib64")
set(CURL_LIBRARY     "/usr/lib64/libcurl.so")
set(CURL_INCLUDE_DIR "/usr/include/curl")
endif()
find_package(CURL REQUIRED MODULE)

# path definitions for InfluxDB (adapt to your needs)
set(INFLUX_DIR         "${CMAKE_SOURCE_DIR}/influxdb-cxx-fork")
set(INFLUX_INCLUDE_DIR "${INFLUX_DIR}/include")
set(INFLUX_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/${RELATIVE_BUILD_PATH}/lib")
add_subdirectory(influxdb-cxx-fork EXCLUDE_FROM_ALL)

# path definitions for libspeedwire (adapt to your needs)
set(SPEEDWIRE_DIR         "${CMAKE_SOURCE_DIR}/libspeedwire")
set(SPEEDWIRE_INCLUDE_DIR "${SPEEDWIRE_DIR}/include")
set(SPEEDWIRE_LIBRARY_DIR "${CMAKE_SOURCE_DIR}/${RELATIVE_BUILD_PATH}/libspeedwire")
add_subdirectory("libspeedwire")

# project sources and include path
set(PROJECT_SOURCES
    src/InfluxDBProducer.cpp
    src/PacketReceiver.cpp
    src/main.cpp
)
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# build configuration
include_directories(${PROJECT_INCLUDE_DIR} ${SPEEDWIRE_INCLUDE_DIR} ${INFLUX_INCLUDE_DIR})
link_directories(${SPEEDWIRE_LIBRARY_DIR} ${INFLUX_LIBRARY_DIR} ${CURL_LIBRARY_DIR})

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
add_dependencies(${PROJECT_NAME} speedwire InfluxDB)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIR} ${SPEEDWIRE_INCLUDE_DIR} ${INFLUX_INCLUDE_DIR})
target_include_directories(speedwire PUBLIC ${SPEEDWIRE_INCLUDE_DIR})
#target_include_directories(InfluxDB PUBLIC ${INFLUX_INCLUDE_DIR})

if (MSVC)
#target_link_libraries(${PROJECT_NAME} speedwire InfluxDB ${CURL_LIBRARY} ws2_32.lib Iphlpapi.lib)
target_link_libraries(${PROJECT_NAME} speedwire InfluxDB ws2_32.lib Iphlpapi.lib)
else()
target_link_libraries(${PROJECT_NAME} speedwire InfluxDB)
endif()

set_target_properties(${PROJECT_NAME}
    PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
