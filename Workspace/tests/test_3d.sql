-- SqliteGIS Phase 3: 3D Coordinate Support Tests
-- Test suite for 3D geometry functions

.print "========================================="
.print "Phase 3: 3D Coordinate Support Tests"
.print "========================================="
.print ""

-- =========================================
-- 1. 3D Constructors
-- =========================================
.print "--- 1. 3D Constructors ---"

-- Test 1.1: ST_MakePointZ - Create 3D point
.print "Test 1.1: ST_MakePointZ(10, 20, 30)"
SELECT ST_AsEWKT(ST_MakePointZ(10, 20, 30)) AS result;
-- Expected: SRID=-1;POINT Z (10 20 30)

-- Test 1.2: ST_MakePointZ - Zero Z coordinate
.print "Test 1.2: ST_MakePointZ(1, 2, 0)"
SELECT ST_AsEWKT(ST_MakePointZ(1, 2, 0)) AS result;
-- Expected: SRID=-1;POINT Z (1 2 0)

-- Test 1.3: ST_MakePointZ - Negative Z coordinate
.print "Test 1.3: ST_MakePointZ(0, 0, -100)"
SELECT ST_AsEWKT(ST_MakePointZ(0, 0, -100)) AS result;
-- Expected: SRID=-1;POINT Z (0 0 -100)

-- Test 1.4: ST_GeomFromText - 3D Point
.print "Test 1.4: ST_GeomFromText('POINT Z (1 2 3)')"
SELECT ST_AsEWKT(ST_GeomFromText('POINT Z (1 2 3)')) AS result;
-- Expected: SRID=-1;POINT Z (1 2 3)

-- Test 1.5: ST_GeomFromText - 3D LineString
.print "Test 1.5: ST_GeomFromText('LINESTRING Z (0 0 0, 1 1 1, 2 2 2)')"
SELECT ST_AsText(ST_GeomFromText('LINESTRING Z (0 0 0, 1 1 1, 2 2 2)')) AS result;
-- Expected: LINESTRING Z (0 0 0, 1 1 1, 2 2 2)

-- Test 1.6: ST_GeomFromText - 3D Polygon
.print "Test 1.6: ST_GeomFromText('POLYGON Z ((0 0 0, 10 0 5, 10 10 10, 0 10 5, 0 0 0))')"
SELECT ST_AsText(ST_GeomFromText('POLYGON Z ((0 0 0, 10 0 5, 10 10 10, 0 10 5, 0 0 0))')) AS result;
-- Expected: POLYGON Z ((0 0 0, 10 0 5, 10 10 10, 0 10 5, 0 0 0))

-- Test 1.7: ST_GeomFromEWKT - 3D Point with SRID
.print "Test 1.7: ST_GeomFromEWKT('SRID=4326;POINT Z (139.69 35.68 100)')"
SELECT ST_AsEWKT(ST_GeomFromEWKT('SRID=4326;POINT Z (139.69 35.68 100)')) AS result;
-- Expected: SRID=4326;POINT Z (139.69 35.68 100)

.print ""

-- =========================================
-- 2. 3D Accessors
-- =========================================
.print "--- 2. 3D Accessors ---"

-- Test 2.1: ST_Z - Get Z coordinate from 3D point
.print "Test 2.1: ST_Z('POINT Z (10 20 30)')"
SELECT ST_Z('POINT Z (10 20 30)') AS z_coord;
-- Expected: 30.0

-- Test 2.2: ST_Z - Zero Z coordinate
.print "Test 2.2: ST_Z('POINT Z (1 2 0)')"
SELECT ST_Z('POINT Z (1 2 0)') AS z_coord;
-- Expected: 0.0

-- Test 2.3: ST_Z - Negative Z coordinate
.print "Test 2.3: ST_Z('POINT Z (0 0 -100)')"
SELECT ST_Z('POINT Z (0 0 -100)') AS z_coord;
-- Expected: -100.0

-- Test 2.4: ST_Z - 2D point (should return NULL)
.print "Test 2.4: ST_Z('POINT(10 20)') - 2D point"
SELECT ST_Z('POINT(10 20)') AS z_coord;
-- Expected: NULL

