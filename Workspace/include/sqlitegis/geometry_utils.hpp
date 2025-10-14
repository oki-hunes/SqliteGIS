#pragma once

#include <sqlite3.h>

namespace sqlitegis {

// Register all utility functions with the database connection
int register_utility_functions(sqlite3* db, char** error_message);

} // namespace sqlitegis
