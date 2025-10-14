#include "sqlitegis/geometry_aggregates.hpp"
#include "sqlitegis/geometry_types.hpp"
#include "sqlitegis/geometry_utils.hpp"

#include <boost/geometry/algorithms/union.hpp>
#include <boost/geometry/algorithms/convex_hull.hpp>

#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <limits>

namespace sqlitegis {

// Aggregate context for collecting geometries
struct CollectContext {
    std::vector<GeometryWrapper> geometries;
    int srid = -1;
    bool has_error = false;
    std::string error_message;
};

// =============================================================================
// ST_Collect: Collect geometries into Multi* or GeometryCollection
// =============================================================================

void st_collect_step(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) return;
    
    // Get or create aggregate context
    auto* collect_ctx = static_cast<CollectContext*>(
        sqlite3_aggregate_context(ctx, sizeof(CollectContext))
    );
    
    if (!collect_ctx) {
        sqlite3_result_error_nomem(ctx);
        return;
    }
    
    // Initialize context on first call
    if (collect_ctx->geometries.empty() && collect_ctx->srid == -1) {
        new (collect_ctx) CollectContext();
    }
    
    // Skip NULL values
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }
    
    const char* ewkt = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    if (!ewkt) return;
    
    auto geom_opt = GeometryWrapper::from_ewkt(ewkt);
    if (!geom_opt) {
        collect_ctx->has_error = true;
        collect_ctx->error_message = "Invalid geometry in ST_Collect";
        return;
    }
    
    auto& geom = *geom_opt;
    
    // Check SRID consistency
    if (collect_ctx->srid == -1) {
        collect_ctx->srid = geom.srid();
    } else if (collect_ctx->srid != geom.srid()) {
        collect_ctx->has_error = true;
        collect_ctx->error_message = "Mixed SRIDs in ST_Collect";
        return;
    }
    
    collect_ctx->geometries.push_back(std::move(geom));
}

void st_collect_final(sqlite3_context* ctx) {
    auto* collect_ctx = static_cast<CollectContext*>(
        sqlite3_aggregate_context(ctx, 0)
    );
    
    if (!collect_ctx || collect_ctx->geometries.empty()) {
        sqlite3_result_null(ctx);
        return;
    }
    
    if (collect_ctx->has_error) {
        sqlite3_result_error(ctx, collect_ctx->error_message.c_str(), -1);
        return;
    }
    
    // Check if all geometries are the same type
    auto first_type = collect_ctx->geometries[0].geometry_type();
    bool same_type = true;
    
    for (const auto& geom : collect_ctx->geometries) {
        if (geom.geometry_type() != first_type) {
            same_type = false;
            break;
        }
    }
    
    std::ostringstream result;
    
    // Build appropriate Multi* type or GeometryCollection
    if (same_type) {
        // Same type -> create Multi* geometry
        switch (first_type) {
            case GeometryType::Point: {
                result << "MULTIPOINT(";
                for (size_t i = 0; i < collect_ctx->geometries.size(); ++i) {
                    if (i > 0) result << ", ";
                    auto wkt = collect_ctx->geometries[i].to_wkt();
                    // Extract coordinates from "POINT(x y)" or "POINT Z (x y z)"
                    auto start = wkt.find('(');
                    auto end = wkt.rfind(')');
                    if (start != std::string::npos && end != std::string::npos) {
                        result << wkt.substr(start + 1, end - start - 1);
                    }
                }
                result << ")";
                break;
            }
            case GeometryType::LineString: {
                result << "MULTILINESTRING(";
                for (size_t i = 0; i < collect_ctx->geometries.size(); ++i) {
                    if (i > 0) result << ", ";
                    auto wkt = collect_ctx->geometries[i].to_wkt();
                    auto start = wkt.find('(');
                    auto end = wkt.rfind(')');
                    if (start != std::string::npos && end != std::string::npos) {
                        result << "(" << wkt.substr(start + 1, end - start - 1) << ")";
                    }
                }
                result << ")";
                break;
            }
            case GeometryType::Polygon: {
                result << "MULTIPOLYGON(";
                for (size_t i = 0; i < collect_ctx->geometries.size(); ++i) {
                    if (i > 0) result << ", ";
                    auto wkt = collect_ctx->geometries[i].to_wkt();
                    auto start = wkt.find('(');
                    auto end = wkt.rfind(')');
                    if (start != std::string::npos && end != std::string::npos) {
                        result << "(" << wkt.substr(start + 1, end - start - 1) << ")";
                    }
                }
                result << ")";
                break;
            }
            default:
                // Already Multi* type, just collect into GEOMETRYCOLLECTION
                same_type = false;
                break;
        }
    }
    
    if (!same_type) {
        // Mixed types or already Multi* -> create GEOMETRYCOLLECTION
        result << "GEOMETRYCOLLECTION(";
        for (size_t i = 0; i < collect_ctx->geometries.size(); ++i) {
            if (i > 0) result << ", ";
            result << collect_ctx->geometries[i].to_wkt();
        }
        result << ")";
    }
    
    // Create result with SRID
    GeometryWrapper result_geom(result.str(), collect_ctx->srid);
    std::string ewkt = result_geom.to_ewkt();
    
    sqlite3_result_text(ctx, ewkt.c_str(), -1, SQLITE_TRANSIENT);
}