-- Test 2.5: ST_Is3D - 3D point
.print "Test 2.5: ST_Is3D('POINT Z (1 2 3)')"
SELECT ST_Is3D('POINT Z (1 2 3)') AS is_3d;
-- Expected: 1

-- Test 2.6: ST_Is3D - 2D point
.print "Test 2.6: ST_Is3D('POINT(1 2)')"
SELECT ST_Is3D('POINT(1 2)') AS is_3d;
-- Expected: 0

-- Test 2.7: ST_Is3D - 3D LineString
.print "Test 2.7: ST_Is3D('LINESTRING Z (0 0 0, 1 1 1)')"
SELECT ST_Is3D('LINESTRING Z (0 0 0, 1 1 1)') AS is_3d;
-- Expected: 1

-- Test 2.8: ST_CoordDim - 3D point
.print "Test 2.8: ST_CoordDim('POINT Z (1 2 3)')"
SELECT ST_CoordDim('POINT Z (1 2 3)') AS coord_dim;
-- Expected: 3

-- Test 2.9: ST_CoordDim - 2D point
.print "Test 2.9: ST_CoordDim('POINT(1 2)')"
SELECT ST_CoordDim('POINT(1 2)') AS coord_dim;
-- Expected: 2

-- Test 2.10: ST_AsText - 3D Point
.print "Test 2.10: ST_AsText('POINT Z (10 20 30)')"
SELECT ST_AsText('POINT Z (10 20 30)') AS wkt;
-- Expected: POINT Z (10 20 30)

-- Test 2.11: ST_AsEWKT - 3D Point with SRID
.print "Test 2.11: ST_AsEWKT(ST_SetSRID('POINT Z (1 2 3)', 4326))"
SELECT ST_AsEWKT(ST_SetSRID('POINT Z (1 2 3)', 4326)) AS ewkt;
-- Expected: SRID=4326;POINT Z (1 2 3)

.print ""

-- =========================================
-- 3. 3D Measurements
-- =========================================
.print "--- 3. 3D Measurements ---"

-- Test 3.1: ST_3DDistance - Basic 3D distance
.print "Test 3.1: ST_3DDistance('POINT Z (0 0 0)', 'POINT Z (1 1 1)')"
SELECT ST_3DDistance('POINT Z (0 0 0)', 'POINT Z (1 1 1)') AS distance;
-- Expected: 1.7320508075688772 (sqrt(3))

-- Test 3.2: ST_3DDistance - Distance along Z axis
.print "Test 3.2: ST_3DDistance('POINT Z (0 0 0)', 'POINT Z (0 0 10)')"
SELECT ST_3DDistance('POINT Z (0 0 0)', 'POINT Z (0 0 10)') AS distance;
-- Expected: 10.0

-- Test 3.3: ST_3DDistance - Pythagorean triple 3D
.print "Test 3.3: ST_3DDistance('POINT Z (0 0 0)', 'POINT Z (3 4 0)')"
SELECT ST_3DDistance('POINT Z (0 0 0)', 'POINT Z (3 4 0)') AS distance;
-- Expected: 5.0

-- Test 3.4: ST_3DDistance - Complex 3D distance
.print "Test 3.4: ST_3DDistance('POINT Z (1 2 3)', 'POINT Z (4 6 8)')"
SELECT ST_3DDistance('POINT Z (1 2 3)', 'POINT Z (4 6 8)') AS distance;
-- Expected: 7.0710678118654755 (sqrt(50))

-- Test 3.5: ST_3DDistance - Same point
.print "Test 3.5: ST_3DDistance('POINT Z (5 5 5)', 'POINT Z (5 5 5)')"
SELECT ST_3DDistance('POINT Z (5 5 5)', 'POINT Z (5 5 5)') AS distance;
-- Expected: 0.0

-- Test 3.6: ST_3DLength - Simple 3D line
.print "Test 3.6: ST_3DLength('LINESTRING Z (0 0 0, 1 1 1)')"
SELECT ST_3DLength('LINESTRING Z (0 0 0, 1 1 1)') AS length;
-- Expected: 1.7320508075688772 (sqrt(3))

-- Test 3.7: ST_3DLength - Multi-segment 3D line
.print "Test 3.7: ST_3DLength('LINESTRING Z (0 0 0, 1 0 0, 1 1 0, 1 1 1)')"
SELECT ST_3DLength('LINESTRING Z (0 0 0, 1 0 0, 1 1 0, 1 1 1)') AS length;
-- Expected: 3.0

