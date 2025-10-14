#pragma once

#include <sqlite3.h>

namespace sqlitegis {

// Register all measurement functions with the database connection
int register_measure_functions(sqlite3* db, char** error_message);

} // namespace sqlitegis
