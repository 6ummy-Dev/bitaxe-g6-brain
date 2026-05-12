idf_component_register(
    SRCS "g6_brain.c"
    INCLUDE_DIRS "."
    REQUIRES driver
)

# Note: g6_safety.c is no longer required (all safety logic is integrated in g6_brain.c)
# You may safely delete g6_safety.c and g6_safety.h for a minimal build.
