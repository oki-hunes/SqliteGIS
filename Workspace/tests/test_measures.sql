-- ============================================================================
-- SqliteGIS Test Suite: Measurement Functions
-- ============================================================================

.load build/sqlitegis.dylib

.print "===================================="
.print "Test Suite: Measurement Functions"
.print "===================================="
.print ""

-- ----------------------------------------------------------------------------
-- Test 1: ST_Area - Polygon area
-- ----------------------------------------------------------------------------
.print "Test 1: ST_Area - Simple square (10x10)"
SELECT ST_Area('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 100

.print ""
.print "Test 2: ST_Area - Rectangle (20x5)"
SELECT ST_Area('POLYGON((0 0, 20 0, 20 5, 0 5, 0 0))');
-- Expected: 100

.print ""
.print "Test 3: ST_Area - Triangle"
SELECT ST_Area('POLYGON((0 0, 10 0, 5 10, 0 0))');
-- Expected: ~50

.print ""
.print "Test 4: ST_Area - MULTIPOLYGON (two 10x10 squares)"
SELECT ST_Area('MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)), ((20 0, 30 0, 30 10, 20 10, 20 0)))');
-- Expected: 200

.print ""
.print "Test 5: ST_Area - POINT (should be 0)"
SELECT ST_Area('POINT(10 20)');
-- Expected: 0

.print ""
.print "Test 6: ST_Area - LINESTRING (should be 0)"
SELECT ST_Area('LINESTRING(0 0, 10 10)');
-- Expected: 0

-- ----------------------------------------------------------------------------
-- Test 7: ST_Perimeter - Polygon perimeter
-- ----------------------------------------------------------------------------
.print ""
.print "Test 7: ST_Perimeter - Square (10x10)"
SELECT ST_Perimeter('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 40

.print ""
.print "Test 8: ST_Perimeter - Rectangle (20x5)"
SELECT ST_Perimeter('POLYGON((0 0, 20 0, 20 5, 0 5, 0 0))');
-- Expected: 50

.print ""
.print "Test 9: ST_Perimeter - MULTIPOLYGON (two squares)"
SELECT ST_Perimeter('MULTIPOLYGON(((0 0, 5 0, 5 5, 0 5, 0 0)), ((10 0, 15 0, 15 5, 10 5, 10 0)))');
-- Expected: 40 (20 + 20)

.print ""
.print "Test 10: ST_Perimeter - POINT (should be 0)"
SELECT ST_Perimeter('POINT(10 20)');
-- Expected: 0

.print ""
.print "Test 11: ST_Perimeter - LINESTRING (should be 0)"
SELECT ST_Perimeter('LINESTRING(0 0, 10 10)');
-- Expected: 0

-- ----------------------------------------------------------------------------
-- Test 12: ST_Length - LineString length
-- ----------------------------------------------------------------------------
.print ""
.print "Test 12: ST_Length - Horizontal line"
SELECT ST_Length('LINESTRING(0 0, 10 0)');
-- Expected: 10

.print ""
.print "Test 13: ST_Length - Vertical line"
SELECT ST_Length('LINESTRING(0 0, 0 10)');
-- Expected: 10

.print ""
.print "Test 14: ST_Length - Diagonal line (3-4-5 triangle)"
SELECT ST_Length('LINESTRING(0 0, 3 4)');
-- Expected: 5

.print ""
.print "Test 15: ST_Length - Multi-segment line"
SELECT ST_Length('LINESTRING(0 0, 10 0, 10 10, 0 10)');
-- Expected: 30 (10 + 10 + 10)

.print ""
.print "Test 16: ST_Length - MULTILINESTRING"
SELECT ST_Length('MULTILINESTRING((0 0, 10 0), (0 10, 10 10))');
-- Expected: 20 (10 + 10)

.print ""
.print "Test 17: ST_Length - POINT (should be 0)"
SELECT ST_Length('POINT(10 20)');
-- Expected: 0

.print ""
.print "Test 18: ST_Length - POLYGON (should be 0 or perimeter)"
SELECT ST_Length('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 0 or 40 (depends on implementation)

-- ----------------------------------------------------------------------------
-- Test 19: ST_Distance - Point-to-Point distance
-- ----------------------------------------------------------------------------
.print ""
.print "Test 19: ST_Distance - Horizontal distance"
SELECT ST_Distance('POINT(0 0)', 'POINT(10 0)');
-- Expected: 10

.print ""
.print "Test 20: ST_Distance - Vertical distance"
SELECT ST_Distance('POINT(0 0)', 'POINT(0 10)');
-- Expected: 10

.print ""
.print "Test 21: ST_Distance - Diagonal (3-4-5 triangle)"
SELECT ST_Distance('POINT(0 0)', 'POINT(3 4)');
-- Expected: 5

.print ""
.print "Test 22: ST_Distance - Same point (should be 0)"
SELECT ST_Distance('POINT(5 5)', 'POINT(5 5)');
-- Expected: 0

.print ""
.print "Test 23: ST_Distance - Negative coordinates"
SELECT ST_Distance('POINT(-10 -10)', 'POINT(10 10)');
-- Expected: ~28.28 (20*sqrt(2))

.print ""
.print "Test 24: ST_Distance - Point to LINESTRING"
SELECT ST_Distance('POINT(5 5)', 'LINESTRING(0 0, 10 0)');
-- Expected: 5 (perpendicular distance to line)

.print ""
.print "Test 25: ST_Distance - Point to POLYGON"
SELECT ST_Distance('POINT(15 5)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 5 (distance from (15,5) to nearest edge at x=10)

.print ""
.print "Test 26: ST_Distance - Point inside POLYGON (should be 0)"
SELECT ST_Distance('POINT(5 5)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 0

-- ----------------------------------------------------------------------------
-- Test 27: Combined measurements
-- ----------------------------------------------------------------------------
.print ""
.print "Test 27: Calculate all measurements for a square"
SELECT 
    ST_Area('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS area,
    ST_Perimeter('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS perimeter;
-- Expected: area=100, perimeter=40

.print ""
.print "Test 28: Verify Pythagorean theorem with ST_Distance"
SELECT 
    ST_Distance('POINT(0 0)', 'POINT(30 40)') AS hypotenuse,
    ROUND(50.0, 2) AS expected;
-- Expected: hypotenuse=50, expected=50

-- ----------------------------------------------------------------------------
-- Test 29: NULL handling
-- ----------------------------------------------------------------------------
.print ""
.print "Test 29: ST_Area - NULL input"
SELECT ST_Area(NULL);
-- Expected: (empty/null)

.print ""
.print "Test 30: ST_Length - NULL input"
SELECT ST_Length(NULL);
-- Expected: (empty/null)

.print ""
.print "Test 31: ST_Distance - NULL first argument"
SELECT ST_Distance(NULL, 'POINT(0 0)');
-- Expected: (empty/null)

.print ""
.print "Test 32: ST_Distance - NULL second argument"
SELECT ST_Distance('POINT(0 0)', NULL);
-- Expected: (empty/null)

-- ----------------------------------------------------------------------------
-- Test 33: Real-world example (Tokyo to Osaka distance - planar approximation)
-- ----------------------------------------------------------------------------
.print ""
.print "Test 33: Tokyo to Osaka (decimal degrees - planar approximation only)"
SELECT ST_Distance(
    'SRID=4326;POINT(139.6917 35.6895)',  -- Tokyo
    'SRID=4326;POINT(135.5022 34.6937)'   -- Osaka
);
-- Expected: ~4.96 degrees (NOT accurate for real geodesic distance)
-- Note: Phase 5 will add proper geodesic calculations

.print ""
.print "Test 34: Building footprint area (meters, UTM-like projection)"
SELECT ST_Area('SRID=32654;POLYGON((500000 4000000, 500100 4000000, 500100 4000050, 500000 4000050, 500000 4000000))');
-- Expected: 5000 (100m × 50m)

.print ""
.print "===================================="
.print "Measurement Tests Complete"
.print "===================================="
