# SqliteGIS

PostGIS互換のSQLite GIS拡張機能

## 概要

SqliteGISは、SQLiteにGIS（地理情報システム）機能を追加するローダブル拡張です。PostGISとの互換性を重視し、EWKT/EWKB形式のジオメトリ処理と4次元座標システム（XY/XYZ/XYM/XYZM）をサポートします。

### 主な特徴

- ✅ **PostGIS互換:** EWKT/EWKB形式に準拠
- ✅ **4次元座標:** 2D (XY), 3D (XYZ), M座標 (XYM), 4D (XYZM) をサポート
- ✅ **SRID対応:** すべてのジオメトリでSRID管理（デフォルト: -1 = 未定義）
- ✅ **高性能:** C++20とBoost.Geometryによる最適化実装
- ✅ **軽量:** 外部データベース不要、SQLiteに直接ロード可能

### バージョン情報

- **Current Version:** v0.6 (Phase 6完了)
- **開発開始:** 2025年10月
- **ビルド環境:** macOS, C++20, CMake
- **依存ライブラリ:** Boost.Geometry 1.71+, PROJ 9.2.0

## 開発フェーズ

### Phase 1: 基礎実装 ✅
- 基本的なジオメトリ型（Point, LineString, Polygon）
- WKT/EWKT パーサー
- 基本的な空間関数

### Phase 2: EWKB統合 ✅
- WKB単体サポートの削除
- EWKB形式への完全移行
- SRIDデフォルト値変更（0 → -1）

### Phase 3: 4次元座標システム ✅
- DimensionType アーキテクチャ（XY/XYZ/XYM/XYZM）
- WKB_M_FLAG サポート
- 3D関数実装（ST_MakePointZ, ST_Z, ST_Is3D等）
- 2D/3D変換関数（ST_Force2D, ST_Force3D）

### Phase 4: 空間インデックス ✅
- バウンディングボックス関数（8個）
- SQLite R-tree拡張との統合
- 高速空間検索サポート

### Phase 5: 集約関数 ✅
- ST_Collect（ジオメトリ集約）
- ST_Union（トポロジーマージ）
- ST_ConvexHull_Agg（凸包計算）
- ST_Extent_Agg（範囲集約）

### Phase 6: 座標変換システム ✅ (現在)
- PROJ 9.2.0 統合
- ST_Transform（座標参照系変換）
- ST_SetSRID（SRID設定）
- 6,000以上のEPSG CRS対応
- **関数総数:** 36個

## 対応する座標次元

SqliteGISは4種類の座標次元をサポートします:

| 次元タイプ | 座標 | 次元数 | 説明 |
|-----------|------|--------|------|
| **XY** | (X, Y) | 2 | 標準的な2D座標 |
| **XYZ** | (X, Y, Z) | 3 | 3D座標（高さ・深さ） |
| **XYM** | (X, Y, M) | 3 | 2D + 測定値（距離、時間等） |
| **XYZM** | (X, Y, Z, M) | 4 | 3D + 測定値 |

### WKT表記例

```sql
-- 2D
POINT(1 2)
LINESTRING(0 0, 1 1)

-- 3D (Z座標)
POINT Z (1 2 3)
LINESTRING Z (0 0 0, 1 1 1)

-- 2D + Measure
POINT M (1 2 100)
LINESTRING M (0 0 0, 1 1 50)

-- 4D (Z + M)
POINT ZM (1 2 3 100)
LINESTRING ZM (0 0 0 0, 1 1 1 50)
```

## インストールとビルド

### 必要な環境

- **コンパイラ:** C++20対応（GCC 10+, Clang 12+, AppleClang 14+）
- **CMake:** 3.15以上
- **Boost.Geometry:** 1.71以上
- **SQLite:** 3.46.0 (含まれています)

### ビルド手順

```bash
# リポジトリのクローン
git clone <repository-url>
cd SqliteGIS/Workspace

# ビルドディレクトリの作成
mkdir -p build
cd build

# CMake設定
cmake ..

# ビルド実行
make

# 成果物: sqlitegis.dylib (macOS) または sqlitegis.so (Linux)
```

