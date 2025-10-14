#include "sqlitegis/geometry_constructors.hpp"
#include "sqlitegis/geometry_types.hpp"

#include <boost/geometry/io/wkt/write.hpp>

#include <array>
#include <cstring>
#include <sstream>

namespace sqlitegis {

namespace {

constexpr const char* kErrPrefix = "sqlitegis: ";

// Helper to read TEXT argument from SQLite
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

// Helper to read INTEGER argument from SQLite
std::optional<int> read_int_arg(sqlite3_value* value) {
    if (sqlite3_value_type(value) == SQLITE_NULL) {
        return std::nullopt;
    }
    if (sqlite3_value_type(value) != SQLITE_INTEGER) {
        return std::nullopt;
    }
    return sqlite3_value_int(value);
}

// Helper to read REAL argument from SQLite
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

// Helper to read BLOB argument from SQLite
std::optional<std::vector<uint8_t>> read_blob_arg(sqlite3_value* value) {
    if (sqlite3_value_type(value) == SQLITE_NULL) {
        return std::nullopt;
    }
    if (sqlite3_value_type(value) != SQLITE_BLOB) {
        return std::nullopt;
    }
    const void* blob = sqlite3_value_blob(value);
    int size = sqlite3_value_bytes(value);
    if (!blob || size <= 0) {
        return std::nullopt;
    }
    const uint8_t* data = static_cast<const uint8_t*>(blob);
    return std::vector<uint8_t>(data, data + size);
}

// ST_GeomFromText(wkt TEXT) -> Geometry
// ST_GeomFromText(wkt TEXT, srid INTEGER) -> Geometry
void st_geom_from_text(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    // Validate argument count
    if (argc != 1 && argc != 2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromText expects 1 or 2 arguments", -1);
        return;
    }
    
    // Handle NULL input
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read WKT string
    auto wkt = read_text_arg(argv[0]);
    if (!wkt) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromText first argument must be TEXT", -1);
        return;
    }
    
    // Read optional SRID
    int srid = -1;  // Default to -1 (undefined)
    if (argc == 2) {
        auto srid_opt = read_int_arg(argv[1]);
        if (!srid_opt) {
            sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromText second argument must be INTEGER", -1);
            return;
        }
        srid = *srid_opt;
    }
    
    // Create GeometryWrapper
    auto geom = GeometryWrapper::from_wkt(*wkt, srid);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromText invalid WKT format", -1);
        return;
    }
    
    // Return as EWKT string
    std::string result = geom->to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), static_cast<int>(result.length()), SQLITE_TRANSIENT);
}

// ST_GeomFromEWKT(ewkt TEXT) -> Geometry
void st_geom_from_ewkt(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromEWKT expects exactly 1 argument", -1);
        return;
    }
    
    // Handle NULL input
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read EWKT string
    auto ewkt = read_text_arg(argv[0]);
    if (!ewkt) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromEWKT argument must be TEXT", -1);
        return;
    }
    
    // Parse EWKT
    auto geom = GeometryWrapper::from_ewkt(*ewkt);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromEWKT invalid EWKT format", -1);
        return;
    }
    
    // Return as EWKT string (normalized)
    std::string result = geom->to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), static_cast<int>(result.length()), SQLITE_TRANSIENT);
}

// ST_MakePoint(x REAL, y REAL) -> Point
void st_make_point(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_MakePoint expects exactly 2 arguments", -1);
        return;
    }
    
    // Read X coordinate
    auto x = read_real_arg(argv[0]);
    if (!x) {
        sqlite3_result_error(ctx, "sqlitegis: ST_MakePoint first argument must be REAL", -1);
        return;
    }
    
    // Read Y coordinate
    auto y = read_real_arg(argv[1]);
    if (!y) {
        sqlite3_result_error(ctx, "sqlitegis: ST_MakePoint second argument must be REAL", -1);
        return;
    }
    
    // Create Point geometry using Boost.Geometry
    point_t point(*x, *y);
    
    // Convert to WKT
    std::ostringstream oss;
    try {
        oss << boost::geometry::wkt(point);
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_MakePoint WKT generation failed: " + std::string(e.what())).c_str(), -1);
        return;
    }
    
    // Create GeometryWrapper with SRID=-1 (undefined)
    GeometryWrapper geom(oss.str(), -1);
    std::string result = geom.to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), static_cast<int>(result.length()), SQLITE_TRANSIENT);
}

