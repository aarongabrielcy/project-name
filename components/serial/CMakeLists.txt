idf_component_register(
    SRCS "monitor.c"
    INCLUDE_DIRS "include"
    REQUIRES power sim uart net
)