### SQLiteへのロード

```bash
# SQLiteを起動
sqlite3 test.db

# 拡張機能をロード
.load ./build/sqlitegis

# 動作確認
SELECT ST_AsEWKT(ST_MakePoint(139.7, 35.7, 4326));
-- 結果: SRID=4326;POINT(139.7 35.7)
```

## SQL関数リファレンス

### コンストラクタ関数 (6個)

| 関数 | 説明 | 例 |
|------|------|-----|
| **ST_GeomFromText**(wkt, srid) | WKTからジオメトリ作成 | `ST_GeomFromText('POINT(1 2)', 4326)` |
| **ST_GeomFromEWKT**(ewkt) | EWKTからジオメトリ作成 | `ST_GeomFromEWKT('SRID=4326;POINT(1 2)')` |
| **ST_GeomFromEWKB**(ewkb) | EWKBからジオメトリ作成 | `ST_GeomFromEWKB(x'...')` |
| **ST_MakePoint**(x, y [, srid]) | 2D Point作成 | `ST_MakePoint(1.0, 2.0, -1)` |
| **ST_MakePointZ**(x, y, z [, srid]) | 3D Point作成 | `ST_MakePointZ(1.0, 2.0, 3.0, 4326)` |
| **ST_MakeLine**(point1, point2) | 2点からLineString作成 | `ST_MakeLine(p1, p2)` |

### アクセサー関数 (10個)

| 関数 | 説明 | 例 |
|------|------|-----|
| **ST_AsText**(geom) | WKT形式で出力 | `ST_AsText(geom)` → `'POINT(1 2)'` |
| **ST_AsEWKT**(geom) | EWKT形式で出力 | `ST_AsEWKT(geom)` → `'SRID=4326;POINT(1 2)'` |
| **ST_AsEWKB**(geom) | EWKB形式で出力（バイナリ） | `ST_AsEWKB(geom)` |
| **ST_SRID**(geom) | SRID取得 | `ST_SRID(geom)` → `4326` |
| **ST_X**(geom) | X座標取得 | `ST_X(point)` → `1.0` |
| **ST_Y**(geom) | Y座標取得 | `ST_Y(point)` → `2.0` |
| **ST_Z**(geom) | Z座標取得 | `ST_Z(point_3d)` → `3.0` |
| **ST_GeometryType**(geom) | ジオメトリタイプ取得 | `ST_GeometryType(geom)` → `'POINT'` |
| **ST_Is3D**(geom) | 3D判定 | `ST_Is3D(point_3d)` → `1` |
| **ST_CoordDim**(geom) | 座標次元数取得 | `ST_CoordDim(geom)` → `2/3/4` |

### 測定関数 (4個)

| 関数 | 説明 | 例 |
|------|------|-----|
| **ST_Distance**(geom1, geom2) | 2点間距離（2D/3D対応） | `ST_Distance(p1, p2)` → `1.414` |
| **ST_Length**(geom) | LineStringの長さ | `ST_Length(line)` → `100.5` |
| **ST_Area**(geom) | Polygonの面積 | `ST_Area(polygon)` → `50.0` |
| **ST_Perimeter**(geom) | Polygonの周長 | `ST_Perimeter(polygon)` → `28.28` |

### 空間関係関数 (4個)

| 関数 | 説明 | 例 |
|------|------|-----|
| **ST_Intersects**(geom1, geom2) | 交差判定 | `ST_Intersects(g1, g2)` → `1/0` |
| **ST_Contains**(geom1, geom2) | 包含判定 | `ST_Contains(polygon, point)` → `1/0` |
| **ST_Within**(geom1, geom2) | 内包判定 | `ST_Within(point, polygon)` → `1/0` |
| **ST_Disjoint**(geom1, geom2) | 非交差判定 | `ST_Disjoint(g1, g2)` → `1/0` |

### 操作関数 (4個)

| 関数 | 説明 | 例 |
|------|------|-----|
| **ST_Centroid**(geom) | 重心取得 | `ST_Centroid(polygon)` |
| **ST_Buffer**(geom, distance) | バッファー生成 | `ST_Buffer(point, 10.0)` |
| **ST_Force2D**(geom) | 2D変換（Z/M削除） | `ST_Force2D(point_3d)` |
| **ST_Force3D**(geom [, z_default]) | 3D変換（Z追加） | `ST_Force3D(point, 0.0)` |

