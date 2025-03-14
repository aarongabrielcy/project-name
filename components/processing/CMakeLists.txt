idf_component_register(
    SRCS "serviceInfo.c" "additionalData.c" "moduleData.c" "trackerData.c"
    INCLUDE_DIRS "include"
    REQUIRES utils sim
)