// ST_MakePointZ(x REAL, y REAL, z REAL) -> Point
// ST_MakePointZ(x REAL, y REAL, z REAL, srid INTEGER) -> Point
void st_make_point_z(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 3 && argc != 4) {
        sqlite3_result_error(ctx, "sqlitegis: ST_MakePointZ expects 3 or 4 arguments", -1);
        return;
    }
    
    // Read X coordinate
    auto x = read_real_arg(argv[0]);
    if (!x) {
        sqlite3_result_error(ctx, "sqlitegis: ST_MakePointZ first argument must be REAL", -1);
        return;
    }
    
    // Read Y coordinate
    auto y = read_real_arg(argv[1]);
    if (!y) {
        sqlite3_result_error(ctx, "sqlitegis: ST_MakePointZ second argument must be REAL", -1);
        return;
    }
    
    // Read Z coordinate
    auto z = read_real_arg(argv[2]);
    if (!z) {
        sqlite3_result_error(ctx, "sqlitegis: ST_MakePointZ third argument must be REAL", -1);
        return;
    }
    
    // Read optional SRID
    int srid = -1;  // Default to -1 (undefined)
    if (argc == 4) {
        auto srid_opt = read_int_arg(argv[3]);
        if (!srid_opt) {
            sqlite3_result_error(ctx, "sqlitegis: ST_MakePointZ fourth argument must be INTEGER", -1);
            return;
        }
        srid = *srid_opt;
    }
    
    // Create 3D Point geometry using Boost.Geometry
    point_3d_t point(*x, *y, *z);
    
    // Convert to WKT
    std::ostringstream oss;
    try {
        oss << boost::geometry::wkt(point);
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_MakePointZ WKT generation failed: " + std::string(e.what())).c_str(), -1);
        return;
    }
    
    // Boost.Geometry outputs "POINT(x y z)", we need "POINT Z (x y z)"
    std::string wkt = oss.str();
    if (wkt.find("POINT(") == 0) {
        wkt = "POINT Z (" + wkt.substr(6);
    }
    
    // Create GeometryWrapper with 3D flag
    GeometryWrapper geom(wkt, srid, DimensionType::XYZ);
    std::string result = geom.to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), static_cast<int>(result.length()), SQLITE_TRANSIENT);
}

// ST_SetSRID(geom TEXT, srid INTEGER) -> Geometry
void st_set_srid(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(ctx, "sqlitegis: ST_SetSRID expects exactly 2 arguments", -1);
        return;
    }
    
    // Handle NULL geometry
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read geometry (EWKT or WKT)
    auto geom_text = read_text_arg(argv[0]);
    if (!geom_text) {
        sqlite3_result_error(ctx, "sqlitegis: ST_SetSRID first argument must be TEXT", -1);
        return;
    }
    
    // Read new SRID
    auto new_srid = read_int_arg(argv[1]);
    if (!new_srid) {
        sqlite3_result_error(ctx, "sqlitegis: ST_SetSRID second argument must be INTEGER", -1);
        return;
    }
    
    // Parse geometry (accepts both WKT and EWKT)
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_SetSRID invalid geometry format", -1);
        return;
    }
    
    // Set new SRID (no coordinate transformation)
    geom->set_srid(*new_srid);
    
    // Return updated EWKT
    std::string result = geom->to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), static_cast<int>(result.length()), SQLITE_TRANSIENT);
}

// ST_GeomFromEWKB(ewkb BLOB) -> Geometry
void st_geom_from_ewkb(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    // Validate argument count
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromEWKB expects 1 argument", -1);
        return;
    }
    
    // Handle NULL input
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // Read EWKB binary data
    auto ewkb = read_blob_arg(argv[0]);
    if (!ewkb) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromEWKB argument must be BLOB", -1);
        return;
    }
    
    // Parse EWKB
    auto geom = GeometryWrapper::from_ewkb(*ewkb);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_GeomFromEWKB invalid EWKB format", -1);
        return;
    }
    
    // Return as EWKT string
    std::string result = geom->to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), static_cast<int>(result.length()), SQLITE_TRANSIENT);
}

using sqlite_func = void (*)(sqlite3_context*, int, sqlite3_value**);

struct function_entry {
    const char* name;
    int arg_count;  // -1 for variable arguments
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

int register_constructor_functions(sqlite3* db, char** error_message) {
    static const std::array<function_entry, 6> kFunctions{{
        {"ST_GeomFromText", -1, &st_geom_from_text},  // 1 or 2 args
        {"ST_GeomFromEWKT", 1, &st_geom_from_ewkt},
        {"ST_GeomFromEWKB", 1, &st_geom_from_ewkb},
        {"ST_MakePoint", 2, &st_make_point},
        {"ST_MakePointZ", -1, &st_make_point_z},      // 3 or 4 args
        {"ST_SetSRID", 2, &st_set_srid},
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
