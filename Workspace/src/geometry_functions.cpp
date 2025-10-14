#include "sqlitegis/geometry_functions.hpp"

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/io/wkt/read.hpp>

#include <array>
#include <optional>
#include <string>
#include <variant>

namespace sqlitegis {

namespace {

using point_t = boost::geometry::model::d2::point_xy<double>;
using polygon_t = boost::geometry::model::polygon<point_t>;
using multipolygon_t = boost::geometry::model::multi_polygon<polygon_t>;
using linestring_t = boost::geometry::model::linestring<point_t>;
using multilinestring_t = boost::geometry::model::multi_linestring<linestring_t>;

constexpr const char* kErrPrefix = "sqlitegis: ";

// Helper to convert SQLite TEXT to std::string, returning empty optional on failure.
std::optional<std::string> read_text_argument(sqlite3_value* value) {
    if (sqlite3_value_type(value) != SQLITE_TEXT) {
        return std::nullopt;
    }
    const unsigned char* text = sqlite3_value_text(value);
    if (!text) {
        return std::nullopt;
    }
    return std::string(reinterpret_cast<const char*>(text));
}

struct wkt_polygon_variant {
    polygon_t polygon;
    multipolygon_t multipolygon;
    bool is_multi = false;
};

struct wkt_linestring_variant {
    linestring_t linestring;
    multilinestring_t multilinestring;
    bool is_multi = false;
};

std::optional<wkt_polygon_variant> parse_polygon_like(const std::string& wkt) {
    wkt_polygon_variant variant{};
    try {
        boost::geometry::read_wkt(wkt, variant.polygon);
        if (!variant.polygon.outer().empty()) {
            return variant;
        }
    } catch (...) {
        // fallthrough and try multi
    }

    try {
        boost::geometry::read_wkt(wkt, variant.multipolygon);
        if (!boost::geometry::is_empty(variant.multipolygon)) {
            variant.is_multi = true;
            return variant;
        }
    } catch (...) {
        // ignore
    }

    return std::nullopt;
}

std::optional<wkt_linestring_variant> parse_linestring_like(const std::string& wkt) {
    wkt_linestring_variant variant{};
    try {
        boost::geometry::read_wkt(wkt, variant.linestring);
        if (!variant.linestring.empty()) {
            return variant;
        }
    } catch (...) {
        // fallthrough
    }

    try {
        boost::geometry::read_wkt(wkt, variant.multilinestring);
        if (!boost::geometry::is_empty(variant.multilinestring)) {
            variant.is_multi = true;
            return variant;
        }
    } catch (...) {
        // ignore
    }

    return std::nullopt;
}

void set_sqlite_error(sqlite3_context* context, const std::string& message) {
    sqlite3_result_error(context, (std::string(kErrPrefix) + message).c_str(), -1);
}

void st_area(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        set_sqlite_error(context, "ST_Area expects exactly 1 argument");
        return;
    }

    auto wkt = read_text_argument(argv[0]);
    if (!wkt) {
        set_sqlite_error(context, "ST_Area argument must be TEXT");
        return;
    }

    auto polygon = parse_polygon_like(*wkt);
    if (!polygon) {
        set_sqlite_error(context, "ST_Area could not parse WKT");
        return;
    }

    double result = polygon->is_multi
        ? boost::geometry::area(polygon->multipolygon)
        : boost::geometry::area(polygon->polygon);

    sqlite3_result_double(context, result);
}

void st_perimeter(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        set_sqlite_error(context, "ST_Perimeter expects exactly 1 argument");
        return;
    }

    auto wkt = read_text_argument(argv[0]);
    if (!wkt) {
        set_sqlite_error(context, "ST_Perimeter argument must be TEXT");
        return;
    }

    auto polygon = parse_polygon_like(*wkt);
    if (!polygon) {
        set_sqlite_error(context, "ST_Perimeter could not parse WKT");
        return;
    }

    double result = polygon->is_multi
        ? boost::geometry::perimeter(polygon->multipolygon)
        : boost::geometry::perimeter(polygon->polygon);

    sqlite3_result_double(context, result);
}

void st_length(sqlite3_context* context, int argc, sqlite3_value** argv) {
    if (argc != 1) {
        set_sqlite_error(context, "ST_Length expects exactly 1 argument");
        return;
    }

    auto wkt = read_text_argument(argv[0]);
    if (!wkt) {
        set_sqlite_error(context, "ST_Length argument must be TEXT");
        return;
    }

    auto linestring = parse_linestring_like(*wkt);
    if (!linestring) {
        set_sqlite_error(context, "ST_Length could not parse WKT");
        return;
    }

    double result = linestring->is_multi
        ? boost::geometry::length(linestring->multilinestring)
        : boost::geometry::length(linestring->linestring);

    sqlite3_result_double(context, result);
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

} // namespace

int register_geometry_functions(sqlite3* db, char** error_message) {
    static constexpr std::array<function_entry, 3> kFunctions{{
        {"ST_Area", 1, &st_area},
        {"ST_Perimeter", 1, &st_perimeter},
        {"ST_Length", 1, &st_length},
    }};

    for (const auto& entry : kFunctions) {
        const int rc = register_function(db, entry);
        if (rc != SQLITE_OK) {
            if (error_message) {
                *error_message = sqlite3_mprintf("failed to register %s: %s", entry.name, sqlite3_errstr(rc));
            }
            return rc;
        }
    }

    return SQLITE_OK;
}

} // namespace sqlitegis
