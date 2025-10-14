#pragma once

#include <sqlite3.h>

namespace sqlitegis {

// Registers all geometry related functions with the current database connection.
int register_geometry_functions(sqlite3* db, char** error_message);

}
