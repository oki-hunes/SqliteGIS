#pragma once

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

namespace sqlitegis {

// WKB/EWKB format constants
enum class ByteOrder : uint8_t {
    BigEndian = 0,
    LittleEndian = 1
};

enum class WKBType : uint32_t {
    Point = 1,
    LineString = 2,
    Polygon = 3,
    MultiPoint = 4,
    MultiLineString = 5,
    MultiPolygon = 6,
    GeometryCollection = 7
};

// SRID flag for EWKB format (PostGIS compatible)
constexpr uint32_t SRID_FLAG = 0x20000000;

// Dimension flags for EWKB format (PostGIS compatible)
constexpr uint32_t WKB_Z_FLAG = 0x80000000;  // Z coordinate (3D)
constexpr uint32_t WKB_M_FLAG = 0x40000000;  // M coordinate (measured)

// Dimension type enumeration
enum class DimensionType : int {
    XY = 0,   // 2D (default)
    XYZ = 1,  // 3D with Z coordinate
    XYM = 2,  // 2D with M coordinate (measured)
    XYZM = 3  // 3D with both Z and M coordinates
};

// Boost.Geometry type aliases for 2D geometries
using point_t = boost::geometry::model::d2::point_xy<double>;
using linestring_t = boost::geometry::model::linestring<point_t>;
using polygon_t = boost::geometry::model::polygon<point_t>;
using multipoint_t = boost::geometry::model::multi_point<point_t>;
using multilinestring_t = boost::geometry::model::multi_linestring<linestring_t>;
using multipolygon_t = boost::geometry::model::multi_polygon<polygon_t>;
using box_t = boost::geometry::model::box<point_t>;

// Boost.Geometry type aliases for 3D geometries
using point_3d_t = boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian>;
using linestring_3d_t = boost::geometry::model::linestring<point_3d_t>;
using polygon_3d_t = boost::geometry::model::polygon<point_3d_t>;
using multipoint_3d_t = boost::geometry::model::multi_point<point_3d_t>;
using multilinestring_3d_t = boost::geometry::model::multi_linestring<linestring_3d_t>;
using multipolygon_3d_t = boost::geometry::model::multi_polygon<polygon_3d_t>;
using box_3d_t = boost::geometry::model::box<point_3d_t>;

// Variant type to hold any supported geometry
using geometry_variant = std::variant<
    point_t,
    linestring_t,
    polygon_t,
    multipoint_t,
    multilinestring_t,
    multipolygon_t
>;

// Variant type to hold any supported 3D geometry
using geometry_3d_variant = std::variant<
    point_3d_t,
    linestring_3d_t,
    polygon_3d_t,
    multipoint_3d_t,
    multilinestring_3d_t,
    multipolygon_3d_t
>;

// Geometry type enumeration
enum class GeometryType {
    Unknown,
    Point,
    LineString,
    Polygon,
    MultiPoint,
    MultiLineString,
    MultiPolygon
};

/**
 * @brief Wrapper class for managing geometry data with SRID support.
 * 
 * This class stores geometry as WKT string internally and manages SRID metadata.
 * It supports both standard WKT and PostGIS-compatible EWKT formats.
 */
class GeometryWrapper {
public:
    // Default constructor: empty geometry with SRID=-1 (undefined)
    GeometryWrapper() : wkt_(), srid_(-1), dimension_(DimensionType::XY) {}
    
    // Constructor from WKT and optional SRID
    GeometryWrapper(std::string wkt, int srid = -1, DimensionType dimension = DimensionType::XY) 
        : wkt_(std::move(wkt)), srid_(srid), dimension_(dimension) {}
    
    /**
     * @brief Parse EWKT string and extract SRID.
     * 
     * Format: "SRID=4326;POINT(139.69 35.68)" -> wkt="POINT(...)", srid=4326
     * If no SRID prefix, defaults to SRID=0.
     * 
     * @param ewkt EWKT or WKT string
     * @return GeometryWrapper if parsing succeeds, nullopt otherwise
     */
    static std::optional<GeometryWrapper> from_ewkt(const std::string& ewkt);
    
    /**
     * @brief Parse standard WKT string with explicit SRID.
     * 
     * @param wkt WKT string without SRID prefix
     * @param srid SRID value (default -1 = undefined)
     * @return GeometryWrapper if WKT is valid, nullopt otherwise
     */
    static std::optional<GeometryWrapper> from_wkt(const std::string& wkt, int srid = -1);
    
    /**
     * @brief Parse EWKB (Extended Well-Known Binary) with embedded SRID.
     * 
     * @param ewkb Binary data in EWKB format (PostGIS compatible)
     * @return GeometryWrapper if parsing succeeds, nullopt otherwise
     */
    static std::optional<GeometryWrapper> from_ewkb(const std::vector<uint8_t>& ewkb);
    
    /**
     * @brief Generate EWKT string with SRID prefix.
     * 
     * @return EWKT string like "SRID=4326;POINT(139.69 35.68)"
     */
    std::string to_ewkt() const;
    
    /**
     * @brief Convert to EWKB (Extended Well-Known Binary) format with SRID.
     * 
     * @return Binary data in EWKB format (PostGIS compatible)
     */
    std::vector<uint8_t> to_ewkb() const;
    
    /**
     * @brief Get WKT string without SRID information.
     * 
     * @return WKT string
     */
    const std::string& to_wkt() const { return wkt_; }
    
    /**
     * @brief Get SRID value.
     * 
     * @return SRID (-1 means undefined/not set)
     */
    int srid() const { return srid_; }
    
