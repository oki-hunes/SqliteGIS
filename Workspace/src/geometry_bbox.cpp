#include "sqlitegis/geometry_bbox.hpp"
#include "sqlitegis/geometry_types.hpp"

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT3

#include <array>
#include <cstring>

namespace sqlitegis {

namespace {

// Helper: Read TEXT argument from SQLite
const char* read_text_arg(sqlite3_context* ctx, sqlite3_value* val) {
    if (sqlite3_value_type(val) != SQLITE_TEXT) {
        sqlite3_result_error(ctx, "Expected TEXT argument", -1);
        return nullptr;
    }
    return reinterpret_cast<const char*>(sqlite3_value_text(val));
}

// ST_Envelope(geometry) -> POLYGON
void st_envelope(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "ST_Envelope requires 1 argument", -1);
        return;
    }
    
    const char* ewkt = read_text_arg(ctx, argv[0]);
    if (!ewkt) return;
    
    auto geom = GeometryWrapper::from_ewkt(ewkt);
    if (!geom) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    auto envelope = geom->envelope();
    if (!envelope) {
        sqlite3_result_null(ctx);
        return;
    }
    
    std::string result = envelope->to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), -1, SQLITE_TRANSIENT);
}

// ST_Extent(geometry) -> TEXT "BOX(minX minY, maxX maxY)"
void st_extent(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "ST_Extent requires 1 argument", -1);
        return;
    }
    
    const char* ewkt = read_text_arg(ctx, argv[0]);
    if (!ewkt) return;
    
    auto geom = GeometryWrapper::from_ewkt(ewkt);
    if (!geom) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    auto extent = geom->extent();
    if (!extent) {
        sqlite3_result_null(ctx);
        return;
    }
    
    sqlite3_result_text(ctx, extent->c_str(), -1, SQLITE_TRANSIENT);
}

// ST_XMin(geometry) -> REAL
void st_xmin(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "ST_XMin requires 1 argument", -1);
        return;
    }
    
    const char* ewkt = read_text_arg(ctx, argv[0]);
    if (!ewkt) return;
    
    auto geom = GeometryWrapper::from_ewkt(ewkt);
    if (!geom) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    auto value = geom->x_min();
    if (!value) {
        sqlite3_result_null(ctx);
        return;
    }
    
    sqlite3_result_double(ctx, *value);
}

// ST_XMax(geometry) -> REAL
void st_xmax(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "ST_XMax requires 1 argument", -1);
        return;
    }
    
    const char* ewkt = read_text_arg(ctx, argv[0]);
    if (!ewkt) return;
    
    auto geom = GeometryWrapper::from_ewkt(ewkt);
    if (!geom) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    auto value = geom->x_max();
    if (!value) {
        sqlite3_result_null(ctx);
        return;
    }
    
    sqlite3_result_double(ctx, *value);
}

// ST_YMin(geometry) -> REAL
void st_ymin(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "ST_YMin requires 1 argument", -1);
        return;
    }
    
    const char* ewkt = read_text_arg(ctx, argv[0]);
    if (!ewkt) return;
    
    auto geom = GeometryWrapper::from_ewkt(ewkt);
    if (!geom) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    auto value = geom->y_min();
    if (!value) {
        sqlite3_result_null(ctx);
        return;
    }
    
    sqlite3_result_double(ctx, *value);
}

// ST_YMax(geometry) -> REAL
void st_ymax(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "ST_YMax requires 1 argument", -1);
        return;
    }
    
    const char* ewkt = read_text_arg(ctx, argv[0]);
    if (!ewkt) return;
    
    auto geom = GeometryWrapper::from_ewkt(ewkt);
    if (!geom) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    auto value = geom->y_max();
    if (!value) {
        sqlite3_result_null(ctx);
        return;
    }
    
    sqlite3_result_double(ctx, *value);
}

// ST_ZMin(geometry) -> REAL
void st_zmin(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "ST_ZMin requires 1 argument", -1);
        return;
    }
    
    const char* ewkt = read_text_arg(ctx, argv[0]);
    if (!ewkt) return;
    
    auto geom = GeometryWrapper::from_ewkt(ewkt);
    if (!geom) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    auto value = geom->z_min();
    if (!value) {
        sqlite3_result_null(ctx);
        return;
    }
    
    sqlite3_result_double(ctx, *value);
}

// ST_ZMax(geometry) -> REAL
void st_zmax(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "ST_ZMax requires 1 argument", -1);
        return;
    }
    
    const char* ewkt = read_text_arg(ctx, argv[0]);
    if (!ewkt) return;
    
    auto geom = GeometryWrapper::from_ewkt(ewkt);
    if (!geom) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    auto value = geom->z_max();
    if (!value) {
        sqlite3_result_null(ctx);
        return;
    }
    
    sqlite3_result_double(ctx, *value);
}

} // anonymous namespace

void register_bbox_functions(sqlite3* db) {
    struct function_entry {
        const char* name;
        int nargs;
        void (*func)(sqlite3_context*, int, sqlite3_value**);
    };
    
    constexpr std::array<function_entry, 8> functions = {{
        {"ST_Envelope", 1, st_envelope},
        {"ST_Extent", 1, st_extent},
        {"ST_XMin", 1, st_xmin},
        {"ST_XMax", 1, st_xmax},
        {"ST_YMin", 1, st_ymin},
        {"ST_YMax", 1, st_ymax},
        {"ST_ZMin", 1, st_zmin},
        {"ST_ZMax", 1, st_zmax}
    }};
    
    for (const auto& entry : functions) {
        sqlite3_create_function_v2(
            db,
            entry.name,
            entry.nargs,
            SQLITE_UTF8 | SQLITE_DETERMINISTIC,
            nullptr,
            entry.func,
            nullptr,
            nullptr,
            nullptr
        );
    }
}

} // namespace sqlitegis
