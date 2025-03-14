idf_component_register(
    SRCS "sim7600.c"
    INCLUDE_DIRS "include"
    REQUIRES driver uart power
)