### バウンディングボックス関数 (8個)

| 関数 | 説明 | 例 |
|------|------|-----|
| **ST_Envelope**(geom) | バウンディングボックス（POLYGON） | `ST_Envelope(line)` → `POLYGON(...)` |
| **ST_Extent**(geom) | 範囲文字列 | `ST_Extent(geom)` → `'BOX(0 0, 10 10)'` |
| **ST_XMin**(geom) | X座標の最小値 | `ST_XMin(geom)` → `0.0` |
| **ST_XMax**(geom) | X座標の最大値 | `ST_XMax(geom)` → `10.0` |
| **ST_YMin**(geom) | Y座標の最小値 | `ST_YMin(geom)` → `0.0` |
| **ST_YMax**(geom) | Y座標の最大値 | `ST_YMax(geom)` → `10.0` |
| **ST_ZMin**(geom) | Z座標の最小値（3Dのみ） | `ST_ZMin(geom_3d)` → `5.0` |
| **ST_ZMax**(geom) | Z座標の最大値（3Dのみ） | `ST_ZMax(geom_3d)` → `15.0` |

### 集約関数 (4個) 🆕 Phase 5

| 関数 | 説明 | 例 |
|------|------|-----|
| **ST_Collect**(geom) | ジオメトリコレクション作成 | `SELECT ST_Collect(geom) FROM table` |
| **ST_Union**(geom) | ジオメトリの結合（トポロジ処理） | `SELECT ST_Union(polygon) FROM table` |
| **ST_ConvexHull_Agg**(geom) | 凸包の集約計算 | `SELECT ST_ConvexHull_Agg(geom) FROM table` |
| **ST_Extent_Agg**(geom) | バウンディングボックスの集約 | `SELECT ST_Extent_Agg(geom) FROM table` |

### 座標変換関数 (4個) 🆕 Phase 6

| 関数 | 説明 | 例 |
|------|------|-----|
| **ST_Transform**(geom, target_srid) | 座標変換（6,000+EPSG対応） | `ST_Transform(geom, 3857)` |
| **ST_SetSRID**(geom, srid) | SRIDのみ変更（座標は不変） | `ST_SetSRID(geom, 4326)` |
| **PROJ_Version**() | PROJライブラリのバージョン | `PROJ_Version()` → `'9.2.0'` |
| **PROJ_GetCRSInfo**(srid) | 座標参照系の名称取得 | `PROJ_GetCRSInfo(4326)` → `'WGS 84'` |

**関数総数: 36個** (v0.6時点)

## 使用例

### 基本的なPoint操作

```sql
-- 2D Pointの作成
SELECT ST_AsEWKT(ST_MakePoint(139.7, 35.7, 4326));
-- 結果: SRID=4326;POINT(139.7 35.7)

-- 3D Pointの作成
SELECT ST_AsEWKT(ST_MakePointZ(139.7, 35.7, 100.0, 4326));
-- 結果: SRID=4326;POINT Z (139.7 35.7 100)

-- 座標の取得
SELECT ST_X(ST_MakePoint(1.0, 2.0));  -- 1.0
SELECT ST_Y(ST_MakePoint(1.0, 2.0));  -- 2.0
SELECT ST_Z(ST_MakePointZ(1.0, 2.0, 3.0));  -- 3.0
```

### 2D/3D変換

```sql
-- 3D → 2D変換
SELECT ST_AsEWKT(
    ST_Force2D(ST_MakePointZ(1, 2, 3))
);
-- 結果: SRID=-1;POINT(1 2)

-- 2D → 3D変換（デフォルトZ=0）
SELECT ST_AsEWKT(
    ST_Force3D(ST_MakePoint(1, 2))
);
-- 結果: SRID=-1;POINT Z (1 2 0)

-- 2D → 3D変換（Z値指定）
SELECT ST_AsEWKT(
    ST_Force3D(ST_MakePoint(1, 2), 10.0)
);
-- 結果: SRID=-1;POINT Z (1 2 10)
```

