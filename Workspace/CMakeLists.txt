cmake_minimum_required(VERSION 3.20)

project(SqliteGIS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Boost REQUIRED)
find_package(SQLite3 REQUIRED)

# Find PROJ library for coordinate transformations
find_package(PROJ QUIET)
if(NOT PROJ_FOUND)
    # Try manual detection
    find_path(PROJ_INCLUDE_DIR proj.h
        PATHS /usr/local/include /usr/include
    )
    find_library(PROJ_LIBRARY NAMES proj
        PATHS /usr/local/lib /usr/lib
    )
    if(PROJ_INCLUDE_DIR AND PROJ_LIBRARY)
        set(PROJ_FOUND TRUE)
        set(PROJ_INCLUDE_DIRS ${PROJ_INCLUDE_DIR})
        set(PROJ_LIBRARIES ${PROJ_LIBRARY})
        message(STATUS "Found PROJ (manual): ${PROJ_LIBRARY}")
    else()
        message(WARNING "PROJ not found - coordinate transformation will be disabled")
    endif()
endif()

add_library(sqlitegis SHARED
    src/sqlitegis_extension.cpp
    src/geometry_types.cpp
    src/geometry_constructors.cpp
    src/geometry_accessors.cpp
    src/geometry_measures.cpp
    src/geometry_relations.cpp
    src/geometry_operations.cpp
    src/geometry_utils.cpp
    src/geometry_bbox.cpp
    src/geometry_aggregates.cpp
    src/geometry_transform.cpp
)

set_target_properties(sqlitegis PROPERTIES
    OUTPUT_NAME "sqlitegis"
    PREFIX ""
    WINDOWS_EXPORT_ALL_SYMBOLS ON
)

target_include_directories(sqlitegis
    PRIVATE
        include
        ${SQLite3_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}
)

if(PROJ_FOUND)
    target_include_directories(sqlitegis PRIVATE ${PROJ_INCLUDE_DIRS})
    target_compile_definitions(sqlitegis PRIVATE HAVE_PROJ)
endif()

target_link_libraries(sqlitegis
    PRIVATE
        ${SQLite3_LIBRARIES}
)

if(PROJ_FOUND)
    target_link_libraries(sqlitegis PRIVATE ${PROJ_LIBRARIES})
endif()

include(GNUInstallDirs)

install(TARGETS sqlitegis
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
