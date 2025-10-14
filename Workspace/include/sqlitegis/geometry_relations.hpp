#pragma once

#include <sqlite3.h>

namespace sqlitegis {

// Register all spatial relationship functions with the database connection
int register_relation_functions(sqlite3* db, char** error_message);

} // namespace sqlitegis
