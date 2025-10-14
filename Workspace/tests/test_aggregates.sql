-- Test Suite: Aggregate Functions
-- Phase 5 implementation tests

.load ./sqlitegis.dylib

-- =============================================================================
-- ST_Collect テスト
-- =============================================================================

.print '=== ST_Collect Tests ==='

-- Test 1: Collect POINTs into MULTIPOINT
.print 'Test 1: ST_Collect - POINT → MULTIPOINT'
CREATE TEMP TABLE test_points (id INT, geom TEXT);
INSERT INTO test_points VALUES
    (1, ST_AsEWKT(ST_MakePoint(0, 0, 4326))),
    (2, ST_AsEWKT(ST_MakePoint(1, 1, 4326))),
    (3, ST_AsEWKT(ST_MakePoint(2, 2, 4326)));

SELECT 'Result: ' || ST_AsEWKT(ST_Collect(geom)) as collected FROM test_points;
SELECT 'Type: ' || ST_GeometryType(ST_Collect(geom)) as geom_type FROM test_points;

-- Test 2: Collect LINESTRINGs into MULTILINESTRING
.print ''
.print 'Test 2: ST_Collect - LINESTRING → MULTILINESTRING'
CREATE TEMP TABLE test_lines (id INT, geom TEXT);
INSERT INTO test_lines VALUES
    (1, 'SRID=4326;LINESTRING(0 0, 1 1)'),
    (2, 'SRID=4326;LINESTRING(2 2, 3 3)');

SELECT 'Result: ' || ST_AsEWKT(ST_Collect(geom)) as collected FROM test_lines;

-- Test 3: Collect POLYGONs into MULTIPOLYGON
.print ''
.print 'Test 3: ST_Collect - POLYGON → MULTIPOLYGON'
CREATE TEMP TABLE test_polys (id INT, geom TEXT);
INSERT INTO test_polys VALUES
    (1, 'SRID=4326;POLYGON((0 0, 2 0, 2 2, 0 2, 0 0))'),
    (2, 'SRID=4326;POLYGON((3 3, 5 3, 5 5, 3 5, 3 3))');

SELECT 'Result: ' || ST_AsEWKT(ST_Collect(geom)) as collected FROM test_polys;

-- Test 4: Mixed types → GEOMETRYCOLLECTION
.print ''
.print 'Test 4: ST_Collect - Mixed types → GEOMETRYCOLLECTION'
CREATE TEMP TABLE test_mixed (id INT, geom TEXT);
INSERT INTO test_mixed VALUES
    (1, ST_AsEWKT(ST_MakePoint(0, 0, 4326))),
    (2, 'SRID=4326;LINESTRING(1 1, 2 2)');

SELECT 'Result: ' || ST_AsEWKT(ST_Collect(geom)) as collected FROM test_mixed;

-- =============================================================================
-- ST_Union テスト
-- =============================================================================

.print ''
.print '=== ST_Union Tests ==='

-- Test 5: Union overlapping polygons
.print 'Test 5: ST_Union - Overlapping polygons'
DROP TABLE IF EXISTS test_polys;
CREATE TEMP TABLE test_polys (id INT, geom TEXT);
INSERT INTO test_polys VALUES
    (1, 'SRID=4326;POLYGON((0 0, 2 0, 2 2, 0 2, 0 0))'),
    (2, 'SRID=4326;POLYGON((1 1, 3 1, 3 3, 1 3, 1 1))');

SELECT 'Result: ' || ST_AsEWKT(ST_Union(geom)) as union_result FROM test_polys;
SELECT 'Area: ' || ST_Area(ST_Union(geom)) as union_area FROM test_polys;
.print 'Expected area: 7.0 (overlap counted once)'

-- Test 6: Union single polygon (should return same)
.print ''
.print 'Test 6: ST_Union - Single polygon'
DELETE FROM test_polys WHERE id = 2;
SELECT 'Result: ' || ST_AsEWKT(ST_Union(geom)) as union_result FROM test_polys;

-- =============================================================================
-- ST_ConvexHull_Agg テスト
-- =============================================================================

.print ''
.print '=== ST_ConvexHull_Agg Tests ==='

-- Test 7: Convex hull of points
.print 'Test 7: ST_ConvexHull_Agg - Point cloud'
DROP TABLE IF EXISTS test_points;
CREATE TEMP TABLE test_points (id INT, geom TEXT);
INSERT INTO test_points VALUES
    (1, ST_AsEWKT(ST_MakePoint(0, 0, 4326))),
    (2, ST_AsEWKT(ST_MakePoint(4, 0, 4326))),
    (3, ST_AsEWKT(ST_MakePoint(4, 3, 4326))),
    (4, ST_AsEWKT(ST_MakePoint(0, 3, 4326))),
    (5, ST_AsEWKT(ST_MakePoint(2, 1.5, 4326)));  -- Internal point

SELECT 'Result: ' || ST_AsEWKT(ST_ConvexHull_Agg(geom)) as convex_hull FROM test_points;
SELECT 'Type: ' || ST_GeometryType(ST_ConvexHull_Agg(geom)) as hull_type FROM test_points;
SELECT 'Area: ' || ST_Area(ST_ConvexHull_Agg(geom)) as hull_area FROM test_points;
.print 'Expected area: 12.0'

