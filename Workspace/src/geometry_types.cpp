#include "sqlitegis/geometry_types.hpp"

#include <boost/geometry/io/wkt/read.hpp>
#include <boost/geometry/io/wkt/write.hpp>
#include <boost/geometry/algorithms/envelope.hpp>

#include <regex>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iomanip>
#include <sstream>

namespace sqlitegis {

namespace {

// Helper function to trim whitespace
std::string trim(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

// Helper function to convert string to uppercase
std::string to_upper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

} // anonymous namespace

std::optional<GeometryWrapper> GeometryWrapper::from_ewkt(const std::string& ewkt) {
    if (ewkt.empty()) {
        return std::nullopt;
    }
    
    std::string trimmed = trim(ewkt);
    
    // Regular expression to match EWKT format: SRID=<number>;<wkt>
    // Case-insensitive matching
    std::regex ewkt_pattern(R"(^\s*SRID\s*=\s*(\d+)\s*;\s*(.+)$)", 
                           std::regex::icase);
    std::smatch match;
    
    if (std::regex_match(trimmed, match, ewkt_pattern)) {
        // EWKT format detected
        try {
            int srid = std::stoi(match[1].str());
            std::string wkt = trim(match[2].str());
            
            if (wkt.empty()) {
                return std::nullopt;
            }
            
            // Detect dimension type (Z and/or M)
            DimensionType dim = DimensionType::XY;
            std::regex zm_pattern(R"(^\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\s+(Z\s+M|ZM|M\s+Z|MZ)\s+)", 
                                std::regex::icase);
            std::regex z_pattern(R"(^\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\s+Z\s+)", 
                                std::regex::icase);
            std::regex m_pattern(R"(^\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\s+M\s+)", 
                                std::regex::icase);
            
            if (std::regex_search(wkt, zm_pattern)) {
                dim = DimensionType::XYZM;
            } else if (std::regex_search(wkt, z_pattern)) {
                dim = DimensionType::XYZ;
            } else if (std::regex_search(wkt, m_pattern)) {
                dim = DimensionType::XYM;
            }
            
            return GeometryWrapper(wkt, srid, dim);
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    
    // No SRID prefix, treat as standard WKT with SRID=-1 (undefined)
    // Detect dimension type (Z and/or M)
    DimensionType dim = DimensionType::XY;
    std::regex zm_pattern(R"(^\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\s+(Z\s+M|ZM|M\s+Z|MZ)\s+)", 
                        std::regex::icase);
    std::regex z_pattern(R"(^\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\s+Z\s+)", 
                        std::regex::icase);
    std::regex m_pattern(R"(^\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\s+M\s+)", 
                        std::regex::icase);
    
    if (std::regex_search(trimmed, zm_pattern)) {
        dim = DimensionType::XYZM;
    } else if (std::regex_search(trimmed, z_pattern)) {
        dim = DimensionType::XYZ;
    } else if (std::regex_search(trimmed, m_pattern)) {
        dim = DimensionType::XYM;
    }
    
    return GeometryWrapper(trimmed, -1, dim);
}

std::optional<GeometryWrapper> GeometryWrapper::from_wkt(const std::string& wkt, int srid) {
    if (wkt.empty()) {
        return std::nullopt;
    }
    
    std::string trimmed = trim(wkt);
    if (trimmed.empty()) {
        return std::nullopt;
    }
    
    // Detect dimension type (Z and/or M)
    DimensionType dim = DimensionType::XY;
    std::regex zm_pattern(R"(^\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\s+(Z\s+M|ZM|M\s+Z|MZ)\s+)", 
                         std::regex::icase);
    std::regex z_pattern(R"(^\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\s+Z\s+)", 
                         std::regex::icase);
    std::regex m_pattern(R"(^\s*(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)\s+M\s+)", 
                         std::regex::icase);
    
    if (std::regex_search(trimmed, zm_pattern)) {
        dim = DimensionType::XYZM;
    } else if (std::regex_search(trimmed, z_pattern)) {
        dim = DimensionType::XYZ;
    } else if (std::regex_search(trimmed, m_pattern)) {
        dim = DimensionType::XYM;
    }
    
    // Validate WKT format by checking geometry type keywords
    std::string upper_wkt = to_upper(trimmed);
    bool valid = 
        upper_wkt.find("POINT") == 0 ||
        upper_wkt.find("LINESTRING") == 0 ||
        upper_wkt.find("POLYGON") == 0 ||
        upper_wkt.find("MULTIPOINT") == 0 ||
        upper_wkt.find("MULTILINESTRING") == 0 ||
        upper_wkt.find("MULTIPOLYGON") == 0 ||
        upper_wkt.find("GEOMETRYCOLLECTION") == 0;
    
    if (!valid) {
        return std::nullopt;
    }
    
    return GeometryWrapper(trimmed, srid, dim);
}

std::string GeometryWrapper::to_ewkt() const {
    if (wkt_.empty()) {
        return "";
    }
    
    // Always include SRID prefix for EWKT format
    return "SRID=" + std::to_string(srid_) + ";" + wkt_;
}

GeometryType GeometryWrapper::geometry_type() const {
    if (wkt_.empty()) {
        return GeometryType::Unknown;
    }
    
    std::string upper_wkt = to_upper(wkt_);
    
    if (upper_wkt.find("MULTIPOLYGON") == 0) {
        return GeometryType::MultiPolygon;
    } else if (upper_wkt.find("MULTILINESTRING") == 0) {
        return GeometryType::MultiLineString;
    } else if (upper_wkt.find("MULTIPOINT") == 0) {
        return GeometryType::MultiPoint;
    } else if (upper_wkt.find("POLYGON") == 0) {
        return GeometryType::Polygon;
    } else if (upper_wkt.find("LINESTRING") == 0) {
        return GeometryType::LineString;
    } else if (upper_wkt.find("POINT") == 0) {
        return GeometryType::Point;
    }
    
    return GeometryType::Unknown;
}

std::string GeometryWrapper::geometry_type_name() const {
    return geometry_type_to_string(geometry_type());
}

template<typename BoostGeomType>
std::optional<BoostGeomType> GeometryWrapper::as() const {
    if (wkt_.empty()) {
        return std::nullopt;
    }
    
    try {
        BoostGeomType geom;
        boost::geometry::read_wkt(wkt_, geom);
        return geom;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// Explicit template instantiations
template std::optional<point_t> GeometryWrapper::as<point_t>() const;
template std::optional<linestring_t> GeometryWrapper::as<linestring_t>() const;
template std::optional<polygon_t> GeometryWrapper::as<polygon_t>() const;
template std::optional<multipoint_t> GeometryWrapper::as<multipoint_t>() const;
template std::optional<multilinestring_t> GeometryWrapper::as<multilinestring_t>() const;
template std::optional<multipolygon_t> GeometryWrapper::as<multipolygon_t>() const;

std::optional<geometry_variant> GeometryWrapper::as_variant() const {
    if (wkt_.empty()) {
        return std::nullopt;
    }
    
    GeometryType type = geometry_type();
    
    try {
        switch (type) {
            case GeometryType::Point: {
                auto geom = as<point_t>();
                if (geom) return geometry_variant(*geom);
                break;
            }
            case GeometryType::LineString: {
                auto geom = as<linestring_t>();
                if (geom) return geometry_variant(*geom);
                break;
            }
            case GeometryType::Polygon: {
                auto geom = as<polygon_t>();
                if (geom) return geometry_variant(*geom);
                break;
            }
            case GeometryType::MultiPoint: {
                auto geom = as<multipoint_t>();
                if (geom) return geometry_variant(*geom);
                break;
            }
            case GeometryType::MultiLineString: {
                auto geom = as<multilinestring_t>();
                if (geom) return geometry_variant(*geom);
                break;
            }
            case GeometryType::MultiPolygon: {
                auto geom = as<multipolygon_t>();
                if (geom) return geometry_variant(*geom);
                break;
            }
            default:
                break;
        }
    } catch (const std::exception&) {
        return std::nullopt;
    }
    
    return std::nullopt;
}

std::string geometry_type_to_string(GeometryType type) {
    switch (type) {
        case GeometryType::Point:
            return "ST_Point";
        case GeometryType::LineString:
            return "ST_LineString";
        case GeometryType::Polygon:
            return "ST_Polygon";
        case GeometryType::MultiPoint:
            return "ST_MultiPoint";
        case GeometryType::MultiLineString:
            return "ST_MultiLineString";
        case GeometryType::MultiPolygon:
            return "ST_MultiPolygon";
        default:
            return "ST_Unknown";
    }
}

GeometryType string_to_geometry_type(const std::string& type_name) {
    std::string upper_name = to_upper(type_name);
    
    // Remove "ST_" prefix if present
    if (upper_name.find("ST_") == 0) {
        upper_name = upper_name.substr(3);
    }
    
    if (upper_name == "POINT") {
        return GeometryType::Point;
    } else if (upper_name == "LINESTRING") {
        return GeometryType::LineString;
    } else if (upper_name == "POLYGON") {
        return GeometryType::Polygon;
    } else if (upper_name == "MULTIPOINT") {
        return GeometryType::MultiPoint;
    } else if (upper_name == "MULTILINESTRING") {
        return GeometryType::MultiLineString;
    } else if (upper_name == "MULTIPOLYGON") {
        return GeometryType::MultiPolygon;
    }
    
    return GeometryType::Unknown;
}

// WKB/EWKB helper functions
namespace {

// Determine system byte order
bool is_little_endian() {
    uint32_t test = 1;
    return *reinterpret_cast<uint8_t*>(&test) == 1;
}

// Read uint32_t from binary data with specified byte order
uint32_t read_uint32(const uint8_t* data, ByteOrder order) {
    uint32_t value;
    if (order == ByteOrder::LittleEndian) {
        value = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    } else {
        value = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];
    }
    return value;
}

// Write uint32_t to binary data with specified byte order
void write_uint32(std::vector<uint8_t>& buffer, uint32_t value, ByteOrder order) {
    if (order == ByteOrder::LittleEndian) {
        buffer.push_back(value & 0xFF);
        buffer.push_back((value >> 8) & 0xFF);
        buffer.push_back((value >> 16) & 0xFF);
        buffer.push_back((value >> 24) & 0xFF);
    } else {
        buffer.push_back((value >> 24) & 0xFF);
        buffer.push_back((value >> 16) & 0xFF);
        buffer.push_back((value >> 8) & 0xFF);
        buffer.push_back(value & 0xFF);
    }
}

// Read double from binary data with specified byte order
double read_double(const uint8_t* data, ByteOrder order) {
    uint64_t value;
    if (order == ByteOrder::LittleEndian) {
        value = static_cast<uint64_t>(data[0]) |
                (static_cast<uint64_t>(data[1]) << 8) |
                (static_cast<uint64_t>(data[2]) << 16) |
                (static_cast<uint64_t>(data[3]) << 24) |
                (static_cast<uint64_t>(data[4]) << 32) |
                (static_cast<uint64_t>(data[5]) << 40) |
                (static_cast<uint64_t>(data[6]) << 48) |
                (static_cast<uint64_t>(data[7]) << 56);
    } else {
        value = (static_cast<uint64_t>(data[0]) << 56) |
                (static_cast<uint64_t>(data[1]) << 48) |
                (static_cast<uint64_t>(data[2]) << 40) |
                (static_cast<uint64_t>(data[3]) << 32) |
                (static_cast<uint64_t>(data[4]) << 24) |
                (static_cast<uint64_t>(data[5]) << 16) |
                (static_cast<uint64_t>(data[6]) << 8) |
                static_cast<uint64_t>(data[7]);
    }
    double result;
    std::memcpy(&result, &value, sizeof(double));
    return result;
}

// Write double to binary data with specified byte order
void write_double(std::vector<uint8_t>& buffer, double value, ByteOrder order) {
    uint64_t int_value;
    std::memcpy(&int_value, &value, sizeof(double));
    
    if (order == ByteOrder::LittleEndian) {
        buffer.push_back(int_value & 0xFF);
        buffer.push_back((int_value >> 8) & 0xFF);
        buffer.push_back((int_value >> 16) & 0xFF);
        buffer.push_back((int_value >> 24) & 0xFF);
        buffer.push_back((int_value >> 32) & 0xFF);
        buffer.push_back((int_value >> 40) & 0xFF);
        buffer.push_back((int_value >> 48) & 0xFF);
        buffer.push_back((int_value >> 56) & 0xFF);
    } else {
        buffer.push_back((int_value >> 56) & 0xFF);
        buffer.push_back((int_value >> 48) & 0xFF);
        buffer.push_back((int_value >> 40) & 0xFF);
        buffer.push_back((int_value >> 32) & 0xFF);
        buffer.push_back((int_value >> 24) & 0xFF);
        buffer.push_back((int_value >> 16) & 0xFF);
        buffer.push_back((int_value >> 8) & 0xFF);
        buffer.push_back(int_value & 0xFF);
    }
}

// Parse Point from WKB
std::string parse_point_wkb(const uint8_t*& data, ByteOrder order) {
    double x = read_double(data, order);
    data += 8;
    double y = read_double(data, order);
    data += 8;
    
    return "POINT(" + std::to_string(x) + " " + std::to_string(y) + ")";
}

// Parse LineString from WKB
std::string parse_linestring_wkb(const uint8_t*& data, ByteOrder order) {
    uint32_t num_points = read_uint32(data, order);
    data += 4;
    
    std::string wkt = "LINESTRING(";
    for (uint32_t i = 0; i < num_points; ++i) {
        if (i > 0) wkt += ",";
        double x = read_double(data, order);
        data += 8;
        double y = read_double(data, order);
        data += 8;
        wkt += std::to_string(x) + " " + std::to_string(y);
    }
    wkt += ")";
    return wkt;
}

// Parse Polygon from WKB
std::string parse_polygon_wkb(const uint8_t*& data, ByteOrder order) {
    uint32_t num_rings = read_uint32(data, order);
    data += 4;
    
    std::string wkt = "POLYGON(";
    for (uint32_t ring = 0; ring < num_rings; ++ring) {
        if (ring > 0) wkt += ",";
        wkt += "(";
        
        uint32_t num_points = read_uint32(data, order);
        data += 4;
        
        for (uint32_t i = 0; i < num_points; ++i) {
            if (i > 0) wkt += ",";
            double x = read_double(data, order);
            data += 8;
            double y = read_double(data, order);
            data += 8;
            wkt += std::to_string(x) + " " + std::to_string(y);
        }
        wkt += ")";
    }
    wkt += ")";
    return wkt;
}

// Parse MultiPoint from WKB
std::string parse_multipoint_wkb(const uint8_t*& data, ByteOrder order) {
    uint32_t num_points = read_uint32(data, order);
    data += 4;
    
    std::string wkt = "MULTIPOINT(";
    for (uint32_t i = 0; i < num_points; ++i) {
        if (i > 0) wkt += ",";
        
        // Each point has its own byte order and type
        data += 1; // Skip byte order
        data += 4; // Skip type
        
        double x = read_double(data, order);
        data += 8;
        double y = read_double(data, order);
        data += 8;
        wkt += "(" + std::to_string(x) + " " + std::to_string(y) + ")";
    }
    wkt += ")";
    return wkt;
}

// Parse MultiLineString from WKB
std::string parse_multilinestring_wkb(const uint8_t*& data, ByteOrder order) {
    uint32_t num_linestrings = read_uint32(data, order);
    data += 4;
    
    std::string wkt = "MULTILINESTRING(";
    for (uint32_t ls = 0; ls < num_linestrings; ++ls) {
        if (ls > 0) wkt += ",";
        
        // Each linestring has its own byte order and type
        data += 1; // Skip byte order
        data += 4; // Skip type
        
        uint32_t num_points = read_uint32(data, order);
        data += 4;
        
        wkt += "(";
        for (uint32_t i = 0; i < num_points; ++i) {
            if (i > 0) wkt += ",";
            double x = read_double(data, order);
            data += 8;
            double y = read_double(data, order);
            data += 8;
            wkt += std::to_string(x) + " " + std::to_string(y);
        }
        wkt += ")";
    }
    wkt += ")";
    return wkt;
}

// Parse MultiPolygon from WKB
std::string parse_multipolygon_wkb(const uint8_t*& data, ByteOrder order) {
    uint32_t num_polygons = read_uint32(data, order);
    data += 4;
    
    std::string wkt = "MULTIPOLYGON(";
    for (uint32_t poly = 0; poly < num_polygons; ++poly) {
        if (poly > 0) wkt += ",";
        
        // Each polygon has its own byte order and type
        data += 1; // Skip byte order
        data += 4; // Skip type
        
        uint32_t num_rings = read_uint32(data, order);
        data += 4;
        
        wkt += "(";
        for (uint32_t ring = 0; ring < num_rings; ++ring) {
            if (ring > 0) wkt += ",";
            wkt += "(";
            
            uint32_t num_points = read_uint32(data, order);
            data += 4;
            
            for (uint32_t i = 0; i < num_points; ++i) {
                if (i > 0) wkt += ",";
                double x = read_double(data, order);
                data += 8;
                double y = read_double(data, order);
                data += 8;
                wkt += std::to_string(x) + " " + std::to_string(y);
            }
            wkt += ")";
        }
        wkt += ")";
    }
    wkt += ")";
    return wkt;
}

} // anonymous namespace

std::optional<GeometryWrapper> GeometryWrapper::from_ewkb(const std::vector<uint8_t>& ewkb) {
    if (ewkb.size() < 5) {
        return std::nullopt;
    }
    
    try {
        size_t offset = 0;
        
        // Read byte order
        if (offset >= ewkb.size()) return std::nullopt;
        ByteOrder order = static_cast<ByteOrder>(ewkb[offset]);
        if (order != ByteOrder::BigEndian && order != ByteOrder::LittleEndian) {
            return std::nullopt;
        }
        offset += 1;
        
        // Read geometry type
        if (offset + 4 > ewkb.size()) return std::nullopt;
        uint32_t type = read_uint32(&ewkb[offset], order);
        offset += 4;
        
        // Check for SRID flag
        int srid = -1;  // Default to -1 (undefined) if SRID not present
        if (type & SRID_FLAG) {
            // SRID is present
            if (offset + 4 > ewkb.size()) return std::nullopt;
            srid = static_cast<int>(read_uint32(&ewkb[offset], order));
            offset += 4;
            type &= ~SRID_FLAG; // Remove SRID flag from type
        }
        
        // Check for dimension flags (Z and/or M)
        DimensionType dim = DimensionType::XY;
        bool has_z = (type & WKB_Z_FLAG) != 0;
        bool has_m = (type & WKB_M_FLAG) != 0;
        
        if (has_z && has_m) {
            dim = DimensionType::XYZM;
            type &= ~(WKB_Z_FLAG | WKB_M_FLAG);
        } else if (has_z) {
            dim = DimensionType::XYZ;
            type &= ~WKB_Z_FLAG;
        } else if (has_m) {
            dim = DimensionType::XYM;
            type &= ~WKB_M_FLAG;
        }
        
        std::string wkt;
        
        // For now, only support Point type
        if (static_cast<WKBType>(type) == WKBType::Point) {
            // Calculate coordinate size based on dimensions
            size_t coord_size = 16; // XY (2 * 8 bytes)
            if (dim == DimensionType::XYZ || dim == DimensionType::XYM) {
                coord_size = 24; // XYZ or XYM (3 * 8 bytes)
            } else if (dim == DimensionType::XYZM) {
                coord_size = 32; // XYZM (4 * 8 bytes)
            }
            
            if (offset + coord_size > ewkb.size()) return std::nullopt;
            
            double x = read_double(&ewkb[offset], order);
            offset += 8;
            double y = read_double(&ewkb[offset], order);
            offset += 8;
            
            if (dim == DimensionType::XYZ) {
                double z = read_double(&ewkb[offset], order);
                offset += 8;
                wkt = "POINT Z (" + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + ")";
            } else if (dim == DimensionType::XYM) {
                double m = read_double(&ewkb[offset], order);
                offset += 8;
                wkt = "POINT M (" + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(m) + ")";
            } else if (dim == DimensionType::XYZM) {
                double z = read_double(&ewkb[offset], order);
                offset += 8;
                double m = read_double(&ewkb[offset], order);
                offset += 8;
                wkt = "POINT ZM (" + std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z) + " " + std::to_string(m) + ")";
            } else {
                wkt = "POINT(" + std::to_string(x) + " " + std::to_string(y) + ")";
            }
        } else {
            // Other types not yet implemented
            return std::nullopt;
        }
        
        return GeometryWrapper(wkt, srid, dim);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::vector<uint8_t> GeometryWrapper::to_ewkb() const {
    std::vector<uint8_t> result;
    
    // Use little-endian for output (PostGIS default)
    ByteOrder order = ByteOrder::LittleEndian;
    result.push_back(static_cast<uint8_t>(order));
    
    GeometryType type = geometry_type();
    uint32_t wkb_type = 0;
    
    switch (type) {
        case GeometryType::Point:
            wkb_type = static_cast<uint32_t>(WKBType::Point);
            break;
        case GeometryType::LineString:
            wkb_type = static_cast<uint32_t>(WKBType::LineString);
            break;
        case GeometryType::Polygon:
            wkb_type = static_cast<uint32_t>(WKBType::Polygon);
            break;
        case GeometryType::MultiPoint:
            wkb_type = static_cast<uint32_t>(WKBType::MultiPoint);
            break;
        case GeometryType::MultiLineString:
            wkb_type = static_cast<uint32_t>(WKBType::MultiLineString);
            break;
        case GeometryType::MultiPolygon:
            wkb_type = static_cast<uint32_t>(WKBType::MultiPolygon);
            break;
        default:
            return result;
    }
    
    // Add SRID flag to type
    wkb_type |= SRID_FLAG;
    
    // Add dimension flags
    if (dimension_ == DimensionType::XYZ) {
        wkb_type |= WKB_Z_FLAG;
    } else if (dimension_ == DimensionType::XYM) {
        wkb_type |= WKB_M_FLAG;
    } else if (dimension_ == DimensionType::XYZM) {
        wkb_type |= (WKB_Z_FLAG | WKB_M_FLAG);
    }
    
    write_uint32(result, wkb_type, order);
    
    // Write SRID
    write_uint32(result, static_cast<uint32_t>(srid_), order);
    
    // Write coordinates based on geometry type
    if (type == GeometryType::Point) {
        if (dimension_ == DimensionType::XYZ || dimension_ == DimensionType::XYZM) {
            // Parse 3D point for coordinates
            auto pt3d = as_3d_variant();
            if (pt3d && std::holds_alternative<point_3d_t>(*pt3d)) {
                const auto& pt = std::get<point_3d_t>(*pt3d);
                write_double(result, boost::geometry::get<0>(pt), order);
                write_double(result, boost::geometry::get<1>(pt), order);
                write_double(result, boost::geometry::get<2>(pt), order);
                
                // For XYZM, we would need to write M coordinate here
                // But we don't have M coordinate parsing yet, so skip for now
                if (dimension_ == DimensionType::XYZM) {
                    write_double(result, 0.0, order); // Placeholder for M
                }
            }
        } else if (dimension_ == DimensionType::XYM) {
            // XYM not fully supported yet - write as XY
            auto pt = as<point_t>();
            if (pt) {
                write_double(result, pt->x(), order);
                write_double(result, pt->y(), order);
                write_double(result, 0.0, order); // Placeholder for M
            }
        } else {
            auto pt = as<point_t>();
            if (pt) {
                write_double(result, pt->x(), order);
                write_double(result, pt->y(), order);
            }
        }
    }
    // Note: Other geometry types would require more complex serialization
    // For now, we only support Point geometry
    
    return result;
}

// Get Z coordinate from a Point geometry
std::optional<double> GeometryWrapper::get_z() const {
    if ((dimension_ != DimensionType::XYZ && dimension_ != DimensionType::XYZM) || 
        geometry_type() != GeometryType::Point) {
        return std::nullopt;
    }
    
    // Parse the 3D point to extract Z coordinate
    // Format: "POINT Z (x y z)" or "POINT ZM (x y z m)"
    std::regex point_pattern(R"(POINT\s+(Z|ZM)\s*\(\s*([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+))", 
                            std::regex::icase);
    std::smatch match;
    
    if (std::regex_search(wkt_, match, point_pattern)) {
        try {
            double z = std::stod(match[4].str());
            return z;
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    
    return std::nullopt;
}

// Convert 3D geometry to 2D by removing Z coordinates
GeometryWrapper GeometryWrapper::force_2d() const {
    if (dimension_ == DimensionType::XY) {
        // Already 2D without M, return copy
        return *this;
    }
    
    // Remove dimension suffix and coordinates from geometry
    std::string wkt_2d = wkt_;
    
    if (dimension_ == DimensionType::XYZ) {
        // Remove " Z" and Z coordinates
        std::regex z_suffix(R"(\s+Z\s+)", std::regex::icase);
        wkt_2d = std::regex_replace(wkt_2d, z_suffix, " ");
        // Remove Z coordinate: "x y z" -> "x y"
        std::regex coord_pattern(R"(([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+))");
        wkt_2d = std::regex_replace(wkt_2d, coord_pattern, "$1 $2");
    } else if (dimension_ == DimensionType::XYM) {
        // Remove " M" and M coordinates
        std::regex m_suffix(R"(\s+M\s+)", std::regex::icase);
        wkt_2d = std::regex_replace(wkt_2d, m_suffix, " ");
        // Remove M coordinate: "x y m" -> "x y"
        std::regex coord_pattern(R"(([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+))");
        wkt_2d = std::regex_replace(wkt_2d, coord_pattern, "$1 $2");
    } else if (dimension_ == DimensionType::XYZM) {
        // Remove " ZM" and Z, M coordinates
        std::regex zm_suffix(R"(\s+(ZM|Z\s+M|M\s+Z)\s+)", std::regex::icase);
        wkt_2d = std::regex_replace(wkt_2d, zm_suffix, " ");
        // Remove Z and M coordinates: "x y z m" -> "x y"
        std::regex coord_pattern(R"(([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+))");
        wkt_2d = std::regex_replace(wkt_2d, coord_pattern, "$1 $2");
    }
    
    return GeometryWrapper(wkt_2d, srid_, DimensionType::XY);
}

// Convert 2D geometry to 3D by adding Z coordinates
GeometryWrapper GeometryWrapper::force_3d(double z_default) const {
    if (dimension_ == DimensionType::XYZ || dimension_ == DimensionType::XYZM) {
        // Already has Z, return copy
        return *this;
    }
    
    // Add Z to geometry
    std::string wkt_3d = wkt_;
    
    if (dimension_ == DimensionType::XY) {
        // Add " Z" after geometry type
        std::regex geom_type_pattern(R"(^(POINT|LINESTRING|POLYGON|MULTIPOINT|MULTILINESTRING|MULTIPOLYGON|GEOMETRYCOLLECTION)(\s*\())", 
                                    std::regex::icase);
        wkt_3d = std::regex_replace(wkt_3d, geom_type_pattern, "$1 Z$2");
        
        // Add Z coordinate to coordinate tuples: "x y" -> "x y z"
        std::string z_str = " " + std::to_string(z_default);
        std::regex coord_pattern(R"(([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+)(?!\s+[-+]?[0-9]))");
        wkt_3d = std::regex_replace(wkt_3d, coord_pattern, "$1 $2" + z_str);
        
        return GeometryWrapper(wkt_3d, srid_, DimensionType::XYZ);
    } else if (dimension_ == DimensionType::XYM) {
        // Convert XYM to XYZM: change "M" to "ZM" and insert Z before M
        std::regex m_suffix(R"(\s+M\s+)", std::regex::icase);
        wkt_3d = std::regex_replace(wkt_3d, m_suffix, " ZM ");
        
        // Insert Z coordinate: "x y m" -> "x y z m"
        std::string z_str = " " + std::to_string(z_default) + " ";
        std::regex coord_pattern(R"(([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+)\s+([-+]?[0-9]*\.?[0-9]+))");
        wkt_3d = std::regex_replace(wkt_3d, coord_pattern, "$1 $2" + z_str + "$3");
        
        return GeometryWrapper(wkt_3d, srid_, DimensionType::XYZM);
    }
    
    // Already has Z
    return *this;
}

// Parse WKT into geometry_3d_variant (3D geometries)
std::optional<geometry_3d_variant> GeometryWrapper::as_3d_variant() const {
    if (dimension_ != DimensionType::XYZ && dimension_ != DimensionType::XYZM) {
        return std::nullopt;
    }
    
    GeometryType type = geometry_type();
    
    try {
        switch (type) {
            case GeometryType::Point: {
                point_3d_t pt;
                boost::geometry::read_wkt(wkt_, pt);
                return pt;
            }
            case GeometryType::LineString: {
                linestring_3d_t line;
                boost::geometry::read_wkt(wkt_, line);
                return line;
            }
            case GeometryType::Polygon: {
                polygon_3d_t poly;
                boost::geometry::read_wkt(wkt_, poly);
                return poly;
            }
            case GeometryType::MultiPoint: {
                multipoint_3d_t mpt;
                boost::geometry::read_wkt(wkt_, mpt);
                return mpt;
            }
            case GeometryType::MultiLineString: {
                multilinestring_3d_t mline;
                boost::geometry::read_wkt(wkt_, mline);
                return mline;
            }
            case GeometryType::MultiPolygon: {
                multipolygon_3d_t mpoly;
                boost::geometry::read_wkt(wkt_, mpoly);
                return mpoly;
            }
            default:
                return std::nullopt;
        }
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

// Bounding box helper function template
template<typename GeomType>
static box_t get_envelope_2d(const GeomType& geom) {
    box_t bbox;
    boost::geometry::envelope(geom, bbox);
    return bbox;
}

template<typename GeomType>
static box_3d_t get_envelope_3d(const GeomType& geom) {
    box_3d_t bbox;
    boost::geometry::envelope(geom, bbox);
    return bbox;
}

std::optional<double> GeometryWrapper::x_min() const {
    if (is_empty()) return std::nullopt;
    
    namespace bg = boost::geometry;
    
    if (is_3d()) {
        auto geom_var = as_3d_variant();
        if (!geom_var) return std::nullopt;
        
        return std::visit([](const auto& geom) -> double {
            auto bbox = get_envelope_3d(geom);
            return bg::get<bg::min_corner, 0>(bbox);
        }, *geom_var);
    } else {
        auto geom_var = as_variant();
        if (!geom_var) return std::nullopt;
        
        return std::visit([](const auto& geom) -> double {
            auto bbox = get_envelope_2d(geom);
            return bg::get<bg::min_corner, 0>(bbox);
        }, *geom_var);
    }
}

std::optional<double> GeometryWrapper::x_max() const {
    if (is_empty()) return std::nullopt;
    
    namespace bg = boost::geometry;
    
    if (is_3d()) {
        auto geom_var = as_3d_variant();
        if (!geom_var) return std::nullopt;
        
        return std::visit([](const auto& geom) -> double {
            auto bbox = get_envelope_3d(geom);
            return bg::get<bg::max_corner, 0>(bbox);
        }, *geom_var);
    } else {
        auto geom_var = as_variant();
        if (!geom_var) return std::nullopt;
        
        return std::visit([](const auto& geom) -> double {
            auto bbox = get_envelope_2d(geom);
            return bg::get<bg::max_corner, 0>(bbox);
        }, *geom_var);
    }
}

std::optional<double> GeometryWrapper::y_min() const {
    if (is_empty()) return std::nullopt;
    
    namespace bg = boost::geometry;
    
    if (is_3d()) {
        auto geom_var = as_3d_variant();
        if (!geom_var) return std::nullopt;
        
        return std::visit([](const auto& geom) -> double {
            auto bbox = get_envelope_3d(geom);
            return bg::get<bg::min_corner, 1>(bbox);
        }, *geom_var);
    } else {
        auto geom_var = as_variant();
        if (!geom_var) return std::nullopt;
        
        return std::visit([](const auto& geom) -> double {
            auto bbox = get_envelope_2d(geom);
            return bg::get<bg::min_corner, 1>(bbox);
        }, *geom_var);
    }
}

std::optional<double> GeometryWrapper::y_max() const {
    if (is_empty()) return std::nullopt;
    
    namespace bg = boost::geometry;
    
    if (is_3d()) {
        auto geom_var = as_3d_variant();
        if (!geom_var) return std::nullopt;
        
        return std::visit([](const auto& geom) -> double {
            auto bbox = get_envelope_3d(geom);
            return bg::get<bg::max_corner, 1>(bbox);
        }, *geom_var);
    } else {
        auto geom_var = as_variant();
        if (!geom_var) return std::nullopt;
        
        return std::visit([](const auto& geom) -> double {
            auto bbox = get_envelope_2d(geom);
            return bg::get<bg::max_corner, 1>(bbox);
        }, *geom_var);
    }
}

std::optional<double> GeometryWrapper::z_min() const {
    if (is_empty() || !is_3d()) return std::nullopt;
    
    namespace bg = boost::geometry;
    
    auto geom_var = as_3d_variant();
    if (!geom_var) return std::nullopt;
    
    return std::visit([](const auto& geom) -> double {
        auto bbox = get_envelope_3d(geom);
        return bg::get<bg::min_corner, 2>(bbox);
    }, *geom_var);
}

std::optional<double> GeometryWrapper::z_max() const {
    if (is_empty() || !is_3d()) return std::nullopt;
    
    namespace bg = boost::geometry;
    
    auto geom_var = as_3d_variant();
    if (!geom_var) return std::nullopt;
    
    return std::visit([](const auto& geom) -> double {
        auto bbox = get_envelope_3d(geom);
        return bg::get<bg::max_corner, 2>(bbox);
    }, *geom_var);
}

std::optional<GeometryWrapper> GeometryWrapper::envelope() const {
    auto xmin = x_min();
    auto xmax = x_max();
    auto ymin = y_min();
    auto ymax = y_max();
    
    if (!xmin || !xmax || !ymin || !ymax) {
        return std::nullopt;
    }
    
    // Create envelope polygon: BOX(minX minY, maxX maxY) -> POLYGON((minX minY, maxX minY, maxX maxY, minX maxY, minX minY))
    std::ostringstream oss;
    oss << std::setprecision(15);
    oss << "POLYGON(("
        << *xmin << " " << *ymin << ","
        << *xmax << " " << *ymin << ","
        << *xmax << " " << *ymax << ","
        << *xmin << " " << *ymax << ","
        << *xmin << " " << *ymin
        << "))";
    
    return GeometryWrapper(oss.str(), srid_, DimensionType::XY);
}

std::optional<std::string> GeometryWrapper::extent() const {
    auto xmin = x_min();
    auto xmax = x_max();
    auto ymin = y_min();
    auto ymax = y_max();
    
    if (!xmin || !xmax || !ymin || !ymax) {
        return std::nullopt;
    }
    
    std::ostringstream oss;
    oss << std::setprecision(15);
    oss << "BOX(" << *xmin << " " << *ymin << "," << *xmax << " " << *ymax << ")";
    
    return oss.str();
}

} // namespace sqlitegis