-- Test 3.8: ST_3DLength - Vertical line
.print "Test 3.8: ST_3DLength('LINESTRING Z (0 0 0, 0 0 10)')"
SELECT ST_3DLength('LINESTRING Z (0 0 0, 0 0 10)') AS length;
-- Expected: 10.0

-- Test 3.9: ST_3DPerimeter - 3D triangle
.print "Test 3.9: ST_3DPerimeter('POLYGON Z ((0 0 0, 1 0 0, 0 1 0, 0 0 0))')"
SELECT ST_3DPerimeter('POLYGON Z ((0 0 0, 1 0 0, 0 1 0, 0 0 0))') AS perimeter;
-- Expected: 3.414213562373095 (1 + 1 + sqrt(2))

-- Test 3.10: ST_3DPerimeter - 3D square at different heights
.print "Test 3.10: ST_3DPerimeter('POLYGON Z ((0 0 0, 10 0 5, 10 10 10, 0 10 5, 0 0 0))')"
SELECT ST_3DPerimeter('POLYGON Z ((0 0 0, 10 0 5, 10 10 10, 0 10 5, 0 0 0))') AS perimeter;
-- Expected: ~44.72 (4 * sqrt(125))

.print ""

-- =========================================
-- 4. 2D/3D Conversion
-- =========================================
.print "--- 4. 2D/3D Conversion ---"

-- Test 4.1: ST_Force2D - Convert 3D point to 2D
.print "Test 4.1: ST_Force2D('POINT Z (10 20 30)')"
SELECT ST_AsText(ST_Force2D('POINT Z (10 20 30)')) AS result;
-- Expected: POINT(10 20)

-- Test 4.2: ST_Force2D - 2D point unchanged
.print "Test 4.2: ST_Force2D('POINT(1 2)')"
SELECT ST_AsText(ST_Force2D('POINT(1 2)')) AS result;
-- Expected: POINT(1 2)

-- Test 4.3: ST_Force2D - 3D LineString to 2D
.print "Test 4.3: ST_Force2D('LINESTRING Z (0 0 0, 1 1 1, 2 2 2)')"
SELECT ST_AsText(ST_Force2D('LINESTRING Z (0 0 0, 1 1 1, 2 2 2)')) AS result;
-- Expected: LINESTRING(0 0, 1 1, 2 2)

-- Test 4.4: ST_Force3D - Convert 2D point to 3D (default Z=0)
.print "Test 4.4: ST_Force3D('POINT(10 20)')"
SELECT ST_AsText(ST_Force3D('POINT(10 20)')) AS result;
-- Expected: POINT Z (10 20 0)

-- Test 4.5: ST_Force3D - Convert 2D point to 3D with custom Z
.print "Test 4.5: ST_Force3D('POINT(10 20)', 100)"
SELECT ST_AsText(ST_Force3D('POINT(10 20)', 100)) AS result;
-- Expected: POINT Z (10 20 100)

-- Test 4.6: ST_Force3D - 3D point unchanged
.print "Test 4.6: ST_Force3D('POINT Z (1 2 3)')"
SELECT ST_AsText(ST_Force3D('POINT Z (1 2 3)')) AS result;
-- Expected: POINT Z (1 2 3)

-- Test 4.7: ST_Force3D - 2D LineString to 3D
.print "Test 4.7: ST_Force3D('LINESTRING(0 0, 1 1, 2 2)', 5)"
SELECT ST_AsText(ST_Force3D('LINESTRING(0 0, 1 1, 2 2)', 5)) AS result;
-- Expected: LINESTRING Z (0 0 5, 1 1 5, 2 2 5)

-- Test 4.8: ST_ZMin - Minimum Z coordinate
.print "Test 4.8: ST_ZMin('LINESTRING Z (0 0 10, 1 1 20, 2 2 5)')"
SELECT ST_ZMin('LINESTRING Z (0 0 10, 1 1 20, 2 2 5)') AS z_min;
-- Expected: 5.0

-- Test 4.9: ST_ZMax - Maximum Z coordinate
.print "Test 4.9: ST_ZMax('LINESTRING Z (0 0 10, 1 1 20, 2 2 5)')"
SELECT ST_ZMax('LINESTRING Z (0 0 10, 1 1 20, 2 2 5)') AS z_max;
-- Expected: 20.0