-- Test 8: Convex hull of linestrings
.print ''
.print 'Test 8: ST_ConvexHull_Agg - Linestrings'
DROP TABLE IF EXISTS test_lines;
CREATE TEMP TABLE test_lines (id INT, geom TEXT);
INSERT INTO test_lines VALUES
    (1, 'SRID=4326;LINESTRING(0 0, 2 0)'),
    (2, 'SRID=4326;LINESTRING(2 0, 2 2)'),
    (3, 'SRID=4326;LINESTRING(2 2, 0 2)');

SELECT 'Result: ' || ST_AsEWKT(ST_ConvexHull_Agg(geom)) as convex_hull FROM test_lines;

-- =============================================================================
-- ST_Extent_Agg テスト
-- =============================================================================

.print ''
.print '=== ST_Extent_Agg Tests ==='

-- Test 9: Extent of points
.print 'Test 9: ST_Extent_Agg - Points'
DROP TABLE IF EXISTS test_points;
CREATE TEMP TABLE test_points (id INT, geom TEXT);
INSERT INTO test_points VALUES
    (1, ST_AsEWKT(ST_MakePoint(0, 0, 4326))),
    (2, ST_AsEWKT(ST_MakePoint(10, 5, 4326))),
    (3, ST_AsEWKT(ST_MakePoint(-5, 8, 4326)));

SELECT 'Extent: ' || ST_Extent_Agg(geom) as extent FROM test_points;
.print 'Expected: BOX(-5 0, 10 8)'

-- Test 10: Extent of polygons
.print ''
.print 'Test 10: ST_Extent_Agg - Polygons'
DROP TABLE IF EXISTS test_polys;
CREATE TEMP TABLE test_polys (id INT, geom TEXT);
INSERT INTO test_polys VALUES
    (1, 'SRID=4326;POLYGON((0 0, 2 0, 2 2, 0 2, 0 0))'),
    (2, 'SRID=4326;POLYGON((5 5, 10 5, 10 10, 5 10, 5 5))');

SELECT 'Extent: ' || ST_Extent_Agg(geom) as extent FROM test_polys;
.print 'Expected: BOX(0 0, 10 10)'

-- =============================================================================
-- NULL handling tests
-- =============================================================================

.print ''
.print '=== NULL Handling Tests ==='

-- Test 11: ST_Collect with NULLs
.print 'Test 11: ST_Collect with NULL values'
CREATE TEMP TABLE test_nulls (id INT, geom TEXT);
INSERT INTO test_nulls VALUES
    (1, ST_AsEWKT(ST_MakePoint(0, 0, 4326))),
    (2, NULL),
    (3, ST_AsEWKT(ST_MakePoint(1, 1, 4326)));

SELECT 'Result: ' || ST_AsEWKT(ST_Collect(geom)) as collected FROM test_nulls;
.print 'Expected: Two points (NULL ignored)'

-- Test 12: All NULLs
.print ''
.print 'Test 12: ST_Collect with all NULLs'
DELETE FROM test_nulls WHERE geom IS NOT NULL;
SELECT 'Result: ' || COALESCE(ST_AsEWKT(ST_Collect(geom)), 'NULL') as collected FROM test_nulls;

-- =============================================================================
-- Performance test (optional, small scale)
-- =============================================================================

.print ''
.print '=== Performance Test ==='

-- Test 13: 1000 points
.print 'Test 13: ST_Collect 1000 points'
DROP TABLE IF EXISTS perf_test;
CREATE TEMP TABLE perf_test (id INT, geom TEXT);

-- Insert 100 points (more than 1000 may be slow in SQLite without prepared statements)
WITH RECURSIVE cnt(x) AS (
    SELECT 0
    UNION ALL
    SELECT x+1 FROM cnt WHERE x < 99
)
INSERT INTO perf_test
SELECT x, ST_AsEWKT(ST_MakePoint(x * 0.1, x * 0.2, 4326)) FROM cnt;

.print 'Collecting 100 points...'
.timer on
SELECT 'Points collected: ' || CASE 
    WHEN ST_GeometryType(ST_Collect(geom)) = 'ST_MultiPoint' THEN 'Success'
    ELSE 'Failed'
END as result FROM perf_test;
.timer off

.print 'Computing extent...'
.timer on
SELECT 'Extent: ' || ST_Extent_Agg(geom) as extent FROM perf_test;
.timer off

.print 'Computing convex hull...'
.timer on
SELECT 'Hull computed: ' || CASE 
    WHEN ST_GeometryType(ST_ConvexHull_Agg(geom)) = 'ST_Polygon' THEN 'Success'
    ELSE 'Failed'
END as result FROM perf_test;
.timer off

-- Cleanup
DROP TABLE IF EXISTS test_points;
DROP TABLE IF EXISTS test_lines;
DROP TABLE IF EXISTS test_polys;
DROP TABLE IF EXISTS test_mixed;
DROP TABLE IF EXISTS test_nulls;
DROP TABLE IF EXISTS perf_test;

.print ''
.print '=== All Aggregate Function Tests Complete ==='
