# SqliteGIS 仕様書

## 概要
本仕様書は、SQLite用のPostGIS互換GIS拡張機能の実装仕様を定義します。
Boost.Geometryをコアライブラリとして使用し、段階的に機能を実装していきます。

---

## 1. データ型

### 1.1 Geometry型の内部表現
SQLiteには独自のGeometry型が存在しないため、以下の形式で格納します：

| 形式 | 説明 | 使用場面 | SRID対応 |
|------|------|----------|----------|
| **WKT (Well-Known Text)** | `TEXT` 型で格納 | 人間可読、OGC標準準拠 | ✗ |
| **EWKT (Extended WKT)** | `TEXT` 型で格納、SRID接頭辞付き | PostGIS互換、人間可読 | ✓ |
| **WKB (Well-Known Binary)** | `BLOB` 型で格納 | コンパクト、OGC標準準拠 | ✗ |
| **EWKB (Extended WKB)** | `BLOB` 型で格納、SRIDヘッダ付き | PostGIS互換、高速 | ✓ |
| **GeoJSON** | `TEXT` 型で格納 | Web連携、可視化 | ✓ (crs属性) |

**初期実装ではWKT/EWKTを優先**し、後続でWKB/EWKB、GeoJSONを追加します。

### 1.2 EWKT/EWKB フォーマット仕様

#### EWKT (Extended Well-Known Text)
PostGIS互換の拡張WKT形式。SRID情報を `SRID=xxxx;` 接頭辞で表現します。

**構文**:
```
SRID=<srid>;<wkt_geometry>
```

**例**:
```sql
-- WGS84座標系（SRID=4326）の点
'SRID=4326;POINT(139.6917 35.6895)'

-- 平面座標系（SRID=0、デフォルト）
'SRID=0;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'

-- SRID省略時はSRID=0と解釈
'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'
```

#### EWKB (Extended Well-Known Binary)
PostGIS互換の拡張WKB形式。バイナリヘッダにSRID情報を埋め込みます。

**バイナリ構造** (Little Endian例):
```
[Byte Order (1 byte)] [wkbType + SRID flag (4 bytes)] [SRID (4 bytes)] [Geometry Data...]
```

- **Byte Order**: `01` (Little Endian) or `00` (Big Endian)
- **wkbType**: OGCのwkbType値に `0x20000000` (SRID_FLAG) をOR
  - 例: `wkbPoint (1)` + SRID flag = `0x20000001`
- **SRID**: 4バイト符号付き整数
- **Geometry Data**: 通常のWKBと同じ座標データ

**例** (SRID=4326のPOINT(10 20)):
```
01                    // Little Endian
01 00 00 20           // wkbPoint (1) | SRID_FLAG (0x20000000)
E6 10 00 00           // SRID = 4326 (0x000010E6)
00 00 00 00 00 00 24 40  // X = 10.0 (double)
00 00 00 00 00 00 34 40  // Y = 20.0 (double)
```

### 1.2 サポートするGeometry型

PostGISに準拠した以下の型をサポートします：

| Geometry型 | 次元 | 説明 | WKT例 | EWKT例 (SRID=4326) |
|-----------|------|------|-------|-------------------|
| `Point` | 2D/3D | 点 | `POINT(30 10)` | `SRID=4326;POINT(139.69 35.68)` |
| `LineString` | 2D/3D | 線 | `LINESTRING(30 10, 10 30, 40 40)` | `SRID=4326;LINESTRING(...)` |
| `Polygon` | 2D/3D | 多角形 | `POLYGON((30 10, 40 40, 20 40, 10 20, 30 10))` | `SRID=4326;POLYGON(...)` |
| `MultiPoint` | 2D/3D | 複数点 | `MULTIPOINT((10 40), (40 30))` | `SRID=4326;MULTIPOINT(...)` |
| `MultiLineString` | 2D/3D | 複数線 | `MULTILINESTRING((10 10, 20 20), (15 15, 30 15))` | `SRID=4326;MULTILINESTRING(...)` |
| `MultiPolygon` | 2D/3D | 複数多角形 | `MULTIPOLYGON(((30 20, 45 40, 10 40, 30 20)))` | `SRID=4326;MULTIPOLYGON(...)` |
| `GeometryCollection` | 2D/3D | 異種混合 | `GEOMETRYCOLLECTION(POINT(4 6),LINESTRING(4 6,7 10))` | `SRID=4326;GEOMETRYCOLLECTION(...)` |

