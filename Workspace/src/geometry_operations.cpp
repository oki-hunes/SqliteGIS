#include "sqlitegis/geometry_operations.hpp"
#include "sqlitegis/geometry_types.hpp"

#include <boost/geometry/algorithms/centroid.hpp>
#include <boost/geometry/algorithms/buffer.hpp>
#include <boost/geometry/strategies/cartesian/buffer_point_circle.hpp>
#include <boost/geometry/strategies/cartesian/buffer_join_round.hpp>
#include <boost/geometry/strategies/cartesian/buffer_end_round.hpp>
#include <boost/geometry/strategies/cartesian/buffer_side_straight.hpp>
#include <boost/geometry/io/wkt/write.hpp>

#include <array>
#include <sstream>

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

// Helper to read REAL argument
std::optional<double> read_real_arg(sqlite3_value* value) {
    if (sqlite3_value_type(value) == SQLITE_NULL) {
        return std::nullopt;
    }
    int type = sqlite3_value_type(value);
    if (type == SQLITE_FLOAT || type == SQLITE_INTEGER) {
        return sqlite3_value_double(value);
    }
    return std::nullopt;
}

// ST_Centroid(geom TEXT) -> Point (TEXT)
void st_centroid(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Centroid expects exactly 1 argument", -1);
        return;
    }
    
    // Handle NULL
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read geometry
    auto geom_text = read_text_arg(argv[0]);
    if (!geom_text) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Centroid argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Centroid invalid geometry format", -1);
        return;
    }
    
    // Get variant
    auto var = geom->as_variant();
    if (!var) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Centroid failed to parse geometry", -1);
        return;
    }
    
    // Calculate centroid
    try {
        point_t centroid;
        std::visit([&centroid](const auto& g) {
            boost::geometry::centroid(g, centroid);
        }, *var);
        
        // Convert centroid to WKT
        std::ostringstream oss;
        oss << boost::geometry::wkt(centroid);
        
        // Create result geometry with same SRID as input
        GeometryWrapper result(oss.str(), geom->srid());
        std::string result_ewkt = result.to_ewkt();
        
        sqlite3_result_text(ctx, result_ewkt.c_str(), static_cast<int>(result_ewkt.length()), SQLITE_TRANSIENT);
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_Centroid calculation failed: " + std::string(e.what())).c_str(), -1);
    }
}

// ST_Buffer(geom TEXT, distance REAL) -> Geometry (TEXT)
void st_buffer(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Buffer expects exactly 2 arguments", -1);
        return;
    }
    
    // Handle NULL
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read geometry
    auto geom_text = read_text_arg(argv[0]);
    if (!geom_text) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Buffer first argument must be TEXT", -1);
        return;
    }
    
    // Read distance
    auto distance = read_real_arg(argv[1]);
    if (!distance) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Buffer second argument must be REAL", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Buffer invalid geometry format", -1);
        return;
    }
    
    // Get variant
    auto var = geom->as_variant();
    if (!var) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Buffer failed to parse geometry", -1);
        return;
    }
    
    // Calculate buffer
    try {
        // Buffer result is always a multi_polygon
        multipolygon_t buffered;
        
        // Set up buffer strategies
        const int points_per_circle = 36;
        boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(*distance);
        boost::geometry::strategy::buffer::join_round join_strategy(points_per_circle);
        boost::geometry::strategy::buffer::end_round end_strategy(points_per_circle);
        boost::geometry::strategy::buffer::point_circle circle_strategy(points_per_circle);
        boost::geometry::strategy::buffer::side_straight side_strategy;
        
        // Perform buffer operation
        std::visit([&](const auto& g) {
            boost::geometry::buffer(g, buffered, 
                                   distance_strategy, 
                                   side_strategy,
                                   join_strategy, 
                                   end_strategy, 
                                   circle_strategy);
        }, *var);
        
        // Convert result to WKT
        std::ostringstream oss;
        oss << boost::geometry::wkt(buffered);
        
        // Create result geometry with same SRID as input
        GeometryWrapper result(oss.str(), geom->srid());
        std::string result_ewkt = result.to_ewkt();
        
        sqlite3_result_text(ctx, result_ewkt.c_str(), static_cast<int>(result_ewkt.length()), SQLITE_TRANSIENT);
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_Buffer calculation failed: " + std::string(e.what())).c_str(), -1);
    }
}

// ST_Force2D(geom TEXT) -> Geometry (2D)
void st_force_2d(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Force2D expects exactly 1 argument", -1);
        return;
    }
    
    // Handle NULL
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read geometry
    auto geom_text = read_text_arg(argv[0]);
    if (!geom_text) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Force2D argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Force2D invalid geometry format", -1);
        return;
    }
    
    // Convert to 2D
    GeometryWrapper geom_2d = geom->force_2d();
    
    // Return as EWKT
    std::string result = geom_2d.to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), static_cast<int>(result.length()), SQLITE_TRANSIENT);
}

// ST_Force3D(geom TEXT) -> Geometry (3D)
// ST_Force3D(geom TEXT, z_default REAL) -> Geometry (3D)
void st_force_3d(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1 && argc != 2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Force3D expects 1 or 2 arguments", -1);
        return;
    }
    
    // Handle NULL
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read geometry
    auto geom_text = read_text_arg(argv[0]);
    if (!geom_text) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Force3D first argument must be TEXT", -1);
        return;
    }
    
    // Read optional Z default value
    double z_default = 0.0;
    if (argc == 2) {
        auto z_opt = read_real_arg(argv[1]);
        if (!z_opt) {
            sqlite3_result_error(ctx, "sqlitegis: ST_Force3D second argument must be REAL", -1);
            return;
        }
        z_default = *z_opt;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Force3D invalid geometry format", -1);
        return;
    }
    
    // Convert to 3D
    GeometryWrapper geom_3d = geom->force_3d(z_default);
    
    // Return as EWKT
    std::string result = geom_3d.to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), static_cast<int>(result.length()), SQLITE_TRANSIENT);
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

int register_operation_functions(sqlite3* db, char** error_message) {
    static const std::array<function_entry, 4> kFunctions{{
        {"ST_Centroid", 1, &st_centroid},
        {"ST_Buffer", 2, &st_buffer},
        {"ST_Force2D", 1, &st_force_2d},
        {"ST_Force3D", -1, &st_force_3d},  // 1 or 2 args
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
