#pragma once

#include <sqlite3.h>

namespace sqlitegis {

/**
 * @brief Register all coordinate transformation functions with SQLite.
 * 
 * This function registers the following functions:
 * - ST_Transform: Transform geometry to different coordinate reference system
 * - ST_SetSRID: Set SRID without coordinate transformation
 * - PROJ_Version: Get PROJ library version
 * - PROJ_GetCRSInfo: Get CRS information for given SRID
 * 
 * @param db SQLite database connection
 */
void register_transform_functions(sqlite3* db);

} // namespace sqlitegis