// =============================================================================
// ST_Union: Union geometries (topology merge)
// =============================================================================

void st_union_step(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) return;
    
    auto* collect_ctx = static_cast<CollectContext*>(
        sqlite3_aggregate_context(ctx, sizeof(CollectContext))
    );
    
    if (!collect_ctx) {
        sqlite3_result_error_nomem(ctx);
        return;
    }
    
    if (collect_ctx->geometries.empty() && collect_ctx->srid == -1) {
        new (collect_ctx) CollectContext();
    }
    
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }
    
    const char* ewkt = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    if (!ewkt) return;
    
    auto geom_opt = GeometryWrapper::from_ewkt(ewkt);
    if (!geom_opt) {
        collect_ctx->has_error = true;
        collect_ctx->error_message = "Invalid geometry in ST_Union";
        return;
    }
    
    auto& geom = *geom_opt;
    
    if (collect_ctx->srid == -1) {
        collect_ctx->srid = geom.srid();
    } else if (collect_ctx->srid != geom.srid()) {
        collect_ctx->has_error = true;
        collect_ctx->error_message = "Mixed SRIDs in ST_Union";
        return;
    }
    
    collect_ctx->geometries.push_back(std::move(geom));
}

void st_union_final(sqlite3_context* ctx) {
    auto* collect_ctx = static_cast<CollectContext*>(
        sqlite3_aggregate_context(ctx, 0)
    );
    
    if (!collect_ctx || collect_ctx->geometries.empty()) {
        sqlite3_result_null(ctx);
        return;
    }
    
    if (collect_ctx->has_error) {
        sqlite3_result_error(ctx, collect_ctx->error_message.c_str(), -1);
        return;
    }
    
    // For now, we'll use a simplified approach: collect into MultiPolygon
    // Full union requires more complex topology processing
    // This is a placeholder that needs proper boost::geometry::union_ implementation
    
    // Get first geometry variant
    auto first_var = collect_ctx->geometries[0].as_variant();
    if (!first_var) {
        sqlite3_result_error(ctx, "Cannot parse geometry in ST_Union", -1);
        return;
    }
    
    // For single geometry, just return it
    if (collect_ctx->geometries.size() == 1) {
        std::string ewkt = collect_ctx->geometries[0].to_ewkt();
        sqlite3_result_text(ctx, ewkt.c_str(), -1, SQLITE_TRANSIENT);
        return;
    }
    
    // For multiple geometries, perform union operations
    std::vector<polygon_t> output;
    
    try {
        // Process each geometry pair
        for (size_t i = 0; i < collect_ctx->geometries.size(); ++i) {
            auto var = collect_ctx->geometries[i].as_variant();
            if (!var) continue;
            
            std::visit([&output](auto&& geom) {
                using T = std::decay_t<decltype(geom)>;
                if constexpr (std::is_same_v<T, polygon_t>) {
                    if (output.empty()) {
                        output.push_back(geom);
                    } else {
                        std::vector<polygon_t> union_result;
                        boost::geometry::union_(output[0], geom, union_result);
                        output = std::move(union_result);
                    }
                }
                // Other types would need conversion or different handling
            }, *var);
        }
        
        // Build result
        if (output.empty()) {
            sqlite3_result_error(ctx, "Union produced no result", -1);
            return;
        }
        
        std::ostringstream result_wkt;
        if (output.size() == 1) {
            result_wkt << boost::geometry::wkt(output[0]);
        } else {
            multipolygon_t multi;
            for (const auto& poly : output) {
                multi.push_back(poly);
            }
            result_wkt << boost::geometry::wkt(multi);
        }
        
        GeometryWrapper result_geom(result_wkt.str(), collect_ctx->srid);
        std::string ewkt = result_geom.to_ewkt();
        sqlite3_result_text(ctx, ewkt.c_str(), -1, SQLITE_TRANSIENT);
        
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("ST_Union error: " + std::string(e.what())).c_str(), -1);
    }
}

