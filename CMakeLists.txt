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

# path definitions for InfluxDB (adapt to your needs)
add_subdirectory(influxdb-cxx-fork EXCLUDE_FROM_ALL)

# path definitions for libspeedwire (adapt to your needs)
add_subdirectory(libspeedwire EXCLUDE_FROM_ALL)

# project sources and include path
set(PROJECT_SOURCES
    src/InfluxDBProducer.cpp
    src/PacketReceiver.cpp
    src/HttpClient.cpp
    src/Url.cpp
    src/main.cpp
)
set(PROJECT_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)

# build configuration
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
add_dependencies(${PROJECT_NAME} speedwire InfluxDB)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIR} speedwire InfluxDB)

if (MSVC)
target_link_libraries(${PROJECT_NAME} speedwire InfluxDB ws2_32.lib Iphlpapi.lib)
else()
target_link_libraries(${PROJECT_NAME} speedwire InfluxDB)
endif()

set_target_properties(${PROJECT_NAME}
    PROPERTIES OUTPUT_NAME ${PROJECT_NAME}
)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})
