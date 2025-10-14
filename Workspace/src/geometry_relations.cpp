#include "sqlitegis/geometry_relations.hpp"
#include "sqlitegis/geometry_types.hpp"

#include <boost/geometry/algorithms/distance.hpp>
#include <boost/geometry/algorithms/intersects.hpp>
#include <boost/geometry/algorithms/within.hpp>
#include <boost/geometry/algorithms/covered_by.hpp>

#include <array>

namespace sqlitegis {

namespace {

constexpr const char* kErrPrefix = "sqlitegis: ";

// Helper to read TEXT argument
std::optional<std::string> read_text_arg(sqlite3_value* value) {
    if (sqlite3_value_type(value) == SQLITE_NULL) {
        return std::nullopt;
    }
    if (sqlite3_value_type(value) != SQLITE_TEXT) {
        return std::nullopt;
    }
    const unsigned char* text = sqlite3_value_text(value);
    if (!text) {
        return std::nullopt;
    }
    return std::string(reinterpret_cast<const char*>(text));
}

// Helper to compute distance between two geometry variants
template<typename Geom1, typename Geom2>
double compute_distance(const Geom1& g1, const Geom2& g2) {
    return boost::geometry::distance(g1, g2);
}

// ST_Distance(geom1 TEXT, geom2 TEXT) -> REAL
void st_distance(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Distance expects exactly 2 arguments", -1);
        return;
    }
    
    // Handle NULL
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || 
        sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read geometries
    auto geom1_text = read_text_arg(argv[0]);
    auto geom2_text = read_text_arg(argv[1]);
    
    if (!geom1_text || !geom2_text) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Distance arguments must be TEXT", -1);
        return;
    }
    
    // Parse geometries
    auto geom1 = GeometryWrapper::from_ewkt(*geom1_text);
    auto geom2 = GeometryWrapper::from_ewkt(*geom2_text);
    
    if (!geom1 || !geom2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Distance invalid geometry format", -1);
        return;
    }
    
    // Get variants
    auto var1 = geom1->as_variant();
    auto var2 = geom2->as_variant();
    
    if (!var1 || !var2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Distance failed to parse geometries", -1);
        return;
    }
    
    // Compute distance using std::visit
    try {
        double distance = std::visit([](const auto& g1, const auto& g2) {
            return boost::geometry::distance(g1, g2);
        }, *var1, *var2);
        
        sqlite3_result_double(ctx, distance);
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_Distance calculation failed: " + std::string(e.what())).c_str(), -1);
    }
}

// ST_Intersects(geom1 TEXT, geom2 TEXT) -> BOOLEAN (INTEGER)
void st_intersects(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Intersects expects exactly 2 arguments", -1);
        return;
    }
    
    // Handle NULL
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || 
        sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read geometries
    auto geom1_text = read_text_arg(argv[0]);
    auto geom2_text = read_text_arg(argv[1]);
    
    if (!geom1_text || !geom2_text) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Intersects arguments must be TEXT", -1);
        return;
    }
    
    // Parse geometries
    auto geom1 = GeometryWrapper::from_ewkt(*geom1_text);
    auto geom2 = GeometryWrapper::from_ewkt(*geom2_text);
    
    if (!geom1 || !geom2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Intersects invalid geometry format", -1);
        return;
    }
    
    // Get variants
    auto var1 = geom1->as_variant();
    auto var2 = geom2->as_variant();
    
    if (!var1 || !var2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Intersects failed to parse geometries", -1);
        return;
    }
    
    // Check intersection
    try {
        bool intersects = std::visit([](const auto& g1, const auto& g2) {
            return boost::geometry::intersects(g1, g2);
        }, *var1, *var2);
        
        sqlite3_result_int(ctx, intersects ? 1 : 0);
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_Intersects calculation failed: " + std::string(e.what())).c_str(), -1);
    }
}

// ST_Contains(geom1 TEXT, geom2 TEXT) -> BOOLEAN (INTEGER)
void st_contains(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Contains expects exactly 2 arguments", -1);
        return;
    }
    
    // Handle NULL
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || 
        sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read geometries
    auto geom1_text = read_text_arg(argv[0]);
    auto geom2_text = read_text_arg(argv[1]);
    
    if (!geom1_text || !geom2_text) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Contains arguments must be TEXT", -1);
        return;
    }
    
    // Parse geometries
    auto geom1 = GeometryWrapper::from_ewkt(*geom1_text);
    auto geom2 = GeometryWrapper::from_ewkt(*geom2_text);
    
    if (!geom1 || !geom2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Contains invalid geometry format", -1);
        return;
    }
    
    // Get variants
    auto var1 = geom1->as_variant();
    auto var2 = geom2->as_variant();
    
    if (!var1 || !var2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Contains failed to parse geometries", -1);
        return;
    }
    
    // Check containment using relation matrix (DE-9IM)
    // Contains: Interior(geom2) ∩ Interior(geom1) != ∅ AND Interior(geom2) ∩ Exterior(geom1) = ∅
    // Simplified: Check if geom1 fully contains geom2
    try {
        bool contains = std::visit([](const auto& g1, const auto& g2) -> bool {
            using G1 = std::decay_t<decltype(g1)>;
            using G2 = std::decay_t<decltype(g2)>;
            
            // For unsupported combinations, return false
            // Boost.Geometry only supports specific within/contains combinations
            // Supported: Point in Polygon, Polygon in Polygon, etc.
            if constexpr (
                (std::is_same_v<G2, point_t> && (std::is_same_v<G1, polygon_t> || std::is_same_v<G1, multipolygon_t>)) ||
                (std::is_same_v<G2, linestring_t> && (std::is_same_v<G1, polygon_t> || std::is_same_v<G1, multipolygon_t>)) ||
                (std::is_same_v<G2, polygon_t> && (std::is_same_v<G1, polygon_t> || std::is_same_v<G1, multipolygon_t>)) ||
                (std::is_same_v<G2, multipoint_t> && (std::is_same_v<G1, polygon_t> || std::is_same_v<G1, multipolygon_t>)) ||
                (std::is_same_v<G2, multilinestring_t> && (std::is_same_v<G1, polygon_t> || std::is_same_v<G1, multipolygon_t>)) ||
                (std::is_same_v<G2, multipolygon_t> && (std::is_same_v<G1, polygon_t> || std::is_same_v<G1, multipolygon_t>))
            ) {
                return boost::geometry::within(g2, g1);
            } else {
                return false;  // Unsupported combination
            }
        }, *var1, *var2);
        
        sqlite3_result_int(ctx, contains ? 1 : 0);
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_Contains calculation failed: " + std::string(e.what())).c_str(), -1);
    }
}