// =============================================================================
// ST_ConvexHull_Agg: Compute convex hull of all geometries
// =============================================================================

void st_convexhull_agg_step(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) return;
    
    auto* collect_ctx = static_cast<CollectContext*>(
        sqlite3_aggregate_context(ctx, sizeof(CollectContext))
    );
    
    if (!collect_ctx) {
        sqlite3_result_error_nomem(ctx);
        return;
    }
    
    if (collect_ctx->geometries.empty() && collect_ctx->srid == -1) {
        new (collect_ctx) CollectContext();
    }
    
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }
    
    const char* ewkt = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    if (!ewkt) return;
    
    auto geom_opt = GeometryWrapper::from_ewkt(ewkt);
    if (!geom_opt) {
        collect_ctx->has_error = true;
        collect_ctx->error_message = "Invalid geometry in ST_ConvexHull_Agg";
        return;
    }
    
    auto& geom = *geom_opt;
    
    if (collect_ctx->srid == -1) {
        collect_ctx->srid = geom.srid();
    } else if (collect_ctx->srid != geom.srid()) {
        collect_ctx->has_error = true;
        collect_ctx->error_message = "Mixed SRIDs in ST_ConvexHull_Agg";
        return;
    }
    
    collect_ctx->geometries.push_back(std::move(geom));
}

void st_convexhull_agg_final(sqlite3_context* ctx) {
    auto* collect_ctx = static_cast<CollectContext*>(
        sqlite3_aggregate_context(ctx, 0)
    );
    
    if (!collect_ctx || collect_ctx->geometries.empty()) {
        sqlite3_result_null(ctx);
        return;
    }
    
    if (collect_ctx->has_error) {
        sqlite3_result_error(ctx, collect_ctx->error_message.c_str(), -1);
        return;
    }
    
    try {
        // Collect all points from all geometries
        multipoint_t all_points;
        
        for (const auto& geom : collect_ctx->geometries) {
            auto var = geom.as_variant();
            if (!var) continue;
            
            std::visit([&all_points](auto&& g) {
                using T = std::decay_t<decltype(g)>;
                if constexpr (std::is_same_v<T, point_t>) {
                    all_points.push_back(g);
                } else if constexpr (std::is_same_v<T, linestring_t>) {
                    for (const auto& pt : g) {
                        all_points.push_back(pt);
                    }
                } else if constexpr (std::is_same_v<T, polygon_t>) {
                    for (const auto& pt : g.outer()) {
                        all_points.push_back(pt);
                    }
                } else if constexpr (std::is_same_v<T, multipoint_t>) {
                    for (const auto& pt : g) {
                        all_points.push_back(pt);
                    }
                }
                // Add more types as needed
            }, *var);
        }
        
        // Compute convex hull
        polygon_t hull;
        boost::geometry::convex_hull(all_points, hull);
        
        std::ostringstream result_wkt;
        result_wkt << boost::geometry::wkt(hull);
        
        GeometryWrapper result_geom(result_wkt.str(), collect_ctx->srid);
        std::string ewkt = result_geom.to_ewkt();
        sqlite3_result_text(ctx, ewkt.c_str(), -1, SQLITE_TRANSIENT);
        
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("ST_ConvexHull_Agg error: " + std::string(e.what())).c_str(), -1);
    }
}