    /**
     * @brief Set SRID value (does NOT perform coordinate transformation).
     * 
     * @param srid New SRID value
     */
    void set_srid(int srid) { srid_ = srid; }
    
    /**
     * @brief Check if geometry is empty.
     * 
     * @return true if WKT string is empty
     */
    bool is_empty() const { return wkt_.empty(); }
    
    /**
     * @brief Get dimension type (XY, XYZ, XYM, XYZM).
     * 
     * @return Dimension type of the geometry
     */
    DimensionType dimension() const { return dimension_; }
    
    /**
     * @brief Check if geometry has 3D coordinates (Z).
     * 
     * @return true if geometry has Z coordinates
     */
    bool is_3d() const { 
        return dimension_ == DimensionType::XYZ || dimension_ == DimensionType::XYZM; 
    }
    
    /**
     * @brief Check if geometry has M coordinates (measured).
     * 
     * @return true if geometry has M coordinates
     */
    bool has_m() const { 
        return dimension_ == DimensionType::XYM || dimension_ == DimensionType::XYZM; 
    }
    
    /**
     * @brief Get coordinate dimension count (2, 3, or 4).
     * 
     * @return Number of dimensions: 2 (XY), 3 (XYZ or XYM), 4 (XYZM)
     */
    int coord_dimension() const { 
        switch (dimension_) {
            case DimensionType::XY: return 2;
            case DimensionType::XYZ: return 3;
            case DimensionType::XYM: return 3;
            case DimensionType::XYZM: return 4;
            default: return 2;
        }
    }
    
    /**
     * @brief Get Z coordinate from a Point geometry.
     * 
     * @return Z coordinate value if geometry is a 3D Point, nullopt otherwise
     */
    std::optional<double> get_z() const;
    
    /**
     * @brief Convert 3D geometry to 2D by removing Z coordinates.
     * 
     * @return 2D version of the geometry
     */
    GeometryWrapper force_2d() const;
    
    /**
     * @brief Convert 2D geometry to 3D by adding Z coordinates.
     * 
     * @param z_default Default Z value to use (default 0.0)
     * @return 3D version of the geometry
     */
    GeometryWrapper force_3d(double z_default = 0.0) const;
    
    /**
     * @brief Detect geometry type from WKT string.
     * 
     * @return GeometryType enum value
     */
    GeometryType geometry_type() const;
    
    /**
     * @brief Get geometry type name compatible with PostGIS.
     * 
     * @return Type name like "ST_Point", "ST_Polygon", etc.
     */
    std::string geometry_type_name() const;
    
    /**
     * @brief Parse WKT into specific Boost.Geometry type.
     * 
     * Template parameter must match the actual geometry type in WKT.
     * 
     * @tparam BoostGeomType Target Boost.Geometry type
     * @return Parsed geometry if successful, nullopt otherwise
     */
    template<typename BoostGeomType>
    std::optional<BoostGeomType> as() const;
    
    /**
     * @brief Parse WKT into geometry_variant.
     * 
     * Automatically detects the geometry type and returns appropriate variant.
     * 
     * @return geometry_variant if parsing succeeds, nullopt otherwise
     */
    std::optional<geometry_variant> as_variant() const;
    
    /**
     * @brief Parse WKT into geometry_3d_variant (3D geometries).
     * 
     * Automatically detects the geometry type and returns appropriate 3D variant.
     * 
     * @return geometry_3d_variant if parsing succeeds, nullopt otherwise
     */
    std::optional<geometry_3d_variant> as_3d_variant() const;
    
    /**
     * @brief Get minimum X coordinate of bounding box.
     * 
     * @return Minimum X value, or nullopt if geometry is empty
     */
    std::optional<double> x_min() const;
    
    /**
     * @brief Get maximum X coordinate of bounding box.
     * 
     * @return Maximum X value, or nullopt if geometry is empty
     */
    std::optional<double> x_max() const;
    
    /**
     * @brief Get minimum Y coordinate of bounding box.
     * 
     * @return Minimum Y value, or nullopt if geometry is empty
     */
    std::optional<double> y_min() const;
    
    /**
     * @brief Get maximum Y coordinate of bounding box.
     * 
     * @return Maximum Y value, or nullopt if geometry is empty
     */
    std::optional<double> y_max() const;
    
    /**
     * @brief Get minimum Z coordinate of bounding box.
     * 
     * @return Minimum Z value for 3D geometries, or nullopt if 2D or empty
     */
    std::optional<double> z_min() const;
    
    /**
     * @brief Get maximum Z coordinate of bounding box.
     * 
     * @return Maximum Z value for 3D geometries, or nullopt if 2D or empty
     */
    std::optional<double> z_max() const;
    
    /**
     * @brief Get bounding box as POLYGON geometry (envelope).
     * 
     * Creates a rectangular POLYGON representing the min/max coordinates.
     * For 3D geometries, only X and Y are used (Z is ignored).
     * 
     * @return Envelope polygon with same SRID, or nullopt if geometry is empty
     */
    std::optional<GeometryWrapper> envelope() const;
    
    /**
     * @brief Get bounding box as "BOX(minX minY, maxX maxY)" string.
     * 
     * PostGIS-compatible format for representing extent.
     * 
     * @return Extent string, or nullopt if geometry is empty
     */
    std::optional<std::string> extent() const;

private:
    std::string wkt_;         // WKT string (without SRID prefix)
    int srid_;                // SRID value (-1 = undefined/not set)
    DimensionType dimension_; // Dimension type: XY, XYZ, XYM, or XYZM
};

// Geometry type name conversion
std::string geometry_type_to_string(GeometryType type);
GeometryType string_to_geometry_type(const std::string& type_name);

} // namespace sqlitegis
