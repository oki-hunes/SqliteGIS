-- ====================================================================
-- WKB/EWKB Format Tests for SqliteGIS Extension
-- ====================================================================
-- This test suite validates binary format support (WKB/EWKB).
--
-- Test Categories:
-- 1. ST_GeomFromWKB - Parse WKB binary format
-- 2. ST_GeomFromEWKB - Parse EWKB binary format with SRID
-- 3. ST_AsBinary - Convert geometry to WKB
-- 4. ST_AsEWKB - Convert geometry to EWKB
-- 5. Roundtrip Tests - Verify format preservation
-- ====================================================================

.load ./sqlitegis.dylib

-- Enable headers for better output readability
.headers on
.mode column

SELECT '====================================================================';
SELECT 'WKB/EWKB Format Tests';
SELECT '====================================================================';

-- --------------------------------------------------------------------
-- Test Category 1: ST_GeomFromWKB
-- Parse WKB binary format and convert to text
-- --------------------------------------------------------------------

SELECT '';
SELECT '--- Test 1.1: ST_GeomFromWKB with Point (Little Endian) ---';
-- WKB: 01 (little endian) 01000000 (Point type) + coordinates
-- Point(1.0, 2.0)
SELECT ST_AsText(ST_GeomFromWKB(
    X'0101000000000000000000F03F0000000000000040'
)) AS wkt;
-- Expected: POINT(1 2)

SELECT '';
SELECT '--- Test 1.2: ST_GeomFromWKB with SRID parameter ---';
SELECT ST_AsEWKT(ST_GeomFromWKB(
    X'0101000000000000000000F03F0000000000000040',
    4326
)) AS ewkt;
-- Expected: SRID=4326;POINT(1 2)

SELECT '';
SELECT '--- Test 1.3: ST_GeomFromWKB with NULL input ---';
SELECT ST_GeomFromWKB(NULL) IS NULL AS is_null;
-- Expected: 1

SELECT '';
SELECT '--- Test 1.4: ST_GeomFromWKB with invalid binary data ---';
SELECT ST_GeomFromWKB(X'FF') IS NULL AS is_null;
-- Expected: NULL or error (handled gracefully)

-- --------------------------------------------------------------------
-- Test Category 2: ST_GeomFromEWKB
-- Parse EWKB binary format with embedded SRID
-- --------------------------------------------------------------------

SELECT '';
SELECT '--- Test 2.1: ST_GeomFromEWKB with Point ---';
-- EWKB: 01 (little endian) 01000020 (Point type with SRID flag)
-- 0x20000001 = Point with SRID flag
-- Then SRID (4326) then coordinates
SELECT ST_AsEWKT(ST_GeomFromEWKB(
    X'0101000020E6100000000000000000F03F0000000000000040'
)) AS ewkt;
-- Expected: SRID=4326;POINT(1 2)

SELECT '';
SELECT '--- Test 2.2: ST_GeomFromEWKB extracts SRID correctly ---';
SELECT ST_SRID(ST_GeomFromEWKB(
    X'0101000020E6100000000000000000F03F0000000000000040'
)) AS srid;
-- Expected: 4326

SELECT '';
SELECT '--- Test 2.3: ST_GeomFromEWKB with NULL input ---';
SELECT ST_GeomFromEWKB(NULL) IS NULL AS is_null;
-- Expected: 1

-- --------------------------------------------------------------------
-- Test Category 3: ST_AsBinary
-- Convert geometry to WKB format
-- --------------------------------------------------------------------

SELECT '';
SELECT '--- Test 3.1: ST_AsBinary converts Point to WKB ---';
SELECT hex(ST_AsBinary(ST_MakePoint(1.0, 2.0))) AS wkb_hex;
-- Expected: Hex string starting with 01 (little endian) 01000000 (Point type)

SELECT '';
SELECT '--- Test 3.2: ST_AsBinary with EWKT input ---';
SELECT hex(ST_AsBinary(ST_GeomFromEWKT('SRID=4326;POINT(139.69 35.68)'))) AS wkb_hex;
-- Expected: WKB without SRID (standard WKB format)

SELECT '';
SELECT '--- Test 3.3: ST_AsBinary with NULL input ---';
SELECT ST_AsBinary(NULL) IS NULL AS is_null;
-- Expected: 1

-- --------------------------------------------------------------------
-- Test Category 4: ST_AsEWKB
-- Convert geometry to EWKB format with SRID
-- --------------------------------------------------------------------

SELECT '';
SELECT '--- Test 4.1: ST_AsEWKB converts Point to EWKB ---';
SELECT hex(ST_AsEWKB(ST_SetSRID(ST_MakePoint(1.0, 2.0), 4326))) AS ewkb_hex;
-- Expected: Hex string with SRID flag (0x20000001) and SRID value (E6100000 = 4326 LE)

SELECT '';
SELECT '--- Test 4.2: ST_AsEWKB includes SRID ---';
-- First 9 bytes: 01 (byte order) + 01000020 (type with SRID flag) + E6100000 (SRID 4326)
SELECT substr(hex(ST_AsEWKB(ST_SetSRID(ST_MakePoint(1.0, 2.0), 4326))), 1, 18) AS header_hex;
-- Expected: 0101000020E6100000

SELECT '';
SELECT '--- Test 4.3: ST_AsEWKB with NULL input ---';
SELECT ST_AsEWKB(NULL) IS NULL AS is_null;
-- Expected: 1

-- --------------------------------------------------------------------
-- Test Category 5: Roundtrip Tests
-- Verify data preservation through WKB/EWKB conversion
-- --------------------------------------------------------------------

SELECT '';
SELECT '--- Test 5.1: WKT -> WKB -> WKT roundtrip ---';
SELECT ST_AsText(
    ST_GeomFromWKB(
        ST_AsBinary(
            ST_GeomFromText('POINT(139.69 35.68)')
        )
    )
) AS roundtrip_wkt;
-- Expected: POINT(139.69 35.68)

SELECT '';
SELECT '--- Test 5.2: EWKT -> EWKB -> EWKT roundtrip ---';
SELECT ST_AsEWKT(
    ST_GeomFromEWKB(
        ST_AsEWKB(
            ST_GeomFromEWKT('SRID=4326;POINT(139.69 35.68)')
        )
    )
) AS roundtrip_ewkt;
-- Expected: SRID=4326;POINT(139.69 35.68)

SELECT '';
SELECT '--- Test 5.3: SRID preservation in EWKB roundtrip ---';
SELECT ST_SRID(
    ST_GeomFromEWKB(
        ST_AsEWKB(
            ST_SetSRID(ST_MakePoint(100, 50), 3857)
        )
    )
) AS preserved_srid;
-- Expected: 3857

SELECT '';
SELECT '--- Test 5.4: Coordinate precision in WKB roundtrip ---';
-- Test with high-precision coordinates
SELECT ST_AsText(
    ST_GeomFromWKB(
        ST_AsBinary(
            ST_GeomFromText('POINT(139.691706 35.689506)')
        )
    )
) AS precise_roundtrip;
-- Expected: POINT(139.691706 35.689506) (or very close due to double precision)

-- --------------------------------------------------------------------
-- Summary
-- --------------------------------------------------------------------

SELECT '';
SELECT '====================================================================';
SELECT 'WKB/EWKB Format Tests Complete';
SELECT '====================================================================';
SELECT 'Note: Some tests may return partial results if full WKB serialization';
SELECT 'is not yet implemented. Parser tests should work correctly.';
SELECT '====================================================================';
