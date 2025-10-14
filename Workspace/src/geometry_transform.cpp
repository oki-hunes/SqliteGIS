#include "sqlitegis/geometry_transform.hpp"
#include "sqlitegis/geometry_types.hpp"

#include <sqlite3.h>
#include <string>
#include <sstream>
#include <memory>
#include <map>
#include <mutex>

#ifdef HAVE_PROJ
#include <proj.h>
#endif

namespace sqlitegis {

#ifdef HAVE_PROJ

// =============================================================================
// PROJ Context Management (Thread-safe singleton)
// =============================================================================

class ProjContext {
public:
    static ProjContext& instance() {
        static ProjContext ctx;
        return ctx;
    }
    
    PJ* get_transform(int source_srid, int target_srid) {
        // Same SRID, no transformation needed
        if (source_srid == target_srid) {
            return nullptr;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check cache
        auto key = std::make_pair(source_srid, target_srid);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second;
        }
        
        // Create new transformation
        std::string src = "EPSG:" + std::to_string(source_srid);
        std::string dst = "EPSG:" + std::to_string(target_srid);
        
        PJ* pj = proj_create_crs_to_crs(ctx_, src.c_str(), dst.c_str(), nullptr);
        if (!pj) {
            return nullptr;  // Transformation failed
        }
        
        // Normalize for use
        PJ* pj_normalized = proj_normalize_for_visualization(ctx_, pj);
        if (pj_normalized) {
            proj_destroy(pj);
            pj = pj_normalized;
        }
        
        // Cache it
        cache_[key] = pj;
        transforms_.emplace_back(pj, ProjDeleter{});
        
        return pj;
    }
    
    std::string get_crs_name(int srid) {
        std::string epsg_code = "EPSG:" + std::to_string(srid);
        PJ* crs = proj_create(ctx_, epsg_code.c_str());
        
        if (!crs) {
            return "Unknown";
        }
        
        const char* name = proj_get_name(crs);
        std::string result = name ? name : "Unknown";
        proj_destroy(crs);
        
        return result;
    }
    
    std::string get_proj_version() {
        PJ_INFO info = proj_info();
        return std::string(info.version);
    }
    
    PJ_CONTEXT* context() { return ctx_; }
    
private:
    struct ProjDeleter {
        void operator()(PJ* pj) {
            if (pj) proj_destroy(pj);
        }
    };
    
    ProjContext() : ctx_(proj_context_create()) {}
    
    ~ProjContext() {
        transforms_.clear();  // Destroy all PJ objects
        proj_context_destroy(ctx_);
    }
    
