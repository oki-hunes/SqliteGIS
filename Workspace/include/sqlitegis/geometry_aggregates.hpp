#pragma once

#include <sqlite3.h>

namespace sqlitegis {

/**
 * @brief Register all aggregate functions with SQLite.
 * 
 * This function registers the following aggregate functions:
 * - ST_Collect: Collect geometries into Multi* or GeometryCollection
 * - ST_Union: Union geometries (topology merge)
 * - ST_ConvexHull_Agg: Compute convex hull of all geometries
 * - ST_Extent_Agg: Compute bounding box of all geometries
 * 
 * @param db SQLite database connection
 */
void register_aggregate_functions(sqlite3* db);

} // namespace sqlitegis
