#include "sqlitegis/geometry_constructors.hpp"
#include "sqlitegis/geometry_accessors.hpp"
#include "sqlitegis/geometry_measures.hpp"
#include "sqlitegis/geometry_relations.hpp"
#include "sqlitegis/geometry_operations.hpp"
#include "sqlitegis/geometry_utils.hpp"
#include "sqlitegis/geometry_bbox.hpp"
#include "sqlitegis/geometry_aggregates.hpp"
#include "sqlitegis/geometry_transform.hpp"

#include <sqlite3ext.h>

// NOLINTNEXTLINE - SQLite requires these globals for loadable extensions.
SQLITE_EXTENSION_INIT1

extern "C" {

int sqlite3_sqlitegis_init(sqlite3* db, char** error_message, const sqlite3_api_routines* api) {
    SQLITE_EXTENSION_INIT2(api);
    
    int rc;
    
    // Register constructor functions
    rc = sqlitegis::register_constructor_functions(db, error_message);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    // Register accessor functions
    rc = sqlitegis::register_accessor_functions(db, error_message);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    // Register measurement functions
    rc = sqlitegis::register_measure_functions(db, error_message);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    // Register spatial relationship functions
    rc = sqlitegis::register_relation_functions(db, error_message);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    // Register spatial operation functions
    rc = sqlitegis::register_operation_functions(db, error_message);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    // Register utility functions
    rc = sqlitegis::register_utility_functions(db, error_message);
    if (rc != SQLITE_OK) {
        return rc;
    }
    
    // Register bounding box functions
    sqlitegis::register_bbox_functions(db);
    
    // Register aggregate functions
    sqlitegis::register_aggregate_functions(db);
    
    // Register transformation functions
    sqlitegis::register_transform_functions(db);
    
    return SQLITE_OK;
}

int sqlite3_extension_init(sqlite3* db, char** error_message, const sqlite3_api_routines* api) {
    return sqlite3_sqlitegis_init(db, error_message, api);
}

} // extern "C"