### 距離計算（2D/3D）

```sql
-- 2D距離
SELECT ST_Distance(
    ST_MakePoint(0, 0),
    ST_MakePoint(3, 4)
);
-- 結果: 5.0

-- 3D距離
SELECT ST_Distance(
    ST_MakePointZ(0, 0, 0),
    ST_MakePointZ(1, 1, 1)
);
-- 結果: 1.732... (√3)
```

### 集約関数 🆕 Phase 5

```sql
-- 複数のポイントをまとめる（ST_Collect）
CREATE TABLE points (id INTEGER, geom TEXT);
INSERT INTO points VALUES
    (1, ST_AsEWKT(ST_MakePoint(0, 0, 4326))),
    (2, ST_AsEWKT(ST_MakePoint(1, 1, 4326))),
    (3, ST_AsEWKT(ST_MakePoint(2, 2, 4326)));

SELECT ST_AsEWKT(ST_Collect(ST_GeomFromEWKT(geom))) FROM points;
-- 結果: SRID=4326;MULTIPOINT(0 0,1 1,2 2)

-- ポリゴンの結合（ST_Union）
SELECT ST_AsEWKT(ST_Union(ST_GeomFromEWKT(geom))) FROM polygons;

-- バウンディングボックスの集約（ST_Extent_Agg）
SELECT ST_Extent_Agg(ST_GeomFromEWKT(geom)) FROM buildings;
-- 結果: 'BOX(0 0, 100 100)'
```

### 座標変換 🆕 Phase 6

```sql
-- PROJ ライブラリのバージョン確認
SELECT PROJ_Version();
-- 結果: '9.2.0'

-- 座標参照系の情報取得
SELECT PROJ_GetCRSInfo(4326);
-- 結果: 'WGS 84'

-- WGS 84 (EPSG:4326) から Web Mercator (EPSG:3857) への変換
SELECT ST_AsEWKT(
    ST_Transform(
        ST_MakePoint(139.7454, 35.6586, 4326),  -- 東京タワー (経度, 緯度)
        3857                                     -- Web Mercator
    )
);
-- 結果: SRID=3857;POINT(15556070.04 4256425.78)

-- 日本測地系 JGD2000 (EPSG:4612) から JGD2011 (EPSG:6668) への変換
SELECT ST_AsEWKT(
    ST_Transform(
        ST_MakePoint(139.7454, 35.6586, 4612),
        6668
    )
);

-- SRIDの設定（座標は変更しない）
SELECT ST_AsEWKT(
    ST_SetSRID(ST_MakePoint(139.7, 35.7), 4326)
);
-- 結果: SRID=4326;POINT(139.7 35.7)
```

### テーブルでの使用

```sql
-- ジオメトリカラムを持つテーブル作成
CREATE TABLE places (
    id INTEGER PRIMARY KEY,
    name TEXT,
    location TEXT  -- EWKT形式でジオメトリを保存
);

-- データ挿入
INSERT INTO places (name, location) VALUES
    ('東京タワー', ST_AsEWKT(ST_MakePoint(139.7454, 35.6586, 4326))),
    ('スカイツリー', ST_AsEWKT(ST_MakePoint(139.8107, 35.7101, 4326)));

-- 距離計算
SELECT 
    p1.name,
    p2.name,
    ST_Distance(
        ST_GeomFromEWKT(p1.location),
        ST_GeomFromEWKT(p2.location)
    ) as distance
FROM places p1, places p2
WHERE p1.id < p2.id;
```

### 3D建物データの管理

```sql
-- 3D建物データ
CREATE TABLE buildings (
    id INTEGER PRIMARY KEY,
    name TEXT,
    footprint TEXT,     -- 2D形状（EWKT）
    height REAL,        -- 高さ（メートル）
    base_point TEXT     -- 基準点（3D EWKT）
);

-- データ挿入
INSERT INTO buildings (name, footprint, height, base_point) VALUES
    ('ビルA', 
     ST_AsEWKT(ST_GeomFromText('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))', 4326)),
     50.0,
     ST_AsEWKT(ST_MakePointZ(5, 5, 0, 4326)));

-- 3D高さ情報の取得
SELECT 
    name,
    ST_Z(ST_GeomFromEWKT(base_point)) as base_elevation,
    height,
    ST_Z(ST_GeomFromEWKT(base_point)) + height as top_elevation
FROM buildings;
```

