#pragma once

#include <sqlite3.h>

namespace sqlitegis {

// Register all constructor functions with the database connection
int register_constructor_functions(sqlite3* db, char** error_message);

} // namespace sqlitegis
