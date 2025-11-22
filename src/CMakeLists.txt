cmake_minimum_required(VERSION 3.10)
project(dialpad_driver)

# Create the main dialpad debugging tool
add_executable(dialpad-debug dialpad-debug.cpp)

# Link against the hidpp library
target_link_libraries(dialpad-debug
    hidpp
    Threads::Threads
)

# Create the device finder tool
add_executable(find-dialpad-events find-dialpad-events.cpp)

target_link_libraries(find-dialpad-events
    hidpp
    Threads::Threads
)

# Create the input event monitor (for Linux input subsystem)
add_executable(input-event-monitor input-event-monitor.cpp)

target_link_libraries(input-event-monitor
    Threads::Threads
)

# Create the volume controller (uses pactl for PipeWire/PulseAudio)
add_executable(dialpad-volume dialpad-volume.cpp)

target_link_libraries(dialpad-volume
    Threads::Threads
)

# Install the binaries
install(TARGETS dialpad-debug find-dialpad-events input-event-monitor dialpad-volume
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
