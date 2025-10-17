cmake_minimum_required(VERSION 3.20)

project(SqliteGIS_GDAL_Driver VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GDAL headers from third_party
set(GDAL_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/gdal-3.7.2/port;${CMAKE_CURRENT_SOURCE_DIR}/../third_party/gdal-3.7.2/gcore;${CMAKE_CURRENT_SOURCE_DIR}/../third_party/gdal-3.7.2/ogr;${CMAKE_CURRENT_SOURCE_DIR}/../third_party/gdal-3.7.2/ogr/ogrsf_frmts")

# Find SQLite3 via vcpkg
find_package(unofficial-sqlite3 CONFIG REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GDAL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../Workspace/include
)

# Source files
set(DRIVER_SOURCES
    src/ogrsqlitegisdriver.cpp
    src/ogrsqlitegisgdatasource.cpp
    src/ogrsqlitegisgislayer.cpp
)

# Header files
set(DRIVER_HEADERS
    include/ogrsqlitegisdriver.h
    include/ogrsqlitegisgdatasource.h
    include/ogrsqlitegisgislayer.h
)

# Create shared library (GDAL plugin)
add_library(ogr_SqliteGIS MODULE
    ${DRIVER_SOURCES}
    ${DRIVER_HEADERS}
)

# Link libraries
target_link_libraries(ogr_SqliteGIS
    PRIVATE
        unofficial::sqlite3::sqlite3
)

# GDAL driver plugins don't link against GDAL library
# They are loaded by GDAL at runtime and use GDAL's exported symbols

# Set output properties
set_target_properties(ogr_SqliteGIS PROPERTIES
    PREFIX ""
    OUTPUT_NAME "ogr_SqliteGIS"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/gdalplugins"
)

# Windows specific settings
if(WIN32)
    set_target_properties(ogr_SqliteGIS PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
        SUFFIX ".dll"
    )
endif()

# Installation
install(TARGETS ogr_SqliteGIS
    LIBRARY DESTINATION lib/gdalplugins
    RUNTIME DESTINATION lib/gdalplugins
)

# Print configuration summary
message(STATUS "========================================")
message(STATUS "SqliteGIS GDAL Driver Configuration")
message(STATUS "========================================")
message(STATUS "GDAL version: ${GDAL_VERSION}")
message(STATUS "GDAL include: ${GDAL_INCLUDE_DIRS}")
message(STATUS "GDAL library: ${GDAL_LIBRARIES}")
message(STATUS "========================================")
