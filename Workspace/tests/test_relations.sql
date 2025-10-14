-- ============================================================================
-- SqliteGIS Test Suite: Spatial Relationship Functions
-- ============================================================================

.load build/sqlitegis.dylib

.print "===================================="
.print "Test Suite: Spatial Relationships"
.print "===================================="
.print ""

-- ----------------------------------------------------------------------------
-- Test 1: ST_Intersects - Basic intersection tests
-- ----------------------------------------------------------------------------
.print "Test 1: ST_Intersects - Overlapping POINTs (same location)"
SELECT ST_Intersects('POINT(5 5)', 'POINT(5 5)');
-- Expected: 1 (true)

.print ""
.print "Test 2: ST_Intersects - Non-overlapping POINTs"
SELECT ST_Intersects('POINT(0 0)', 'POINT(10 10)');
-- Expected: 0 (false)

.print ""
.print "Test 3: ST_Intersects - POINT on LINESTRING"
SELECT ST_Intersects('POINT(5 5)', 'LINESTRING(0 0, 10 10)');
-- Expected: 1 (true)

.print ""
.print "Test 4: ST_Intersects - POINT not on LINESTRING"
SELECT ST_Intersects('POINT(0 10)', 'LINESTRING(0 0, 10 0)');
-- Expected: 0 (false)

.print ""
.print "Test 5: ST_Intersects - Crossing LINESTRINGs"
SELECT ST_Intersects('LINESTRING(0 0, 10 10)', 'LINESTRING(0 10, 10 0)');
-- Expected: 1 (true)

.print ""
.print "Test 6: ST_Intersects - Parallel LINESTRINGs"
SELECT ST_Intersects('LINESTRING(0 0, 10 0)', 'LINESTRING(0 5, 10 5)');
-- Expected: 0 (false)

