#include "sqlitegis/geometry_accessors.hpp"
#include "sqlitegis/geometry_types.hpp"

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

// ST_AsText(geom TEXT) -> TEXT
void st_as_text(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_AsText expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_AsText argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry (accepts EWKT or WKT)
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_AsText invalid geometry format", -1);
        return;
    }
    
    // Return WKT without SRID
    const std::string& wkt = geom->to_wkt();
    sqlite3_result_text(ctx, wkt.c_str(), static_cast<int>(wkt.length()), SQLITE_TRANSIENT);
}

// ST_AsEWKT(geom TEXT) -> TEXT
void st_as_ewkt(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_AsEWKT expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_AsEWKT argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_AsEWKT invalid geometry format", -1);
        return;
    }
    
    // Return EWKT with SRID
    std::string ewkt = geom->to_ewkt();
    sqlite3_result_text(ctx, ewkt.c_str(), static_cast<int>(ewkt.length()), SQLITE_TRANSIENT);
}

// ST_AsEWKB(geom TEXT) -> BLOB
void st_as_ewkb(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_AsEWKB expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_AsEWKB argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_AsEWKB invalid geometry format", -1);
        return;
    }
    
    // Convert to EWKB
    std::vector<uint8_t> ewkb = geom->to_ewkb();
    if (ewkb.empty()) {
        sqlite3_result_error(ctx, "sqlitegis: ST_AsEWKB failed to generate EWKB", -1);
        return;
    }
    
    // Return as BLOB
    sqlite3_result_blob(ctx, ewkb.data(), static_cast<int>(ewkb.size()), SQLITE_TRANSIENT);
}

// ST_GeometryType(geom TEXT) -> TEXT
void st_geometry_type(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeometryType expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_GeometryType argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeometryType invalid geometry format", -1);
        return;
    }
    
    // Return geometry type name
    std::string type_name = geom->geometry_type_name();
    sqlite3_result_text(ctx, type_name.c_str(), static_cast<int>(type_name.length()), SQLITE_TRANSIENT);
}

// ST_SRID(geom TEXT) -> INTEGER
void st_srid(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_SRID expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_SRID argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_SRID invalid geometry format", -1);
        return;
    }
    
    // Return SRID
    sqlite3_result_int(ctx, geom->srid());
}

// ST_X(geom TEXT) -> REAL
void st_x(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_X expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_X argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_X invalid geometry format", -1);
        return;
    }
    
    // Check geometry type
    if (geom->geometry_type() != GeometryType::Point) {
        sqlite3_result_error(ctx, "sqlitegis: ST_X requires Point geometry", -1);
        return;
    }
    
    // Parse as Point
    auto point = geom->as<point_t>();
    if (!point) {
        sqlite3_result_error(ctx, "sqlitegis: ST_X failed to parse Point", -1);
        return;
    }
    
    // Return X coordinate
    sqlite3_result_double(ctx, point->x());
}

// ST_Y(geom TEXT) -> REAL
void st_y(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Y expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_Y argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Y invalid geometry format", -1);
        return;
    }
    
    // Check geometry type
    if (geom->geometry_type() != GeometryType::Point) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Y requires Point geometry", -1);
        return;
    }
    
    // Parse as Point
    auto point = geom->as<point_t>();
    if (!point) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Y failed to parse Point", -1);
        return;
    }
    
    // Return Y coordinate
    sqlite3_result_double(ctx, point->y());
}

// ST_Z(geom TEXT) -> REAL
void st_z(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Z expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_Z argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Z invalid geometry format", -1);
        return;
    }
    
    // Get Z coordinate (only for 3D Point geometries)
    auto z = geom->get_z();
    if (!z) {
        // Return NULL if not a 3D Point or Z not available
        sqlite3_result_null(ctx);
        return;
    }
    
    sqlite3_result_double(ctx, *z);
}

// ST_Is3D(geom TEXT) -> INTEGER (boolean)
void st_is_3d(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Is3D expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_Is3D argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_Is3D invalid geometry format", -1);
        return;
    }
    
    // Return boolean as integer (1 = true, 0 = false)
    sqlite3_result_int(ctx, geom->is_3d() ? 1 : 0);
}

// ST_CoordDim(geom TEXT) -> INTEGER
void st_coord_dim(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_CoordDim expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_CoordDim argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_CoordDim invalid geometry format", -1);
        return;
    }
    
    // Return coordinate dimension count (2, 3, or 4)
    sqlite3_result_int(ctx, geom->coord_dimension());
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

int register_accessor_functions(sqlite3* db, char** error_message) {
    static const std::array<function_entry, 10> kFunctions{{
        {"ST_AsText", 1, &st_as_text},
        {"ST_AsEWKT", 1, &st_as_ewkt},
        {"ST_AsEWKB", 1, &st_as_ewkb},
        {"ST_GeometryType", 1, &st_geometry_type},
        {"ST_SRID", 1, &st_srid},
        {"ST_X", 1, &st_x},
        {"ST_Y", 1, &st_y},
        {"ST_Z", 1, &st_z},
        {"ST_Is3D", 1, &st_is_3d},
        {"ST_CoordDim", 1, &st_coord_dim},
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