// ST_Within(geom1 TEXT, geom2 TEXT) -> BOOLEAN (INTEGER)
void st_within(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Within expects exactly 2 arguments", -1);
        return;
    }
    
    // Handle NULL
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL || 
        sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read geometries
    auto geom1_text = read_text_arg(argv[0]);
    auto geom2_text = read_text_arg(argv[1]);
    
    if (!geom1_text || !geom2_text) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Within arguments must be TEXT", -1);
        return;
    }
    
    // Parse geometries
    auto geom1 = GeometryWrapper::from_ewkt(*geom1_text);
    auto geom2 = GeometryWrapper::from_ewkt(*geom2_text);
    
    if (!geom1 || !geom2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Within invalid geometry format", -1);
        return;
    }
    
    // Get variants
    auto var1 = geom1->as_variant();
    auto var2 = geom2->as_variant();
    
    if (!var1 || !var2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Within failed to parse geometries", -1);
        return;
    }
    
    // Check if geom1 is within geom2
    try {
        bool within = std::visit([](const auto& g1, const auto& g2) -> bool {
            using G1 = std::decay_t<decltype(g1)>;
            using G2 = std::decay_t<decltype(g2)>;
            
            // For unsupported combinations, return false
            // Boost.Geometry only supports specific within combinations
            if constexpr (
                (std::is_same_v<G1, point_t> && (std::is_same_v<G2, polygon_t> || std::is_same_v<G2, multipolygon_t>)) ||
                (std::is_same_v<G1, linestring_t> && (std::is_same_v<G2, polygon_t> || std::is_same_v<G2, multipolygon_t>)) ||
                (std::is_same_v<G1, polygon_t> && (std::is_same_v<G2, polygon_t> || std::is_same_v<G2, multipolygon_t>)) ||
                (std::is_same_v<G1, multipoint_t> && (std::is_same_v<G2, polygon_t> || std::is_same_v<G2, multipolygon_t>)) ||
                (std::is_same_v<G1, multilinestring_t> && (std::is_same_v<G2, polygon_t> || std::is_same_v<G2, multipolygon_t>)) ||
                (std::is_same_v<G1, multipolygon_t> && (std::is_same_v<G2, polygon_t> || std::is_same_v<G2, multipolygon_t>))
            ) {
                return boost::geometry::within(g1, g2);
            } else {
                return false;  // Unsupported combination
            }
        }, *var1, *var2);
        
        sqlite3_result_int(ctx, within ? 1 : 0);
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_Within calculation failed: " + std::string(e.what())).c_str(), -1);
    }
}

using sqlite_func = void (*)(sqlite3_context*, int, sqlite3_value**);

struct function_entry {
    const char* name;
    int arg_count;
    sqlite_func function;
};

int register_function(sqlite3* db, const function_entry& entry) {
    int rc = sqlite3_create_function_v2(
        db,
        entry.name,
        entry.arg_count,
        SQLITE_UTF8 | SQLITE_DETERMINISTIC,
        nullptr,
        entry.function,
        nullptr,
        nullptr,
        nullptr);
    return rc;
}

} // anonymous namespace

int register_relation_functions(sqlite3* db, char** error_message) {
    static const std::array<function_entry, 4> kFunctions{{
        {"ST_Distance", 2, &st_distance},
        {"ST_Intersects", 2, &st_intersects},
        {"ST_Contains", 2, &st_contains},
        {"ST_Within", 2, &st_within},
    }};
    
    for (const auto& entry : kFunctions) {
        const int rc = register_function(db, entry);
        if (rc != SQLITE_OK) {
            if (error_message) {
                *error_message = sqlite3_mprintf(
                    "failed to register %s: %s", 
                    entry.name, 
                    sqlite3_errstr(rc));
            }
            return rc;
        }
    }
    
    return SQLITE_OK;
}

} // namespace sqlitegis
