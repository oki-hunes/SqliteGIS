#include "sqlitegis/geometry_measures.hpp"
#include "sqlitegis/geometry_types.hpp"

#include <boost/geometry/algorithms/area.hpp>
#include <boost/geometry/algorithms/length.hpp>
#include <boost/geometry/algorithms/perimeter.hpp>

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

// ST_Area(geom TEXT) -> REAL
void st_area(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Area expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_Area argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry (accepts EWKT or WKT)
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Area invalid geometry format", -1);
        return;
    }
    
    // Calculate area based on geometry type
    GeometryType type = geom->geometry_type();
    double area = 0.0;
    
    try {
        if (type == GeometryType::Polygon) {
            auto polygon = geom->as<polygon_t>();
            if (!polygon) {
                sqlite3_result_error(ctx, "sqlitegis: ST_Area failed to parse Polygon", -1);
                return;
            }
            area = boost::geometry::area(*polygon);
        } else if (type == GeometryType::MultiPolygon) {
            auto multipolygon = geom->as<multipolygon_t>();
            if (!multipolygon) {
                sqlite3_result_error(ctx, "sqlitegis: ST_Area failed to parse MultiPolygon", -1);
                return;
            }
            area = boost::geometry::area(*multipolygon);
        } else {
            sqlite3_result_error(ctx, "sqlitegis: ST_Area requires Polygon or MultiPolygon", -1);
            return;
        }
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_Area calculation failed: " + std::string(e.what())).c_str(), -1);
        return;
    }
    
    sqlite3_result_double(ctx, area);
}

// ST_Perimeter(geom TEXT) -> REAL
void st_perimeter(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Perimeter expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_Perimeter argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Perimeter invalid geometry format", -1);
        return;
    }
    
    // Calculate perimeter based on geometry type
    GeometryType type = geom->geometry_type();
    double perimeter = 0.0;
    
    try {
        if (type == GeometryType::Polygon) {
            auto polygon = geom->as<polygon_t>();
            if (!polygon) {
                sqlite3_result_error(ctx, "sqlitegis: ST_Perimeter failed to parse Polygon", -1);
                return;
            }
            perimeter = boost::geometry::perimeter(*polygon);
        } else if (type == GeometryType::MultiPolygon) {
            auto multipolygon = geom->as<multipolygon_t>();
            if (!multipolygon) {
                sqlite3_result_error(ctx, "sqlitegis: ST_Perimeter failed to parse MultiPolygon", -1);
                return;
            }
            perimeter = boost::geometry::perimeter(*multipolygon);
        } else {
            sqlite3_result_error(ctx, "sqlitegis: ST_Perimeter requires Polygon or MultiPolygon", -1);
            return;
        }
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_Perimeter calculation failed: " + std::string(e.what())).c_str(), -1);
        return;
    }
    
    sqlite3_result_double(ctx, perimeter);
}

// ST_Length(geom TEXT) -> REAL
void st_length(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Length expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_Length argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Length invalid geometry format", -1);
        return;
    }
    
    // Calculate length based on geometry type
    GeometryType type = geom->geometry_type();
    double length = 0.0;
    
    try {
        if (type == GeometryType::LineString) {
            auto linestring = geom->as<linestring_t>();
            if (!linestring) {
                sqlite3_result_error(ctx, "sqlitegis: ST_Length failed to parse LineString", -1);
                return;
            }
            length = boost::geometry::length(*linestring);
        } else if (type == GeometryType::MultiLineString) {
            auto multilinestring = geom->as<multilinestring_t>();
            if (!multilinestring) {
                sqlite3_result_error(ctx, "sqlitegis: ST_Length failed to parse MultiLineString", -1);
                return;
            }
            length = boost::geometry::length(*multilinestring);
        } else {
            sqlite3_result_error(ctx, "sqlitegis: ST_Length requires LineString or MultiLineString", -1);
            return;
        }
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_Length calculation failed: " + std::string(e.what())).c_str(), -1);
        return;
    }
    
    sqlite3_result_double(ctx, length);
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

int register_measure_functions(sqlite3* db, char** error_message) {
    static const std::array<function_entry, 3> kFunctions{{
        {"ST_Area", 1, &st_area},
        {"ST_Perimeter", 1, &st_perimeter},
        {"ST_Length", 1, &st_length},
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
