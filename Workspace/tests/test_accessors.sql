-- ============================================================================
-- SqliteGIS Test Suite: Accessor Functions
-- ============================================================================

.load build/sqlitegis.dylib

.print "===================================="
.print "Test Suite: Accessor Functions"
.print "===================================="
.print ""

-- ----------------------------------------------------------------------------
-- Test 1: ST_AsText - WKT output
-- ----------------------------------------------------------------------------
.print "Test 1: ST_AsText - POINT"
SELECT ST_AsText('SRID=4326;POINT(10 20)');
-- Expected: POINT(10 20)

.print ""
.print "Test 2: ST_AsText - LINESTRING"
SELECT ST_AsText('LINESTRING(0 0, 10 10, 20 0)');
-- Expected: LINESTRING(0 0, 10 10, 20 0)

.print ""
.print "Test 3: ST_AsText - POLYGON"
SELECT ST_AsText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))

.print ""
.print "Test 4: ST_AsText - Should strip SRID"
SELECT ST_AsText('SRID=4326;POINT(139.6917 35.6895)');
-- Expected: POINT(139.6917 35.6895) (no SRID prefix)

-- ----------------------------------------------------------------------------
-- Test 5: ST_AsEWKT - Extended WKT output
-- ----------------------------------------------------------------------------
.print ""
.print "Test 5: ST_AsEWKT - POINT with SRID"
SELECT ST_AsEWKT('SRID=4326;POINT(139.6917 35.6895)');
-- Expected: SRID=4326;POINT(139.6917 35.6895)

.print ""
.print "Test 6: ST_AsEWKT - Geometry without SRID (should add SRID=0)"
SELECT ST_AsEWKT('POINT(10 20)');
-- Expected: SRID=0;POINT(10 20)

.print ""
.print "Test 7: ST_AsEWKT - MULTIPOLYGON"
SELECT ST_AsEWKT('SRID=3857;MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)))');
-- Expected: SRID=3857;MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)))

-- ----------------------------------------------------------------------------
-- Test 8: ST_GeometryType - Type detection
-- ----------------------------------------------------------------------------
.print ""
.print "Test 8: ST_GeometryType - POINT"
SELECT ST_GeometryType('POINT(0 0)');
-- Expected: ST_Point

.print ""
.print "Test 9: ST_GeometryType - LINESTRING"
SELECT ST_GeometryType('LINESTRING(0 0, 10 10)');
-- Expected: ST_LineString

