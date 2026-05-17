# CMakeLists.txt
# Bitaxe G6 Brain component — v1.0.0-beta2
#
# Simple, clean component registration.
# All core logic lives in g6_brain.c (self-contained safety + RLS).

idf_component_register(
    SRCS 
        "g6_brain.c"

    INCLUDE_DIRS 
        "."

    REQUIRES 
        driver
)
