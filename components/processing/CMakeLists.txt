idf_component_register(
    SRCS "cmdsManager.c" "serviceInfo.c" "additionalData.c" "moduleData.c" "trackerData.c" 
    INCLUDE_DIRS "include"
    REQUIRES utils sim serial nvs power
)