.print ""
.print "Test 7: ST_Intersects - POINT inside POLYGON"
SELECT ST_Intersects('POINT(5 5)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 1 (true)

.print ""
.print "Test 8: ST_Intersects - POINT outside POLYGON"
SELECT ST_Intersects('POINT(15 15)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 0 (false)

.print ""
.print "Test 9: ST_Intersects - POLYGON overlapping POLYGON"
SELECT ST_Intersects(
    'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
    'POLYGON((5 5, 15 5, 15 15, 5 15, 5 5))'
);
-- Expected: 1 (true)

.print ""
.print "Test 10: ST_Intersects - Separate POLYGONs"
SELECT ST_Intersects(
    'POLYGON((0 0, 5 0, 5 5, 0 5, 0 0))',
    'POLYGON((10 10, 15 10, 15 15, 10 15, 10 10))'
);
-- Expected: 0 (false)

-- ----------------------------------------------------------------------------
-- Test 11: ST_Contains - Containment tests
-- ----------------------------------------------------------------------------
.print ""
.print "Test 11: ST_Contains - POLYGON contains POINT"
SELECT ST_Contains('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 'POINT(5 5)');
-- Expected: 1 (true)

.print ""
.print "Test 12: ST_Contains - POLYGON does not contain POINT (outside)"
SELECT ST_Contains('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 'POINT(15 15)');
-- Expected: 0 (false)

.print ""
.print "Test 13: ST_Contains - Large POLYGON contains small POLYGON"
SELECT ST_Contains(
    'POLYGON((0 0, 20 0, 20 20, 0 20, 0 0))',
    'POLYGON((5 5, 15 5, 15 15, 5 15, 5 5))'
);
-- Expected: 1 (true)

.print ""
.print "Test 14: ST_Contains - POLYGON does not contain overlapping POLYGON"
SELECT ST_Contains(
    'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
    'POLYGON((5 5, 15 5, 15 15, 5 15, 5 5))'
);
-- Expected: 0 (false - overlaps but not contained)

.print ""
.print "Test 15: ST_Contains - POLYGON contains LINESTRING"
SELECT ST_Contains(
    'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
    'LINESTRING(2 2, 8 8)'
);
-- Expected: 1 (true)

.print ""
.print "Test 16: ST_Contains - POLYGON does not contain LINESTRING (crosses boundary)"
SELECT ST_Contains(
    'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
    'LINESTRING(0 0, 20 20)'
);
-- Expected: 0 (false)

.print ""
.print "Test 17: ST_Contains - MULTIPOLYGON contains POINT"
SELECT ST_Contains(
    'MULTIPOLYGON(((0 0, 10 0, 10 10, 0 10, 0 0)), ((20 20, 30 20, 30 30, 20 30, 20 20)))',
    'POINT(5 5)'
);
-- Expected: 1 (true - in first polygon)

.print ""
.print "Test 18: ST_Contains - Boundary condition (POINT on edge)"
SELECT ST_Contains('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 'POINT(10 5)');
-- Expected: 1 or 0 (depends on boundary handling)

-- ----------------------------------------------------------------------------
-- Test 19: ST_Within - Inverse of Contains
-- ----------------------------------------------------------------------------
.print ""
.print "Test 19: ST_Within - POINT within POLYGON"
SELECT ST_Within('POINT(5 5)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 1 (true)

.print ""
.print "Test 20: ST_Within - POINT not within POLYGON"
SELECT ST_Within('POINT(15 15)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- Expected: 0 (false)

.print ""
.print "Test 21: ST_Within - Small POLYGON within large POLYGON"
SELECT ST_Within(
    'POLYGON((5 5, 15 5, 15 15, 5 15, 5 5))',
    'POLYGON((0 0, 20 0, 20 20, 0 20, 0 0))'
);
-- Expected: 1 (true)

.print ""
.print "Test 22: ST_Within - POLYGON not within POLYGON (overlapping)"
SELECT ST_Within(
    'POLYGON((5 5, 15 5, 15 15, 5 15, 5 5))',
    'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'
);
-- Expected: 0 (false)

.print ""
.print "Test 23: ST_Within - LINESTRING within POLYGON"
SELECT ST_Within(
    'LINESTRING(2 2, 8 8)',
    'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'
);
-- Expected: 1 (true)

.print ""
.print "Test 24: ST_Within - Symmetry check (A within B != B within A)"
SELECT 
    ST_Within('POINT(5 5)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS point_in_poly,
    ST_Within('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 'POINT(5 5)') AS poly_in_point;
-- Expected: point_in_poly=1, poly_in_point=0

-- ----------------------------------------------------------------------------
-- Test 25: Relationship between ST_Contains and ST_Within
-- ----------------------------------------------------------------------------
.print ""
.print "Test 25: ST_Contains(A,B) should equal ST_Within(B,A)"
SELECT 
    ST_Contains('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 'POINT(5 5)') AS contains,
    ST_Within('POINT(5 5)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS within;
-- Expected: contains=1, within=1 (both true)

.print ""
.print "Test 26: Verify inverse relationship"
SELECT 
    ST_Contains('POINT(5 5)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS contains,
    ST_Within('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 'POINT(5 5)') AS within;
-- Expected: contains=0, within=0 (both false)

-- ----------------------------------------------------------------------------
-- Test 27: NULL handling
-- ----------------------------------------------------------------------------
.print ""
.print "Test 27: ST_Intersects - NULL first argument"
SELECT ST_Intersects(NULL, 'POINT(0 0)');
-- Expected: (empty/null)

.print ""
.print "Test 28: ST_Contains - NULL second argument"
SELECT ST_Contains('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', NULL);
-- Expected: (empty/null)

.print ""
.print "Test 29: ST_Within - Both NULL"
SELECT ST_Within(NULL, NULL);
-- Expected: (empty/null)

-- ----------------------------------------------------------------------------
-- Test 30: Unsupported combinations (should return false, not error)
-- ----------------------------------------------------------------------------
.print ""
.print "Test 30: ST_Contains - Unsupported: POINT contains LINESTRING"
SELECT ST_Contains('POINT(5 5)', 'LINESTRING(0 0, 10 10)');
-- Expected: 0 (false - unsupported combination)

.print ""
.print "Test 31: ST_Within - Unsupported: POINT within POINT"
SELECT ST_Within('POINT(5 5)', 'POINT(5 5)');
-- Expected: 0 (false - unsupported combination)

-- ----------------------------------------------------------------------------
-- Test 32: Complex scenarios
-- ----------------------------------------------------------------------------
.print ""
.print "Test 32: Multiple POINTs intersecting a POLYGON"
SELECT 
    ST_Intersects('POINT(5 5)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS p1,
    ST_Intersects('POINT(0 0)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS p2,
    ST_Intersects('POINT(15 15)', 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS p3;
-- Expected: p1=1 (inside), p2=1 (on boundary), p3=0 (outside)

.print ""
.print "Test 33: SRID should not affect spatial relationships (planar)"
SELECT 
    ST_Intersects('SRID=4326;POINT(5 5)', 'SRID=4326;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS same_srid,
    ST_Intersects('SRID=3857;POINT(5 5)', 'SRID=4326;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS diff_srid;
-- Expected: same_srid=1, diff_srid=1 (SRID ignored in Phase 1)

.print ""
.print "===================================="
.print "Spatial Relationship Tests Complete"
.print "===================================="
