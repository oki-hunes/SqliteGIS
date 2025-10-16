-- SqliteGIS基本テスト (Windows)

.print "===================================="
.print "SqliteGIS Basic Tests - Windows"
.print "===================================="

-- 拡張機能をロード
.load ./build/Release/sqlitegis.dll

-- テスト1: 基本的なポイント作成
.print ""
.print "Test 1: ST_MakePoint (2D)"
SELECT ST_AsEWKT(ST_MakePoint(139.7, 35.7));

-- テスト2: WKTからジオメトリ作成
.print ""
.print "Test 2: ST_GeomFromText with SRID"
SELECT ST_AsEWKT(ST_GeomFromText('POINT(1 2)', 4326));

-- テスト3: 3Dポイント
.print ""
.print "Test 3: ST_MakePointZ (3D)"
SELECT ST_AsEWKT(ST_MakePointZ(1.0, 2.0, 3.0, 4326));

-- テスト4: 距離計算（WKT文字列を使用）
.print ""
.print "Test 4: ST_Distance (2D points)"
SELECT ST_Distance('POINT(0 0)', 'POINT(3 4)');

-- テスト5: LineString
.print ""
.print "Test 5: LineString with SRID"
SELECT ST_AsEWKT(ST_GeomFromText('LINESTRING(0 0, 1 1, 2 2)', 4326));

-- テスト6: バウンディングボックス
.print ""
.print "Test 6: ST_Extent"
SELECT ST_Extent('LINESTRING(0 0, 10 10)');

-- テスト7: 3Dジオメトリ判定
.print ""
.print "Test 7: ST_Is3D (3D vs 2D)"
SELECT ST_Is3D(ST_MakePointZ(1, 2, 3, -1)) as '3D_point',
       ST_Is3D(ST_MakePoint(1, 2)) as '2D_point';

-- テスト8: SRID取得
.print ""
.print "Test 8: ST_SRID"
SELECT ST_SRID(ST_GeomFromText('POINT(1 2)', 4326));

-- テスト9: 座標取得
.print ""
.print "Test 9: ST_X, ST_Y coordinates"
SELECT ST_X(ST_MakePoint(10.5, 20.7)) as X,
       ST_Y(ST_MakePoint(10.5, 20.7)) as Y;

-- テスト10: ジオメトリタイプ
.print ""
.print "Test 10: ST_GeometryType"
SELECT ST_GeometryType(ST_GeomFromText('LINESTRING(0 0, 1 1)', -1));

.print ""
.print "===================================="
.print "All tests completed successfully!"
.print "===================================="