### R-tree空間インデックス 🆕

```sql
-- ジオメトリテーブルの作成
CREATE TABLE parks (
    id INTEGER PRIMARY KEY,
    name TEXT,
    geom TEXT  -- EWKT形式
);

-- R-tree仮想テーブルの作成
CREATE VIRTUAL TABLE parks_idx USING rtree(
    id,        -- 主キー（parks.idと対応）
    minX, maxX,
    minY, maxY
);

-- データ挿入とインデックス作成をトリガーで自動化
CREATE TRIGGER parks_insert AFTER INSERT ON parks BEGIN
    INSERT INTO parks_idx VALUES (
        NEW.id,
        ST_XMin(NEW.geom),
        ST_XMax(NEW.geom),
        ST_YMin(NEW.geom),
        ST_YMax(NEW.geom)
    );
END;

-- データ挿入
INSERT INTO parks (name, geom) VALUES
    ('代々木公園', ST_AsEWKT(ST_GeomFromText(
        'POLYGON((139.69 35.67, 139.70 35.67, 139.70 35.68, 139.69 35.68, 139.69 35.67))', 4326))),
    ('新宿御苑', ST_AsEWKT(ST_GeomFromText(
        'POLYGON((139.71 35.68, 139.72 35.68, 139.72 35.69, 139.71 35.69, 139.71 35.68))', 4326)));

-- 高速空間検索（バウンディングボックスによる絞り込み）
SELECT p.name
FROM parks p
WHERE p.id IN (
    SELECT id FROM parks_idx
    WHERE minX <= 139.705 AND maxX >= 139.695
      AND minY <= 35.685 AND maxY >= 35.675
);
```

## EWKT/EWKB フォーマット

### EWKT (Extended Well-Known Text)

```
SRID=<srid>;<geometry_type> [Z] [M] (<coordinates>)
```

例:
```sql
SRID=4326;POINT(139.7 35.7)              -- 2D
SRID=4326;POINT Z (139.7 35.7 100)       -- 3D
SRID=4326;POINT M (139.7 35.7 50)        -- 2D + Measure
SRID=4326;POINT ZM (139.7 35.7 100 50)   -- 4D
```

### EWKB (Extended Well-Known Binary)

EWKBはバイナリ形式で、以下のフラグを使用:

| フラグ | 値 | 説明 |
|--------|-----|------|
| WKB_SRID_FLAG | 0x20000000 | SRID情報を含む（常に設定） |
| WKB_Z_FLAG | 0x80000000 | Z座標を含む |
| WKB_M_FLAG | 0x40000000 | M座標を含む |

組み合わせ:
- **XY**: `WKB_SRID_FLAG`
- **XYZ**: `WKB_SRID_FLAG | WKB_Z_FLAG`
- **XYM**: `WKB_SRID_FLAG | WKB_M_FLAG`
- **XYZM**: `WKB_SRID_FLAG | WKB_Z_FLAG | WKB_M_FLAG`

## 技術スタック

### コア技術
- **言語:** C++20
- **空間ライブラリ:** Boost.Geometry 1.71+
- **データベース:** SQLite 3.46.0
- **ビルドシステム:** CMake 3.15+

### アーキテクチャ

```
┌─────────────────────────────────┐
│     SQLite Extension API        │
├─────────────────────────────────┤
│   SQL Function Registration     │
│  (ST_MakePoint, ST_Distance...)  │
├─────────────────────────────────┤
│      GeometryWrapper Class      │
│  - DimensionType管理             │
│  - EWKT/EWKB変換                │
│  - SRID管理                      │
├─────────────────────────────────┤
│      Boost.Geometry             │
│  - point_t / point_3d_t         │
│  - linestring_t / polygon_t     │
│  - 空間アルゴリズム              │
└─────────────────────────────────┘
```

