-- ============================================================================
-- SqliteGIS Test Suite: Spatial Operations & Utilities
-- ============================================================================

.load build/sqlitegis.dylib

.print "===================================="
.print "Test Suite: Operations & Utilities"
.print "===================================="
.print ""

-- ----------------------------------------------------------------------------
-- Test 1: ST_Centroid - Center point calculation
-- ----------------------------------------------------------------------------
.print "Test 1: ST_Centroid - Square centered at origin"
SELECT ST_AsText(ST_Centroid('POLYGON((-5 -5, 5 -5, 5 5, -5 5, -5 -5))'));
-- Expected: POINT(0 0)

.print ""
.print "Test 2: ST_Centroid - Square (0,0 to 10,10)"
SELECT ST_AsText(ST_Centroid('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
-- Expected: POINT(5 5)

.print ""
.print "Test 3: ST_Centroid - Rectangle"
SELECT ST_AsText(ST_Centroid('POLYGON((0 0, 20 0, 20 10, 0 10, 0 0))'));
-- Expected: POINT(10 5)

.print ""
.print "Test 4: ST_Centroid - Triangle"
SELECT ST_AsText(ST_Centroid('POLYGON((0 0, 10 0, 5 10, 0 0))'));
-- Expected: POINT(5 3.333...) approximately

.print ""
.print "Test 5: ST_Centroid - LINESTRING"
SELECT ST_AsText(ST_Centroid('LINESTRING(0 0, 10 10)'));
-- Expected: POINT(5 5)

.print ""
.print "Test 6: ST_Centroid - POINT (should return itself)"
SELECT ST_AsText(ST_Centroid('POINT(10 20)'));
-- Expected: POINT(10 20)

.print ""
.print "Test 7: ST_Centroid - MULTIPOLYGON"
SELECT ST_AsText(ST_Centroid('MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)), ((20 0, 30 0, 30 10, 20 10, 20 0)))'));
-- Expected: Weighted centroid of both polygons

.print ""
.print "Test 8: ST_Centroid - SRID preservation"
SELECT ST_AsEWKT(ST_Centroid('SRID=4326;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
-- Expected: SRID=4326;POINT(5 5)

-- ----------------------------------------------------------------------------
-- Test 9: ST_Buffer - Buffer zone generation
-- ----------------------------------------------------------------------------
.print ""
.print "Test 9: ST_Buffer - POINT with distance 10"
SELECT ST_GeometryType(ST_Buffer('POINT(0 0)', 10));
-- Expected: ST_Polygon

.print ""
.print "Test 10: ST_Buffer - Verify buffer creates polygon around point"
SELECT 
    ST_Contains(ST_Buffer('POINT(0 0)', 10), 'POINT(5 0)') AS contains_inside,
    ST_Contains(ST_Buffer('POINT(0 0)', 10), 'POINT(15 0)') AS contains_outside;
-- Expected: contains_inside=1, contains_outside=0

.print ""
.print "Test 11: ST_Buffer - POINT buffer area approximation"
SELECT ROUND(ST_Area(ST_Buffer('POINT(0 0)', 10)), 1) AS area;
-- Expected: ~314.2 (π * 10²)

.print ""
.print "Test 12: ST_Buffer - Negative buffer (erosion)"
SELECT ST_GeometryType(ST_Buffer('POLYGON((0 0, 20 0, 20 20, 0 20, 0 0))', -2));
-- Expected: ST_Polygon (smaller than original)

.print ""
.print "Test 13: ST_Buffer - LINESTRING buffer"
SELECT ST_GeometryType(ST_Buffer('LINESTRING(0 0, 10 0)', 5));
-- Expected: ST_Polygon (corridor around line)

.print ""
.print "Test 14: ST_Buffer - Zero distance (should return similar geometry)"
SELECT ST_AsText(ST_Buffer('POINT(5 5)', 0));
-- Expected: Similar to original or empty

.print ""
.print "Test 15: ST_Buffer - SRID preservation"
SELECT ST_SRID(ST_Buffer('SRID=4326;POINT(139.69 35.68)', 0.1));
-- Expected: 4326

.print ""
.print "Test 16: ST_Buffer - Large buffer around POLYGON"
SELECT ST_GeometryType(ST_Buffer('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 20));
-- Expected: ST_Polygon

-- ----------------------------------------------------------------------------
-- Test 17: ST_IsValid - Geometry validation
-- ----------------------------------------------------------------------------
.print ""
.print "Test 17: ST_IsValid - Valid POINT"
SELECT ST_IsValid('POINT(10 20)');
-- Expected: 1 (true)

.print ""
.print "Test 18: ST_IsValid - Valid LINESTRING"
SELECT ST_IsValid('LINESTRING(0 0, 10 10, 20 0)');
-- Expected: 1 (true)

.print ""
.print "Test 19: ST_IsValid - Valid POLYGON"
SELECT ST_IsValid('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 1 (true)

.print ""
.print "Test 20: ST_IsValid - Valid MULTIPOINT"
SELECT ST_IsValid('MULTIPOINT((0 0), (10 10))');
-- Expected: 1 (true)

.print ""
.print "Test 21: ST_IsValid - Self-intersecting POLYGON (bowtie)"
SELECT ST_IsValid('POLYGON((0 0, 10 10, 10 0, 0 10, 0 0))');
-- Expected: 0 (false) - self-intersecting

.print ""
.print "Test 22: ST_IsValid - Unclosed POLYGON (invalid WKT would fail parsing)"
-- Note: This would fail at parsing stage, not validation
.print "SKIP - Invalid WKT fails at parse time"

.print ""
.print "Test 23: ST_IsValid - POLYGON with duplicate consecutive points"
SELECT ST_IsValid('POLYGON((0 0, 10 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 1 or 0 (depends on validator strictness)

-- ----------------------------------------------------------------------------
-- Test 24: ST_IsEmpty - Empty geometry detection
-- ----------------------------------------------------------------------------
.print ""
.print "Test 24: ST_IsEmpty - Non-empty POINT"
SELECT ST_IsEmpty('POINT(10 20)');
-- Expected: 0 (false)

.print ""
.print "Test 25: ST_IsEmpty - Non-empty LINESTRING"
SELECT ST_IsEmpty('LINESTRING(0 0, 10 10)');
-- Expected: 0 (false)

.print ""
.print "Test 26: ST_IsEmpty - Non-empty POLYGON"
SELECT ST_IsEmpty('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 0 (false)

.print ""
.print "Test 27: ST_IsEmpty - MULTIPOINT with points"
SELECT ST_IsEmpty('MULTIPOINT((0 0), (10 10))');
-- Expected: 0 (false)

.print ""
.print "Test 28: ST_IsEmpty - Empty MULTIPOINT"
SELECT ST_IsEmpty('MULTIPOINT EMPTY');
-- Expected: 1 (true) - if parser supports EMPTY syntax

.print ""
.print "Test 29: ST_IsEmpty - Empty GEOMETRYCOLLECTION"
SELECT ST_IsEmpty('GEOMETRYCOLLECTION EMPTY');
-- Expected: 1 (true) - if supported

-- ----------------------------------------------------------------------------
-- Test 30: Combined operations
-- ----------------------------------------------------------------------------
.print ""
.print "Test 30: Create buffer and calculate its area"
SELECT 
    ST_AsText(ST_Centroid('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))')) AS centroid,
    ROUND(ST_Area(ST_Buffer(ST_Centroid('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'), 5)), 1) AS buffer_area;
-- Expected: centroid=POINT(5 5), buffer_area≈78.5 (π*5²)

.print ""
.print "Test 31: Validate buffered geometry"
SELECT ST_IsValid(ST_Buffer('POINT(0 0)', 10));
-- Expected: 1 (true)

.print ""
.print "Test 32: Check if buffered point contains original point"
SELECT ST_Contains(ST_Buffer('POINT(0 0)', 10), 'POINT(0 0)');
-- Expected: 1 (true)

.print ""
.print "Test 33: Buffer around centroid of complex polygon"
SELECT ST_GeometryType(
    ST_Buffer(
        ST_Centroid('POLYGON((0 0, 20 5, 15 15, 5 10, 0 0))'),
        3
    )
);
-- Expected: ST_Polygon

-- ----------------------------------------------------------------------------
-- Test 34: NULL handling
-- ----------------------------------------------------------------------------
.print ""
.print "Test 34: ST_Centroid - NULL input"
SELECT ST_Centroid(NULL);
-- Expected: (empty/null)

.print ""
.print "Test 35: ST_Buffer - NULL geometry"
SELECT ST_Buffer(NULL, 10);
-- Expected: (empty/null)

.print ""
.print "Test 36: ST_IsValid - NULL input"
SELECT ST_IsValid(NULL);
-- Expected: (empty/null)

.print ""
.print "Test 37: ST_IsEmpty - NULL input"
SELECT ST_IsEmpty(NULL);
-- Expected: (empty/null)

-- ----------------------------------------------------------------------------
-- Test 38: Real-world scenario
-- ----------------------------------------------------------------------------
.print ""
.print "Test 38: Create 100m buffer around building and validate"
SELECT 
    ST_IsValid(ST_Buffer('SRID=32654;POLYGON((500000 4000000, 500050 4000000, 500050 4000030, 500000 4000030, 500000 4000000))', 100)) AS is_valid,
    ST_GeometryType(ST_Buffer('SRID=32654;POLYGON((500000 4000000, 500050 4000000, 500050 4000030, 500000 4000030, 500000 4000000))', 100)) AS type;
-- Expected: is_valid=1, type=ST_Polygon

.print ""
.print "Test 39: Find centroid of buffered zone"
SELECT ST_AsText(
    ST_Centroid(
        ST_Buffer('POINT(139.6917 35.6895)', 0.01)
    )
);
-- Expected: POINT close to (139.6917 35.6895)

.print ""
.print "Test 40: Verify all geometries in a set are valid"
SELECT 
    ST_IsValid('POINT(0 0)') AND
    ST_IsValid('LINESTRING(0 0, 10 10)') AND
    ST_IsValid('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS all_valid;
-- Expected: 1 (true)

.print ""
.print "===================================="
.print "Operations & Utilities Tests Complete"
.print "===================================="
