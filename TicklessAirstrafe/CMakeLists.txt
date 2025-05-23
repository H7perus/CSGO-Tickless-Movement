# TicklessAirstrafe project-specific CMakeLists.txt

# Define source files for this project
set(TICKLESS_SOURCES
    tickless_airstrafe.cpp
)

# Create the plugin using the shared function
add_source_engine_plugin(tickless_airstrafe SOURCES ${TICKLESS_SOURCES})