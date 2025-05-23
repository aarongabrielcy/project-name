idf_component_register(
    SRCS "processManager.c" "cellnetData.c" "gnssData.c" "cmdsManager.c" "trackerData.c" 
    INCLUDE_DIRS "include"
    REQUIRES utils sim serial nvs power common
)