-- Test 4.10: ST_ZMin - 2D geometry (should return NULL)
.print "Test 4.10: ST_ZMin('LINESTRING(0 0, 1 1)') - 2D geometry"
SELECT ST_ZMin('LINESTRING(0 0, 1 1)') AS z_min;
-- Expected: NULL

-- Test 4.11: ST_ZMax - 2D geometry (should return NULL)
.print "Test 4.11: ST_ZMax('LINESTRING(0 0, 1 1)') - 2D geometry"
SELECT ST_ZMax('LINESTRING(0 0, 1 1)') AS z_max;
-- Expected: NULL

.print ""

-- =========================================
-- 5. EWKB 3D Support
-- =========================================
.print "--- 5. EWKB 3D Support ---"

-- Test 5.1: ST_AsEWKB - 3D Point to EWKB
.print "Test 5.1: ST_AsEWKB('SRID=4326;POINT Z (10 20 30)') - Check roundtrip"
SELECT ST_AsEWKT(ST_GeomFromEWKB(ST_AsEWKB('SRID=4326;POINT Z (10 20 30)'))) AS roundtrip;
-- Expected: SRID=4326;POINT Z (10 20 30)

-- Test 5.2: ST_AsEWKB - 3D LineString roundtrip
.print "Test 5.2: EWKB roundtrip for 3D LineString"
SELECT ST_AsText(ST_GeomFromEWKB(ST_AsEWKB('LINESTRING Z (0 0 0, 1 1 1)'))) AS roundtrip;
-- Expected: LINESTRING Z (0 0 0, 1 1 1)

-- Test 5.3: ST_GeomFromEWKB - Verify SRID preservation
.print "Test 5.3: EWKB SRID preservation for 3D geometry"
SELECT ST_SRID(ST_GeomFromEWKB(ST_AsEWKB('SRID=4326;POINT Z (1 2 3)'))) AS srid;
-- Expected: 4326

-- Test 5.4: ST_GeomFromEWKB - Verify 3D flag preservation
.print "Test 5.4: EWKB 3D flag preservation"
SELECT ST_Is3D(ST_GeomFromEWKB(ST_AsEWKB('POINT Z (1 2 3)'))) AS is_3d;
-- Expected: 1

.print ""

-- =========================================
-- 6. Mixed 2D/3D Operations
-- =========================================
.print "--- 6. Mixed 2D/3D Operations ---"

-- Test 6.1: ST_3DDistance - 2D point and 3D point (should work)
.print "Test 6.1: ST_3DDistance('POINT(0 0)', 'POINT Z (0 0 10)')"
SELECT ST_3DDistance('POINT(0 0)', 'POINT Z (0 0 10)') AS distance;
-- Expected: 10.0 (2D treated as Z=0)

-- Test 6.2: ST_Distance vs ST_3DDistance - 2D comparison
.print "Test 6.2: ST_Distance('POINT Z (0 0 0)', 'POINT Z (3 4 0)')"
SELECT ST_Distance('POINT Z (0 0 0)', 'POINT Z (3 4 0)') AS dist_2d,
       ST_3DDistance('POINT Z (0 0 0)', 'POINT Z (3 4 0)') AS dist_3d;
-- Expected: Both should be 5.0

-- Test 6.3: ST_Distance vs ST_3DDistance - Different Z
.print "Test 6.3: ST_Distance vs ST_3DDistance with different Z"
SELECT ST_Distance('POINT Z (0 0 0)', 'POINT Z (3 4 10)') AS dist_2d,
       ST_3DDistance('POINT Z (0 0 0)', 'POINT Z (3 4 10)') AS dist_3d;
-- Expected: dist_2d=5.0, dist_3d=11.18 (sqrt(125))

-- Test 6.4: ST_SetSRID - Works with 3D geometry
.print "Test 6.4: ST_SetSRID('POINT Z (1 2 3)', 4326)"
SELECT ST_AsEWKT(ST_SetSRID('POINT Z (1 2 3)', 4326)) AS result;
-- Expected: SRID=4326;POINT Z (1 2 3)

