#include "sqlitegis/geometry_utils.hpp"
#include "sqlitegis/geometry_types.hpp"

#include <boost/geometry/algorithms/is_valid.hpp>
#include <boost/geometry/algorithms/is_empty.hpp>

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

// ST_IsValid(geom TEXT) -> BOOLEAN (INTEGER)
void st_is_valid(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_IsValid expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_IsValid argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        // Invalid WKT format means geometry is not valid
        sqlite3_result_int(ctx, 0);
        return;
    }
    
    // Get variant
    auto var = geom->as_variant();
    if (!var) {
        sqlite3_result_int(ctx, 0);
        return;
    }
    
    // Check validity using Boost.Geometry
    try {
        bool is_valid = std::visit([](const auto& g) {
            return boost::geometry::is_valid(g);
        }, *var);
        
        sqlite3_result_int(ctx, is_valid ? 1 : 0);
    } catch (const std::exception& e) {
        // If validation throws, consider it invalid
        sqlite3_result_int(ctx, 0);
    }
}

// ST_IsEmpty(geom TEXT) -> BOOLEAN (INTEGER)
void st_is_empty(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        sqlite3_result_error(ctx, "sqlitegis: ST_IsEmpty expects exactly 1 argument", -1);
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
        sqlite3_result_error(ctx, "sqlitegis: ST_IsEmpty argument must be TEXT", -1);
        return;
    }
    
    // Parse geometry
    auto geom = GeometryWrapper::from_ewkt(*geom_text);
    if (!geom) {
        sqlite3_result_error(ctx, "sqlitegis: ST_IsEmpty invalid geometry format", -1);
        return;
    }
    
    // Check if GeometryWrapper is empty
    if (geom->is_empty()) {
        sqlite3_result_int(ctx, 1);
        return;
    }
    
    // Get variant
    auto var = geom->as_variant();
    if (!var) {
        sqlite3_result_int(ctx, 1);
        return;
    }
    
    // Check emptiness using Boost.Geometry
    try {
        bool is_empty = std::visit([](const auto& g) {
            return boost::geometry::is_empty(g);
        }, *var);
        
        sqlite3_result_int(ctx, is_empty ? 1 : 0);
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: ST_IsEmpty check failed: " + std::string(e.what())).c_str(), -1);
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

int register_utility_functions(sqlite3* db, char** error_message) {
    static const std::array<function_entry, 2> kFunctions{{
        {"ST_IsValid", 1, &st_is_valid},
        {"ST_IsEmpty", 1, &st_is_empty},
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
