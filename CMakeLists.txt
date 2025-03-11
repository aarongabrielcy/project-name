# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(project-name)

# Cargar la versión desde sdkconfig.h
idf_build_get_property(sdkconfig_dir SDKCONFIG_DIR)
set(PROJECT_VERSION_FILE "${sdkconfig_dir}/sdkconfig.h")

# Leer la versión de CONFIG_PROJECT_VERSION en sdkconfig.h
if(EXISTS ${PROJECT_VERSION_FILE})
    file(STRINGS ${PROJECT_VERSION_FILE} VERSION_LINE REGEX "CONFIG_PROJECT_VERSION=\".*\"")
    string(REGEX MATCH "CONFIG_PROJECT_VERSION=\"([^\"]+)\"" _ ${VERSION_LINE})
    set(PROJECT_VER ${CMAKE_MATCH_1})
endif()

# Imprimir la versión en CMake (opcional)
message(STATUS "Project version: ${PROJECT_VER}")