-- Test 6.5: ST_GeometryType - 3D Point
.print "Test 6.5: ST_GeometryType('POINT Z (1 2 3)')"
SELECT ST_GeometryType('POINT Z (1 2 3)') AS geom_type;
-- Expected: ST_Point

-- Test 6.6: ST_GeometryType - 3D LineString
.print "Test 6.6: ST_GeometryType('LINESTRING Z (0 0 0, 1 1 1)')"
SELECT ST_GeometryType('LINESTRING Z (0 0 0, 1 1 1)') AS geom_type;
-- Expected: ST_LineString

.print ""

-- =========================================
-- 7. Error Handling
-- =========================================
.print "--- 7. Error Handling ---"

-- Test 7.1: ST_Z - Non-point geometry
.print "Test 7.1: ST_Z('LINESTRING Z (0 0 0, 1 1 1)') - Should return NULL or error"
SELECT ST_Z('LINESTRING Z (0 0 0, 1 1 1)') AS result;
-- Expected: NULL (or error)

-- Test 7.2: ST_MakePointZ - NULL arguments
.print "Test 7.2: ST_MakePointZ(NULL, 2, 3) - NULL X"
SELECT ST_MakePointZ(NULL, 2, 3) AS result;
-- Expected: NULL

-- Test 7.3: ST_3DDistance - NULL geometry
.print "Test 7.3: ST_3DDistance(NULL, 'POINT Z (1 2 3)')"
SELECT ST_3DDistance(NULL, 'POINT Z (1 2 3)') AS result;
-- Expected: NULL

-- Test 7.4: ST_Force3D - NULL Z default
.print "Test 7.4: ST_Force3D('POINT(1 2)', NULL)"
SELECT ST_AsText(ST_Force3D('POINT(1 2)', NULL)) AS result;
-- Expected: Error or treat as 0

.print ""

-- =========================================
-- 8. Complex 3D Geometries
-- =========================================
.print "--- 8. Complex 3D Geometries ---"

-- Test 8.1: 3D MultiPoint
.print "Test 8.1: ST_GeomFromText('MULTIPOINT Z ((0 0 0), (1 1 1), (2 2 2))')"
SELECT ST_AsText(ST_GeomFromText('MULTIPOINT Z ((0 0 0), (1 1 1), (2 2 2))')) AS result;
-- Expected: MULTIPOINT Z ((0 0 0), (1 1 1), (2 2 2))

-- Test 8.2: 3D MultiLineString
.print "Test 8.2: ST_GeomFromText('MULTILINESTRING Z ((0 0 0, 1 1 1), (2 2 2, 3 3 3))')"
SELECT ST_AsText(ST_GeomFromText('MULTILINESTRING Z ((0 0 0, 1 1 1), (2 2 2, 3 3 3))')) AS result;
-- Expected: MULTILINESTRING Z ((0 0 0, 1 1 1), (2 2 2, 3 3 3))

-- Test 8.3: 3D MultiPolygon
.print "Test 8.3: ST_Is3D for 3D MultiPolygon"
SELECT ST_Is3D('MULTIPOLYGON Z (((0 0 0, 1 0 0, 1 1 0, 0 1 0, 0 0 0)))') AS is_3d;
-- Expected: 1

-- Test 8.4: ST_3DLength - 3D MultiLineString
.print "Test 8.4: ST_3DLength('MULTILINESTRING Z ((0 0 0, 1 1 1), (2 2 2, 3 3 3))')"
SELECT ST_3DLength('MULTILINESTRING Z ((0 0 0, 1 1 1), (2 2 2, 3 3 3))') AS total_length;
-- Expected: 3.464101615137754 (2 * sqrt(3))

.print ""

-- =========================================
-- Test Summary
-- =========================================
.print "========================================="
.print "Phase 3 Test Suite Complete"
.print "========================================="
.print ""
.print "Test Categories:"
.print "  1. 3D Constructors (7 tests)"
.print "  2. 3D Accessors (11 tests)"
.print "  3. 3D Measurements (10 tests)"
.print "  4. 2D/3D Conversion (11 tests)"
.print "  5. EWKB 3D Support (4 tests)"
.print "  6. Mixed 2D/3D Operations (6 tests)"
.print "  7. Error Handling (4 tests)"
.print "  8. Complex 3D Geometries (4 tests)"
.print ""
.print "Total: 57 test cases"
.print ""