    PJ_CONTEXT* ctx_;
    std::mutex mutex_;
    std::map<std::pair<int, int>, PJ*> cache_;
    std::vector<std::unique_ptr<PJ, ProjDeleter>> transforms_;
};

// =============================================================================
// Coordinate Transformation Helpers
// =============================================================================

// Transform a single 2D point
point_t transform_point_2d(const point_t& pt, PJ* transform) {
    PJ_COORD in = proj_coord(pt.x(), pt.y(), 0, 0);
    PJ_COORD out = proj_trans(transform, PJ_FWD, in);
    return point_t(out.xy.x, out.xy.y);
}

// Transform a single 3D point
point_3d_t transform_point_3d(const point_3d_t& pt, PJ* transform) {
    using boost::geometry::get;
    PJ_COORD in = proj_coord(get<0>(pt), get<1>(pt), get<2>(pt), 0);
    PJ_COORD out = proj_trans(transform, PJ_FWD, in);
    
    point_3d_t result;
    boost::geometry::set<0>(result, out.xyz.x);
    boost::geometry::set<1>(result, out.xyz.y);
    boost::geometry::set<2>(result, out.xyz.z);
    return result;
}

// Transform linestring
linestring_t transform_linestring(const linestring_t& line, PJ* transform) {
    linestring_t result;
    for (const auto& pt : line) {
        result.push_back(transform_point_2d(pt, transform));
    }
    return result;
}

// Transform 3D linestring
linestring_3d_t transform_linestring_3d(const linestring_3d_t& line, PJ* transform) {
    linestring_3d_t result;
    for (const auto& pt : line) {
        result.push_back(transform_point_3d(pt, transform));
    }
    return result;
}

// Transform polygon
polygon_t transform_polygon(const polygon_t& poly, PJ* transform) {
    polygon_t result;
    
    // Outer ring
    for (const auto& pt : poly.outer()) {
        result.outer().push_back(transform_point_2d(pt, transform));
    }
    
    // Inner rings
    auto& result_inners = result.inners();
    for (const auto& inner : poly.inners()) {
        typename polygon_t::ring_type transformed_inner;
        for (const auto& pt : inner) {
            transformed_inner.push_back(transform_point_2d(pt, transform));
        }
        result_inners.push_back(transformed_inner);
    }
    
    return result;
}

// Transform 3D polygon
polygon_3d_t transform_polygon_3d(const polygon_3d_t& poly, PJ* transform) {
    polygon_3d_t result;
    
    // Outer ring
    for (const auto& pt : poly.outer()) {
        result.outer().push_back(transform_point_3d(pt, transform));
    }
    
    // Inner rings
    auto& result_inners = result.inners();
    for (const auto& inner : poly.inners()) {
        typename polygon_3d_t::ring_type transformed_inner;
        for (const auto& pt : inner) {
            transformed_inner.push_back(transform_point_3d(pt, transform));
        }
        result_inners.push_back(transformed_inner);
    }
    
    return result;
}

// Transform multipoint
multipoint_t transform_multipoint(const multipoint_t& mp, PJ* transform) {
    multipoint_t result;
    for (const auto& pt : mp) {
        result.push_back(transform_point_2d(pt, transform));
    }
    return result;
}

// Transform 3D multipoint
multipoint_3d_t transform_multipoint_3d(const multipoint_3d_t& mp, PJ* transform) {
    multipoint_3d_t result;
    for (const auto& pt : mp) {
        result.push_back(transform_point_3d(pt, transform));
    }
    return result;
}

// Transform multilinestring
multilinestring_t transform_multilinestring(const multilinestring_t& ml, PJ* transform) {
    multilinestring_t result;
    for (const auto& line : ml) {
        result.push_back(transform_linestring(line, transform));
    }
    return result;
}

// Transform 3D multilinestring
multilinestring_3d_t transform_multilinestring_3d(const multilinestring_3d_t& ml, PJ* transform) {
    multilinestring_3d_t result;
    for (const auto& line : ml) {
        result.push_back(transform_linestring_3d(line, transform));
    }
    return result;
}

// Transform multipolygon
multipolygon_t transform_multipolygon(const multipolygon_t& mp, PJ* transform) {
    multipolygon_t result;
    for (const auto& poly : mp) {
        result.push_back(transform_polygon(poly, transform));
    }
    return result;
}

// Transform 3D multipolygon
multipolygon_3d_t transform_multipolygon_3d(const multipolygon_3d_t& mp, PJ* transform) {
    multipolygon_3d_t result;
    for (const auto& poly : mp) {
        result.push_back(transform_polygon_3d(poly, transform));
    }
    return result;
}

// Transform geometry wrapper
std::optional<GeometryWrapper> transform_geometry(
    const GeometryWrapper& geom,
    int target_srid
) {
    // Same SRID, no transformation
    if (geom.srid() == target_srid) {
        return geom;
    }
    
    // Get transformation
    PJ* transform = ProjContext::instance().get_transform(geom.srid(), target_srid);
    if (!transform) {
        return std::nullopt;  // Transformation not available
    }
    
    try {
        std::ostringstream result_wkt;
        
        if (geom.is_3d()) {
            // 3D geometry transformation
            auto var = geom.as_3d_variant();
            if (!var) return std::nullopt;
            
            std::visit([&](auto&& g) {
                using T = std::decay_t<decltype(g)>;
                if constexpr (std::is_same_v<T, point_3d_t>) {
                    auto transformed = transform_point_3d(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, linestring_3d_t>) {
                    auto transformed = transform_linestring_3d(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, polygon_3d_t>) {
                    auto transformed = transform_polygon_3d(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, multipoint_3d_t>) {
                    auto transformed = transform_multipoint_3d(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, multilinestring_3d_t>) {
                    auto transformed = transform_multilinestring_3d(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, multipolygon_3d_t>) {
                    auto transformed = transform_multipolygon_3d(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                }
            }, *var);
        } else {
            // 2D geometry transformation
            auto var = geom.as_variant();
            if (!var) return std::nullopt;
            
            std::visit([&](auto&& g) {
                using T = std::decay_t<decltype(g)>;
                if constexpr (std::is_same_v<T, point_t>) {
                    auto transformed = transform_point_2d(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, linestring_t>) {
                    auto transformed = transform_linestring(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, polygon_t>) {
                    auto transformed = transform_polygon(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, multipoint_t>) {
                    auto transformed = transform_multipoint(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, multilinestring_t>) {
                    auto transformed = transform_multilinestring(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                } else if constexpr (std::is_same_v<T, multipolygon_t>) {
                    auto transformed = transform_multipolygon(g, transform);
                    result_wkt << boost::geometry::wkt(transformed);
                }
            }, *var);
        }
        
        return GeometryWrapper(result_wkt.str(), target_srid, geom.dimension());
        
    } catch (const std::exception& e) {
        return std::nullopt;
    }
}

#endif // HAVE_PROJ

// =============================================================================
// SQL Functions
// =============================================================================

// ST_Transform(geometry, target_srid) - Transform coordinates
void st_transform(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
#ifdef HAVE_PROJ
    if (argc != 2) {
        sqlite3_result_error(ctx, "ST_Transform requires 2 arguments", -1);
        return;
    }
    
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL ||
        sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    const char* ewkt = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    int target_srid = sqlite3_value_int(argv[1]);
    
    auto geom_opt = GeometryWrapper::from_ewkt(ewkt);
    if (!geom_opt) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    // Check if source SRID is valid
    if (geom_opt->srid() == -1) {
        sqlite3_result_error(ctx, "Source geometry has undefined SRID (-1)", -1);
        return;
    }
    
    auto transformed = transform_geometry(*geom_opt, target_srid);
    if (!transformed) {
        sqlite3_result_error(ctx, "Transformation failed - invalid SRID or unsupported conversion", -1);
        return;
    }
    
    std::string result = transformed->to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), -1, SQLITE_TRANSIENT);
#else
    sqlite3_result_error(ctx, "ST_Transform not available - PROJ library not found", -1);
#endif
}

// ST_SetSRID(geometry, srid) - Set SRID without transformation
void st_set_srid(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    if (argc != 2) {
        sqlite3_result_error(ctx, "ST_SetSRID requires 2 arguments", -1);
        return;
    }
    
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL ||
        sqlite3_value_type(argv[1]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    const char* ewkt = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
    int new_srid = sqlite3_value_int(argv[1]);
    
    auto geom_opt = GeometryWrapper::from_ewkt(ewkt);
    if (!geom_opt) {
        sqlite3_result_error(ctx, "Invalid geometry", -1);
        return;
    }
    
    geom_opt->set_srid(new_srid);
    std::string result = geom_opt->to_ewkt();
    sqlite3_result_text(ctx, result.c_str(), -1, SQLITE_TRANSIENT);
}

// PROJ_Version() - Get PROJ library version
void proj_version(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
#ifdef HAVE_PROJ
    std::string version = ProjContext::instance().get_proj_version();
    sqlite3_result_text(ctx, version.c_str(), -1, SQLITE_TRANSIENT);
#else
    sqlite3_result_text(ctx, "PROJ not available", -1, SQLITE_STATIC);
#endif
}

// PROJ_GetCRSInfo(srid) - Get CRS information
void proj_get_crs_info(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
#ifdef HAVE_PROJ
    if (argc != 1) {
        sqlite3_result_error(ctx, "PROJ_GetCRSInfo requires 1 argument", -1);
        return;
    }
    
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    int srid = sqlite3_value_int(argv[0]);
    std::string crs_name = ProjContext::instance().get_crs_name(srid);
    sqlite3_result_text(ctx, crs_name.c_str(), -1, SQLITE_TRANSIENT);
#else
    sqlite3_result_text(ctx, "PROJ not available", -1, SQLITE_STATIC);
#endif
}

// =============================================================================
// Registration
// =============================================================================

void register_transform_functions(sqlite3* db) {
    sqlite3_create_function(db, "ST_Transform", 2, SQLITE_UTF8, nullptr,
                           st_transform, nullptr, nullptr);
    
    sqlite3_create_function(db, "ST_SetSRID", 2, SQLITE_UTF8, nullptr,
                           st_set_srid, nullptr, nullptr);
    
    sqlite3_create_function(db, "PROJ_Version", 0, SQLITE_UTF8, nullptr,
                           proj_version, nullptr, nullptr);
    
    sqlite3_create_function(db, "PROJ_GetCRSInfo", 1, SQLITE_UTF8, nullptr,
                           proj_get_crs_info, nullptr, nullptr);
}

} // namespace sqlitegis
