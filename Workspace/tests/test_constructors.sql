-- ============================================================================
-- SqliteGIS Test Suite: Constructor Functions
-- ============================================================================

.load build/sqlitegis.dylib

.print "===================================="
.print "Test Suite: Constructor Functions"
.print "===================================="
.print ""

-- ----------------------------------------------------------------------------
-- Test 1: ST_GeomFromText - Basic WKT parsing
-- ----------------------------------------------------------------------------
.print "Test 1: ST_GeomFromText - POINT"
SELECT ST_AsEWKT(ST_GeomFromText('POINT(10 20)'));
-- Expected: SRID=0;POINT(10 20)

.print ""
.print "Test 2: ST_GeomFromText - LINESTRING"
SELECT ST_AsEWKT(ST_GeomFromText('LINESTRING(0 0, 10 10, 20 0)'));
-- Expected: SRID=0;LINESTRING(0 0, 10 10, 20 0)

.print ""
.print "Test 3: ST_GeomFromText - POLYGON"
SELECT ST_AsEWKT(ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'));
-- Expected: SRID=0;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))

.print ""
.print "Test 4: ST_GeomFromText - MULTIPOINT"
SELECT ST_AsEWKT(ST_GeomFromText('MULTIPOINT((0 0), (10 10))'));
-- Expected: SRID=0;MULTIPOINT((0 0), (10 10))

-- ----------------------------------------------------------------------------
-- Test 5: ST_GeomFromText with SRID
-- ----------------------------------------------------------------------------
.print ""
.print "Test 5: ST_GeomFromText - POINT with SRID=4326"
SELECT ST_AsEWKT(ST_GeomFromText('POINT(139.6917 35.6895)', 4326));
-- Expected: SRID=4326;POINT(139.6917 35.6895)

.print ""
.print "Test 6: ST_GeomFromText - POLYGON with SRID=3857"
SELECT ST_AsEWKT(ST_GeomFromText('POLYGON((0 0, 100 0, 100 100, 0 100, 0 0))', 3857));
-- Expected: SRID=3857;POLYGON((0 0, 100 0, 100 100, 0 100, 0 0))

-- ----------------------------------------------------------------------------
-- Test 7: ST_GeomFromEWKT - Extended WKT parsing
-- ----------------------------------------------------------------------------
.print ""
.print "Test 7: ST_GeomFromEWKT - POINT with SRID"
SELECT ST_AsEWKT(ST_GeomFromEWKT('SRID=4326;POINT(139.6917 35.6895)'));
-- Expected: SRID=4326;POINT(139.6917 35.6895)

.print ""
.print "Test 8: ST_GeomFromEWKT - LINESTRING with SRID"
SELECT ST_AsEWKT(ST_GeomFromEWKT('SRID=3857;LINESTRING(0 0, 1000 1000)'));
-- Expected: SRID=3857;LINESTRING(0 0, 1000 1000)

.print ""
.print "Test 9: ST_GeomFromEWKT - Extract SRID"
SELECT ST_SRID(ST_GeomFromEWKT('SRID=4326;POINT(100 50)'));
-- Expected: 4326

-- ----------------------------------------------------------------------------
-- Test 10: ST_MakePoint - Point construction
-- ----------------------------------------------------------------------------
.print ""
.print "Test 10: ST_MakePoint - Simple point"
SELECT ST_AsText(ST_MakePoint(10, 20));
-- Expected: POINT(10 20)

.print ""
.print "Test 11: ST_MakePoint - Decimal coordinates"
SELECT ST_AsText(ST_MakePoint(139.6917, 35.6895));
-- Expected: POINT(139.6917 35.6895)

.print ""
.print "Test 12: ST_MakePoint - Negative coordinates"
SELECT ST_AsText(ST_MakePoint(-122.4194, 37.7749));
-- Expected: POINT(-122.4194 37.7749)

.print ""
.print "Test 13: ST_MakePoint - Default SRID should be 0"
SELECT ST_SRID(ST_MakePoint(0, 0));
-- Expected: 0

-- ----------------------------------------------------------------------------
-- Test 14: ST_SetSRID - SRID modification
-- ----------------------------------------------------------------------------
.print ""
.print "Test 14: ST_SetSRID - Change SRID=0 to SRID=4326"
SELECT ST_AsEWKT(ST_SetSRID('POINT(139.69 35.68)', 4326));
-- Expected: SRID=4326;POINT(139.69 35.68)

.print ""
.print "Test 15: ST_SetSRID - Override existing SRID"
SELECT ST_AsEWKT(ST_SetSRID('SRID=3857;POINT(100 50)', 4326));
-- Expected: SRID=4326;POINT(100 50)

.print ""
.print "Test 16: ST_SetSRID - Verify SRID extraction"
SELECT ST_SRID(ST_SetSRID('POINT(0 0)', 2154));
-- Expected: 2154

.print ""
.print "Test 17: ST_SetSRID - Complex geometry"
SELECT ST_AsEWKT(ST_SetSRID('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 3857));
-- Expected: SRID=3857;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))

-- ----------------------------------------------------------------------------
-- Test 18: NULL handling
-- ----------------------------------------------------------------------------
.print ""
.print "Test 18: ST_GeomFromText - NULL input"
SELECT ST_GeomFromText(NULL);
-- Expected: (empty/null)

.print ""
.print "Test 19: ST_MakePoint - NULL coordinates"
SELECT ST_MakePoint(NULL, 10);
-- Expected: (empty/null)

.print ""
.print "Test 20: ST_SetSRID - NULL geometry"
SELECT ST_SetSRID(NULL, 4326);
-- Expected: (empty/null)

-- ----------------------------------------------------------------------------
-- Test 21: Error handling - Invalid WKT
-- ----------------------------------------------------------------------------
.print ""
.print "Test 21: ST_GeomFromText - Invalid WKT (should error)"
SELECT ST_GeomFromText('INVALID_WKT');
-- Expected: Error or NULL

.print ""
.print "Test 22: ST_GeomFromEWKT - Malformed EWKT (should error)"
SELECT ST_GeomFromEWKT('SRID=INVALID;POINT(0 0)');
-- Expected: Error or NULL

-- ----------------------------------------------------------------------------
-- Test 23: Roundtrip tests
-- ----------------------------------------------------------------------------
.print ""
.print "Test 23: Roundtrip - WKT -> Geometry -> WKT"
SELECT ST_AsText(ST_GeomFromText('POINT(123.456 789.012)'));
-- Expected: POINT(123.456 789.012)

.print ""
.print "Test 24: Roundtrip - EWKT -> Geometry -> EWKT"
SELECT ST_AsEWKT(ST_GeomFromEWKT('SRID=4326;LINESTRING(0 0, 100 100)'));
-- Expected: SRID=4326;LINESTRING(0 0, 100 100)

.print ""
.print "Test 25: Roundtrip - MakePoint -> SetSRID -> AsEWKT"
SELECT ST_AsEWKT(ST_SetSRID(ST_MakePoint(139.69, 35.68), 4326));
-- Expected: SRID=4326;POINT(139.69 35.68)

.print ""
.print "===================================="
.print "Constructor Tests Complete"
.print "===================================="