### ファイル構成

```
Workspace/
├── CMakeLists.txt              # ビルド設定
├── include/sqlitegis/
│   ├── geometry_types.hpp      # コア型定義（DimensionType等）
│   ├── geometry_constructors.hpp
│   ├── geometry_accessors.hpp
│   ├── geometry_measures.hpp
│   ├── geometry_relations.hpp
│   └── geometry_operations.hpp
├── src/
│   ├── geometry_types.cpp      # EWKT/EWKB実装
│   ├── geometry_constructors.cpp
│   ├── geometry_accessors.cpp
│   ├── geometry_measures.cpp
│   ├── geometry_relations.cpp
│   ├── geometry_operations.cpp
│   └── sqlitegis_extension.cpp # 拡張エントリーポイント
├── tests/
│   ├── test_3d.sql             # 3D機能テスト
│   ├── test_accessors.sql
│   ├── test_constructors.sql
│   └── run_tests.sh
├── docs/
│   ├── specification.md
│   ├── phase1_summary.md
│   ├── phase2_summary.md
│   ├── phase3_plan.md
│   └── phase3_summary.md       # Phase 3実装詳細
└── third_party/
    └── sqlite-autoconf-3460000/ # SQLite本体
```

## テスト

### テストスイート

現在57個のテストケースが実装されています:

```bash
# テストの実行（SQLite CLIが必要）
cd tests
./run_tests.sh
```

**注:** macOS互換性の問題により、一部のテストは実行を保留中

### テストカテゴリ

1. **Constructors** - ジオメトリ作成
2. **Accessors** - 座標・属性アクセス
3. **Measures** - 距離・面積計算
4. **Relations** - 空間関係
5. **Operations** - 変換・演算
6. **EWKB** - バイナリ変換
7. **3D Functions** - 3D固有機能

## パフォーマンス

### 設計方針
- **ゼロコピー:** 可能な限りメモリコピーを回避
- **遅延評価:** 必要になるまでWKT/EWKB変換を遅延
- **型安全性:** std::variant による型安全な実装
- **最適化:** C++20機能とBoost最適化の活用

### ベンチマーク（予定）
- Point作成: ~1μs
- 距離計算: ~2μs
- EWKB変換: ~5μs

## 貢献

### 開発ガイドライン
1. C++20標準に準拠
2. PostGIS互換性を維持
3. すべての関数にテストを追加
4. ドキュメントを更新

### コーディングスタイル
- インデント: 4スペース
- 命名: snake_case（関数）、PascalCase（クラス）
- コメント: 日本語OK

## ライセンス

[ライセンス情報を追加予定]

## 参考資料

### PostGIS ドキュメント
- [PostGIS Reference](https://postgis.net/docs/)
- [EWKT/EWKB Specification](https://postgis.net/docs/using_postgis_dbmanagement.html)

### Boost.Geometry
- [Boost.Geometry Documentation](https://www.boost.org/doc/libs/release/libs/geometry/)
- [Spatial Index](https://www.boost.org/doc/libs/release/libs/geometry/doc/html/geometry/spatial_indexes.html)

### SQLite
- [SQLite Extension Interface](https://www.sqlite.org/loadext.html)
- [Virtual Table](https://www.sqlite.org/vtab.html)

## 変更履歴

### v0.3 (2025-10-13) - Phase 3完了
- ✅ 4次元座標システム（XY/XYZ/XYM/XYZM）
- ✅ DimensionType アーキテクチャ
- ✅ WKB_M_FLAG サポート
- ✅ 6個の新規関数（ST_MakePointZ, ST_Z, ST_Is3D, ST_CoordDim, ST_Force2D, ST_Force3D）
- ✅ 関数総数: 20個

### v0.2 (Phase 2完了)
- ✅ EWKB統合
- ✅ SRIDデフォルト -1 変更
- ✅ 25個の関数実装

### v0.1 (Phase 1完了)
- ✅ 基本ジオメトリ型
- ✅ WKT/EWKT パーサー
- ✅ 基本空間関数

---

**SqliteGIS** - Bringing GIS power to SQLite
