#pragma once

#include <sqlite3.h>

namespace sqlitegis {

// Register all accessor functions with the database connection
int register_accessor_functions(sqlite3* db, char** error_message);

} // namespace sqlitegis