// =============================================================================
// ST_Extent_Agg: Compute bounding box of all geometries
// =============================================================================

struct ExtentContext {
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();
    bool has_data = false;
};

void st_extent_agg_step(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) return;
    
    auto* extent_ctx = static_cast<ExtentContext*>(
        sqlite3_aggregate_context(ctx, sizeof(ExtentContext))
    );
    
    if (!extent_ctx) {
        sqlite3_result_error_nomem(ctx);
        return;
    }
    
    if (!extent_ctx->has_data) {
        new (extent_ctx) ExtentContext();
    }
    
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        return;
    }
    
    const char* ewkt = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    if (!ewkt) return;
    
    auto geom_opt = GeometryWrapper::from_ewkt(ewkt);
    if (!geom_opt) return;
    
    auto& geom = *geom_opt;
    
    // Get bounding box of this geometry
    auto x_min = geom.x_min();
    auto x_max = geom.x_max();
    auto y_min = geom.y_min();
    auto y_max = geom.y_max();
    
    if (x_min && x_max && y_min && y_max) {
        extent_ctx->min_x = std::min(extent_ctx->min_x, *x_min);
        extent_ctx->min_y = std::min(extent_ctx->min_y, *y_min);
        extent_ctx->max_x = std::max(extent_ctx->max_x, *x_max);
        extent_ctx->max_y = std::max(extent_ctx->max_y, *y_max);
        extent_ctx->has_data = true;
    }
}

void st_extent_agg_final(sqlite3_context* ctx) {
    auto* extent_ctx = static_cast<ExtentContext*>(
        sqlite3_aggregate_context(ctx, 0)
    );
    
    if (!extent_ctx || !extent_ctx->has_data) {
        sqlite3_result_null(ctx);
        return;
    }
    
    std::ostringstream result;
    result << "BOX(" 
           << extent_ctx->min_x << " " << extent_ctx->min_y << ", "
           << extent_ctx->max_x << " " << extent_ctx->max_y << ")";
    
    std::string extent_str = result.str();
    sqlite3_result_text(ctx, extent_str.c_str(), -1, SQLITE_TRANSIENT);
}

// =============================================================================
// Registration
// =============================================================================

void register_aggregate_functions(sqlite3* db) {
    // ST_Collect
    sqlite3_create_function(
        db, "ST_Collect", 1, SQLITE_UTF8, nullptr,
        nullptr, st_collect_step, st_collect_final
    );
    
    // ST_Union
    sqlite3_create_function(
        db, "ST_Union", 1, SQLITE_UTF8, nullptr,
        nullptr, st_union_step, st_union_final
    );
    
    // ST_ConvexHull_Agg
    sqlite3_create_function(
        db, "ST_ConvexHull_Agg", 1, SQLITE_UTF8, nullptr,
        nullptr, st_convexhull_agg_step, st_convexhull_agg_final
    );
    
    // ST_Extent_Agg
    sqlite3_create_function(
        db, "ST_Extent_Agg", 1, SQLITE_UTF8, nullptr,
        nullptr, st_extent_agg_step, st_extent_agg_final
    );
}

} // namespace sqlitegis
