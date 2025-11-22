cmake_minimum_required(VERSION 3.10)

# Library source files
set(LOGILINUX_SOURCES
    src/core/library.cpp
    src/core/device_manager.cpp
    src/core/input_monitor.cpp
    src/devices/dialpad_device.cpp
    src/devices/mx_keypad_device.cpp
)

# Create shared library
add_library(logilinux SHARED ${LOGILINUX_SOURCES})

# Set C++17 standard
target_compile_features(logilinux PUBLIC cxx_std_17)

# Include directories
target_include_directories(logilinux
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link pthread for threading support
target_link_libraries(logilinux PRIVATE pthread)

# Set library version
set_target_properties(logilinux PROPERTIES
    VERSION 0.1.0
    SOVERSION 0
    OUTPUT_NAME "logilinux"
)

# Installation rules
install(TARGETS logilinux
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install headers
install(DIRECTORY include/logilinux
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Generate pkg-config file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/logilinux.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/logilinux.pc
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/logilinux.pc
    DESTINATION lib/pkgconfig
)