.print ""
.print "Test 10: ST_GeometryType - POLYGON"
SELECT ST_GeometryType('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: ST_Polygon

.print ""
.print "Test 11: ST_GeometryType - MULTIPOINT"
SELECT ST_GeometryType('MULTIPOINT((0 0), (10 10))');
-- Expected: ST_MultiPoint

.print ""
.print "Test 12: ST_GeometryType - MULTILINESTRING"
SELECT ST_GeometryType('MULTILINESTRING((0 0, 10 10), (20 20, 30 30))');
-- Expected: ST_MultiLineString

.print ""
.print "Test 13: ST_GeometryType - MULTIPOLYGON"
SELECT ST_GeometryType('MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)))');
-- Expected: ST_MultiPolygon

-- ----------------------------------------------------------------------------
-- Test 14: ST_SRID - SRID extraction
-- ----------------------------------------------------------------------------
.print ""
.print "Test 14: ST_SRID - Extract SRID=4326"
SELECT ST_SRID('SRID=4326;POINT(139.69 35.68)');
-- Expected: 4326

.print ""
.print "Test 15: ST_SRID - Extract SRID=3857"
SELECT ST_SRID('SRID=3857;POLYGON((0 0, 100 0, 100 100, 0 100, 0 0))');
-- Expected: 3857

.print ""
.print "Test 16: ST_SRID - Default SRID (no prefix)"
SELECT ST_SRID('POINT(0 0)');
-- Expected: 0

.print ""
.print "Test 17: ST_SRID - Large SRID value"
SELECT ST_SRID('SRID=32633;POINT(500000 4500000)');
-- Expected: 32633

-- ----------------------------------------------------------------------------
-- Test 18: ST_X - X coordinate extraction
-- ----------------------------------------------------------------------------
.print ""
.print "Test 18: ST_X - Extract X from POINT"
SELECT ST_X('POINT(139.6917 35.6895)');
-- Expected: 139.6917

.print ""
.print "Test 19: ST_X - Negative coordinate"
SELECT ST_X('POINT(-122.4194 37.7749)');
-- Expected: -122.4194

.print ""
.print "Test 20: ST_X - Zero coordinate"
SELECT ST_X('POINT(0 100)');
-- Expected: 0

.print ""
.print "Test 21: ST_X - With SRID (should ignore SRID)"
SELECT ST_X('SRID=4326;POINT(10 20)');
-- Expected: 10

-- ----------------------------------------------------------------------------
-- Test 22: ST_Y - Y coordinate extraction
-- ----------------------------------------------------------------------------
.print ""
.print "Test 22: ST_Y - Extract Y from POINT"
SELECT ST_Y('POINT(139.6917 35.6895)');
-- Expected: 35.6895

.print ""
.print "Test 23: ST_Y - Negative coordinate"
SELECT ST_Y('POINT(-122.4194 37.7749)');
-- Expected: 37.7749

.print ""
.print "Test 24: ST_Y - Zero coordinate"
SELECT ST_Y('POINT(100 0)');
-- Expected: 0

.print ""
.print "Test 25: ST_Y - With SRID (should ignore SRID)"
SELECT ST_Y('SRID=4326;POINT(10 20)');
-- Expected: 20

-- ----------------------------------------------------------------------------
-- Test 26: ST_X/ST_Y Error handling - Non-POINT geometries
-- ----------------------------------------------------------------------------
.print ""
.print "Test 26: ST_X - LINESTRING (should error)"
SELECT ST_X('LINESTRING(0 0, 10 10)');
-- Expected: Error

.print ""
.print "Test 27: ST_Y - POLYGON (should error)"
SELECT ST_Y('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: Error

-- ----------------------------------------------------------------------------
-- Test 28: NULL handling
-- ----------------------------------------------------------------------------
.print ""
.print "Test 28: ST_AsText - NULL input"
SELECT ST_AsText(NULL);
-- Expected: (empty/null)

.print ""
.print "Test 29: ST_GeometryType - NULL input"
SELECT ST_GeometryType(NULL);
-- Expected: (empty/null)

.print ""
.print "Test 30: ST_SRID - NULL input"
SELECT ST_SRID(NULL);
-- Expected: (empty/null)

.print ""
.print "Test 31: ST_X - NULL input"
SELECT ST_X(NULL);
-- Expected: (empty/null)

-- ----------------------------------------------------------------------------
-- Test 32: Combined accessor tests
-- ----------------------------------------------------------------------------
.print ""
.print "Test 32: Extract all properties from a POINT"
SELECT 
    ST_AsText(ST_MakePoint(139.6917, 35.6895)) AS wkt,
    ST_GeometryType(ST_MakePoint(139.6917, 35.6895)) AS type,
    ST_X(ST_MakePoint(139.6917, 35.6895)) AS x,
    ST_Y(ST_MakePoint(139.6917, 35.6895)) AS y;
-- Expected: wkt=POINT(139.6917 35.6895), type=ST_Point, x=139.6917, y=35.6895

.print ""
.print "Test 33: Verify SRID preservation through operations"
SELECT 
    ST_SRID(ST_SetSRID('POINT(10 20)', 4326)) AS srid,
    ST_AsEWKT(ST_SetSRID('POINT(10 20)', 4326)) AS ewkt;
-- Expected: srid=4326, ewkt=SRID=4326;POINT(10 20)

.print ""
.print "===================================="
.print "Accessor Tests Complete"
.print "===================================="
