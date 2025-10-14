#pragma once

#include <sqlite3.h>

namespace sqlitegis {

// Register all spatial operation functions with the database connection
int register_operation_functions(sqlite3* db, char** error_message);

} // namespace sqlitegis