**実装優先順位**:
1. Phase 1: Point, LineString, Polygon (2D、WKT/EWKT)
2. Phase 2: Multi* 系 (2D、WKB/EWKB対応)
3. Phase 3: 3D対応 (Z座標)
4. Phase 4: GeometryCollection

---

## 2. 空間参照系 (SRS)

### 2.1 座標系の扱い

| 項目 | 初期実装 | 将来対応 |
|------|----------|----------|
| デフォルト座標系 | 平面座標 (SRID=0) | WGS84 (SRID=4326) |
| 座標変換 | 未対応 | PROJ.4連携 |
| 測地系計算 | 未対応 | GeographicLib連携 |

**Phase 1では平面座標のみ**を扱い、SRIDは常に0とします。

---

## 3. 関数仕様

### 3.1 コンストラクタ関数

Geometryオブジェクトを生成する関数群。

| 関数名 | 引数 | 戻り値 | 説明 | 優先度 |
|--------|------|--------|------|--------|
| `ST_GeomFromText(wkt TEXT)` | WKT文字列 | Geometry (TEXT) | WKTからGeometry生成（SRID=0） | ★★★ |
| `ST_GeomFromText(wkt TEXT, srid INT)` | WKT, SRID | Geometry (TEXT) | SRID付きGeometry生成 | ★★★ |
| `ST_GeomFromEWKT(ewkt TEXT)` | EWKT文字列 | Geometry (TEXT) | EWKTからGeometry生成 | ★★★ |
| `ST_GeomFromWKB(wkb BLOB)` | WKBバイナリ | Geometry (TEXT) | WKBからGeometry生成 | ★★☆ |
| `ST_GeomFromEWKB(ewkb BLOB)` | EWKBバイナリ | Geometry (TEXT) | EWKBからGeometry生成 | ★★☆ |
| `ST_GeomFromGeoJSON(json TEXT)` | GeoJSON文字列 | Geometry (TEXT) | GeoJSONから生成 | ★☆☆ |
| `ST_MakePoint(x REAL, y REAL)` | X座標, Y座標 | Point (TEXT) | 2D点を生成（SRID=0） | ★★★ |
| `ST_MakePoint(x REAL, y REAL, z REAL)` | X, Y, Z座標 | Point (TEXT) | 3D点を生成 | ★☆☆ |
| `ST_SetSRID(geom GEOMETRY, srid INT)` | Geometry, SRID | Geometry (TEXT) | SRIDを設定（座標変換なし） | ★★★ |
| `ST_MakeLine(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのPoint | LineString (TEXT) | 2点から線を生成 | ★★☆ |

### 3.2 アクセサ関数

Geometryの属性を取得する関数群。

| 関数名 | 引数 | 戻り値 | 説明 | 優先度 |
|--------|------|--------|------|--------|
| `ST_AsText(geom GEOMETRY)` | Geometry | TEXT | WKT文字列を取得（SRID情報なし） | ★★★ |
| `ST_AsEWKT(geom GEOMETRY)` | Geometry | TEXT | EWKT文字列を取得（SRID付き） | ★★★ |
| `ST_AsBinary(geom GEOMETRY)` | Geometry | BLOB | WKBバイナリを取得 | ★★☆ |
| `ST_AsEWKB(geom GEOMETRY)` | Geometry | BLOB | EWKBバイナリを取得（SRID付き） | ★★☆ |
| `ST_AsGeoJSON(geom GEOMETRY)` | Geometry | TEXT | GeoJSON文字列を取得 | ★☆☆ |
| `ST_GeometryType(geom GEOMETRY)` | Geometry | TEXT | 型名を取得 (例: "ST_Polygon") | ★★★ |
| `ST_SRID(geom GEOMETRY)` | Geometry | INTEGER | SRID取得 | ★★★ |
| `ST_Dimension(geom GEOMETRY)` | Geometry | INTEGER | 次元数 (2 or 3) | ★★☆ |
| `ST_X(geom GEOMETRY)` | Point | REAL | X座標取得 | ★★★ |
| `ST_Y(geom GEOMETRY)` | Point | REAL | Y座標取得 | ★★★ |
| `ST_Z(geom GEOMETRY)` | Point | REAL | Z座標取得 | ★☆☆ |

### 3.3 計測関数

Geometryの計量を計算する関数群。

| 関数名 | 引数 | 戻り値 | 説明 | 優先度 |
|--------|------|--------|------|--------|
| `ST_Area(geom GEOMETRY)` | Polygon系 | REAL | 面積 (平面) | ★★★ |
| `ST_Perimeter(geom GEOMETRY)` | Polygon系 | REAL | 外周長 (平面) | ★★★ |
| `ST_Length(geom GEOMETRY)` | LineString系 | REAL | 線長 (平面) | ★★★ |
| `ST_Distance(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | REAL | 最短距離 (平面) | ★★★ |
| `ST_3DDistance(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | REAL | 3D最短距離 | ★☆☆ |

### 3.4 空間関係関数 (Spatial Relationships)

2つのGeometry間の位置関係を判定する関数群。

| 関数名 | 引数 | 戻り値 | 説明 | 優先度 |
|--------|------|--------|------|--------|
| `ST_Intersects(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | BOOLEAN (INTEGER) | 交差判定 | ★★★ |
| `ST_Contains(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | BOOLEAN | geom1がgeom2を含む | ★★★ |
| `ST_Within(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | BOOLEAN | geom1がgeom2内にある | ★★★ |
| `ST_Crosses(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | BOOLEAN | 交差する（接触を除く） | ★★☆ |
| `ST_Overlaps(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | BOOLEAN | 重なり判定 | ★★☆ |
| `ST_Touches(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | BOOLEAN | 境界接触判定 | ★★☆ |
| `ST_Disjoint(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | BOOLEAN | 無交差判定 | ★☆☆ |
| `ST_Equals(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | BOOLEAN | 空間的同一判定 | ★★☆ |

### 3.5 空間演算関数 (Spatial Operations)

新しいGeometryを生成する演算関数群。

| 関数名 | 引数 | 戻り値 | 説明 | 優先度 |
|--------|------|--------|------|--------|
| `ST_Buffer(geom GEOMETRY, distance REAL)` | Geometry, バッファ距離 | Geometry | バッファ領域生成 | ★★★ |
| `ST_Intersection(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | Geometry | 交差部分抽出 | ★★★ |
| `ST_Union(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | Geometry | 和集合 | ★★★ |
| `ST_Difference(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | Geometry | 差分 (geom1 - geom2) | ★★☆ |
| `ST_SymDifference(geom1 GEOMETRY, geom2 GEOMETRY)` | 2つのGeometry | Geometry | 対称差 | ★☆☆ |
| `ST_Centroid(geom GEOMETRY)` | Geometry | Point | 重心計算 | ★★★ |
| `ST_ConvexHull(geom GEOMETRY)` | Geometry | Polygon | 凸包 | ★★☆ |
| `ST_Simplify(geom GEOMETRY, tolerance REAL)` | Geometry, 許容誤差 | Geometry | 形状簡略化 | ★★☆ |

### 3.6 集約関数 (Aggregate Functions)

複数行のGeometryをまとめる関数群。

| 関数名 | 引数 | 戻り値 | 説明 | 優先度 |
|--------|------|--------|------|--------|
| `ST_Union(geom GEOMETRY)` | Geometry集合 | Geometry | 全体の和集合 | ★★☆ |
| `ST_Collect(geom GEOMETRY)` | Geometry集合 | GeometryCollection | 集約 | ★☆☆ |
| `ST_Extent(geom GEOMETRY)` | Geometry集合 | BOX2D (TEXT) | 外接矩形 | ★★☆ |

### 3.7 ユーティリティ関数

| 関数名 | 引数 | 戻り値 | 説明 | 優先度 |
|--------|------|--------|------|--------|
| `ST_IsValid(geom GEOMETRY)` | Geometry | BOOLEAN | 幾何妥当性検証 | ★★★ |
| `ST_IsEmpty(geom GEOMETRY)` | Geometry | BOOLEAN | 空判定 | ★★☆ |
| `ST_Transform(geom GEOMETRY, srid INT)` | Geometry, 目標SRID | Geometry | 座標変換 | ★☆☆ |

---

## 4. 実装フェーズ

### Phase 1: 基礎実装 (v0.1)
- **目標**: 2D平面上の基本Geometry型と基礎関数、EWKT対応
- **型**: Point, LineString, Polygon (WKT/EWKT形式)
- **関数**:
  - コンストラクタ: `ST_GeomFromText`, `ST_GeomFromEWKT`, `ST_MakePoint`, `ST_SetSRID`
  - アクセサ: `ST_AsText`, `ST_AsEWKT`, `ST_GeometryType`, `ST_SRID`, `ST_X`, `ST_Y`
  - 計測: `ST_Area`, `ST_Perimeter`, `ST_Length`, `ST_Distance`
  - 空間関係: `ST_Intersects`, `ST_Contains`, `ST_Within`
  - 空間演算: `ST_Buffer`, `ST_Centroid`
  - ユーティリティ: `ST_IsValid`, `ST_IsEmpty`

### Phase 2: Multi型とWKB/EWKB対応 (v0.2)
- **型**: MultiPoint, MultiLineString, MultiPolygon
- **形式**: WKB/EWKB入出力サポート (`ST_GeomFromWKB`, `ST_GeomFromEWKB`, `ST_AsBinary`, `ST_AsEWKB`)
- **関数**: 既存関数のMulti型対応

### Phase 3: 高度な空間演算 (v0.3)
- **関数**:
  - `ST_Intersection`, `ST_Union`, `ST_Difference`
  - `ST_ConvexHull`, `ST_Simplify`
  - `ST_Crosses`, `ST_Overlaps`, `ST_Touches`

### Phase 4: 3D/GeoJSON/集約 (v0.4)
- **3D対応**: Z座標サポート、`ST_3DDistance`
- **GeoJSON**: `ST_GeomFromGeoJSON`, `ST_AsGeoJSON`
- **集約関数**: `ST_Union` (aggregate), `ST_Extent`

### Phase 5: 座標変換 (v1.0)
- **SRID管理**: `spatial_ref_sys` テーブル
- **座標変換**: `ST_Transform` (PROJ.4連携)
- **測地系**: GeographicLib連携

---

## 5. テスト戦略

### 5.1 単体テスト
各関数に対して以下をテスト：
- 正常系: 期待される入力での動作確認
- 異常系: NULL入力、不正なWKT、型不一致
- 境界値: 極小/極大座標、退化した図形

### 5.2 統合テスト
実際のSQLクエリでの動作確認：
```sql
CREATE TABLE test_polygons (id INTEGER, geom TEXT);
INSERT INTO test_polygons VALUES 
  (1, 'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))'),
  (2, 'POLYGON((5 5, 15 5, 15 15, 5 15, 5 5))');

SELECT 
  a.id, b.id, 
  ST_Intersects(a.geom, b.geom) AS intersects,
  ST_Area(ST_Intersection(a.geom, b.geom)) AS intersection_area
FROM test_polygons a, test_polygons b
WHERE a.id < b.id;
```

### 5.3 パフォーマンステスト
- 10万件規模のポイントデータでの空間検索
- 複雑なポリゴンでのバッファ演算

---

## 6. APIデザイン原則

### 6.1 NULL処理
- **入力がNULLの場合**: 関数はNULLを返す（SQLの標準動作に準拠）
- **不正なWKT**: エラーメッセージを出力してNULLを返す

### 6.2 エラーハンドリング
```cpp
// エラー例
sqlite3_result_error(context, "sqlitegis: ST_Area requires Polygon or MultiPolygon", -1);
```

### 6.3 型チェック
Geometry型の識別には`ST_GeometryType()`を内部で使用し、
不適切な型が渡された場合は早期リターン。

---

## 7. Boost.Geometryとの対応

| PostGIS関数 | Boost.Geometry関数/アルゴリズム | 備考 |
|-------------|--------------------------------|------|
| `ST_GeomFromText` | `boost::geometry::read_wkt()` | SRID管理は独自実装 |
| `ST_GeomFromEWKT` | 手動パース + `read_wkt()` | `SRID=xxxx;` 接頭辞を分離 |
| `ST_AsText` | `boost::geometry::wkt()` | SRID情報を除外 |
| `ST_AsEWKT` | `boost::geometry::wkt()` | `SRID=xxxx;` 接頭辞を付与 |
| `ST_Area` | `boost::geometry::area()` | |
| `ST_Length` | `boost::geometry::length()` | |
| `ST_Distance` | `boost::geometry::distance()` | |
| `ST_Intersects` | `boost::geometry::intersects()` | |
| `ST_Within` | `boost::geometry::within()` | |
| `ST_Buffer` | `boost::geometry::buffer()` | |
| `ST_Centroid` | `boost::geometry::centroid()` | |
| `ST_ConvexHull` | `boost::geometry::convex_hull()` | |
| `ST_Intersection` | `boost::geometry::intersection()` | |
| `ST_Union` | `boost::geometry::union_()` | |

**注意**: Boost.GeometryはSRIDの概念を持たないため、SRID管理は独自のラッパークラスで実装します。

---

## 8. ドキュメント生成

各関数について以下を記載したリファレンスを作成：
- **構文**: `ST_Area(geometry) → double precision`
- **説明**: ポリゴンまたはマルチポリゴンの面積を返します。
- **引数**: `geometry` - Polygon型またはMultiPolygon型
- **戻り値**: 平面座標系での面積（単位は座標系に依存）
- **例**:
  ```sql
  -- WKT形式（SRID=0）
  SELECT ST_Area('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
  -- 結果: 100.0
  
  -- EWKT形式（SRID=4326、ただし平面計算）
  SELECT ST_Area('SRID=4326;POLYGON((139.69 35.68, 139.70 35.68, 139.70 35.69, 139.69 35.69, 139.69 35.68))');
  -- 結果: 0.0001（度単位での平面面積）
  ```
- **対応バージョン**: v0.1+
- **関連関数**: `ST_Perimeter`, `ST_Length`

### EWKT/EWKB使用例

#### SRID付きデータの作成
```sql
-- テーブル作成
CREATE TABLE cities (
    id INTEGER PRIMARY KEY,
    name TEXT,
    location TEXT  -- EWKT形式で格納
);

-- WGS84座標系でデータ挿入
INSERT INTO cities VALUES 
    (1, 'Tokyo', 'SRID=4326;POINT(139.6917 35.6895)'),
    (2, 'Osaka', 'SRID=4326;POINT(135.5023 34.6937)');

-- SRIDの確認
SELECT name, ST_SRID(location) AS srid FROM cities;
-- 結果:
-- Tokyo | 4326
-- Osaka | 4326
```

#### EWKT/WKT変換
```sql
-- EWKTからWKTへ（SRID情報を削除）
SELECT ST_AsText('SRID=4326;POINT(139.69 35.68)');
-- 結果: 'POINT(139.69 35.68)'

-- WKTにSRIDを設定してEWKTへ
SELECT ST_AsEWKT(ST_SetSRID('POINT(139.69 35.68)', 4326));
-- 結果: 'SRID=4326;POINT(139.69 35.68)'
```

---

## 9. ビルド構成

### 9.1 依存関係
- **必須**: CMake ≥ 3.20, SQLite3, Boost.Geometry (header-only)
- **オプション**: PROJ (座標変換), GeographicLib (測地系)

### 9.2 コンパイルフラグ
```cmake
target_compile_definitions(sqlitegis PRIVATE
    BOOST_GEOMETRY_DISABLE_DEPRECATED_03_WARNING
)
```

---

## 付録A: PostGIS互換性マトリクス

| 機能カテゴリ | PostGIS 3.x | SqliteGIS v0.1 | SqliteGIS v1.0 (目標) |
|--------------|-------------|----------------|---------------------|
| 2D Geometry | ✓ | ✓ | ✓ |
| 3D Geometry | ✓ | ✗ | ✓ |
| 測地系計算 | ✓ | ✗ | ✓ |
| 空間インデックス (R-Tree) | ✓ | ✗ (SQLite標準機能で代用可) | ✓ |
| ラスタデータ | ✓ | ✗ | ✗ |
| トポロジ | ✓ | ✗ | ✗ |

---

## 改訂履歴
- **2025-10-12**: 初版作成
- **2025-10-12**: EWKT/EWKB仕様を追加、Phase 1にEWKT対応を含める
