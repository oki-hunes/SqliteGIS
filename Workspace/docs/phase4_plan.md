# Phase 4 実装計画: 空間インデックス (R-tree)

## 概要

Phase 4では、SQLiteのVirtual Table機構を使用したR-tree空間インデックスを実装します。これにより、大量のジオメトリデータに対する高速な空間検索が可能になります。

**目標バージョン:** v0.4  
**実装期間:** 2025年10月13日〜  
**前回Phase:** Phase 3 (4次元座標システム)

## 目的

1. **高速な空間検索**: MBR (Minimum Bounding Rectangle) ベースの高速検索
2. **大規模データ対応**: 数万〜数百万件のジオメトリに対応
3. **SQLite標準機能の活用**: Virtual Table APIを使用
4. **PostGIS互換**: PostGISのGiSTインデックスに類似した機能

## 技術仕様

### R-tree Virtual Table

SQLiteのVirtual Table機構を使用してR-treeインデックスを実装します。

#### テーブル構成

```sql
CREATE VIRTUAL TABLE spatial_index USING rtree(
    id,              -- ジオメトリの一意ID (INTEGER)
    minX, maxX,      -- X軸のバウンディングボックス (REAL)
    minY, maxY,      -- Y軸のバウンディングボックス (REAL)
    minZ, maxZ       -- Z軸のバウンディングボックス (REAL, オプション)
);
```

#### 2D R-tree (4カラム)
```sql
CREATE VIRTUAL TABLE rtree_2d USING rtree(
    id,
    minX, maxX,
    minY, maxY
);
```

#### 3D R-tree (6カラム)
```sql
CREATE VIRTUAL TABLE rtree_3d USING rtree(
    id,
    minX, maxX,
    minY, maxY,
    minZ, maxZ
);
```

### 実装方針

#### Option 1: SQLite組み込みR-tree拡張を使用（推奨）

SQLiteには既にR-tree拡張が含まれています（`SQLITE_ENABLE_RTREE`）。

**メリット:**
- 実装が簡単
- 十分にテスト済み
- パフォーマンスが保証されている
- メンテナンス不要

**デメリット:**
- カスタマイズが困難
- SQLiteのビルド設定に依存

#### Option 2: 独自Virtual Table実装

C++ Virtual Table APIを使用して独自実装。

**メリット:**
- 完全なコントロール
- ジオメトリ型との統合が容易
- カスタム最適化が可能

**デメリット:**
- 実装コストが高い
- バグのリスク
- メンテナンスが必要

**決定**: **Option 1** を採用し、SQLite組み込みR-treeを活用します。

## 実装する機能

### 1. ヘルパー関数（6個）

#### ST_Envelope(geometry) → POLYGON
ジオメトリのバウンディングボックスをPOLYGONとして返す

```sql
SELECT ST_AsEWKT(ST_Envelope('LINESTRING(0 0, 10 10)'));
-- 結果: SRID=-1;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))
```

#### ST_Extent(geometry) → TEXT
バウンディングボックスを "BOX(minX minY, maxX maxY)" 形式で返す

```sql
SELECT ST_Extent('LINESTRING(0 0, 10 10)');
-- 結果: BOX(0 0, 10 10)
```

#### ST_XMin(geometry) → REAL
X座標の最小値を取得

```sql
SELECT ST_XMin('LINESTRING(0 0, 10 10)');
-- 結果: 0.0
```

#### ST_XMax(geometry) → REAL
X座標の最大値を取得

```sql
SELECT ST_XMax('LINESTRING(0 0, 10 10)');
-- 結果: 10.0
```

#### ST_YMin(geometry) → REAL
Y座標の最小値を取得

```sql
SELECT ST_YMin('LINESTRING(0 0, 10 10)');
-- 結果: 0.0
```

#### ST_YMax(geometry) → REAL
Y座標の最大値を取得

```sql
SELECT ST_YMax('LINESTRING(0 0, 10 10)');
-- 結果: 10.0
```

### 2. 3D対応関数（2個）

#### ST_ZMin(geometry) → REAL
Z座標の最小値を取得（3Dジオメトリのみ）

```sql
SELECT ST_ZMin('LINESTRING Z (0 0 5, 10 10 15)');
-- 結果: 5.0
```

#### ST_ZMax(geometry) → REAL
Z座標の最大値を取得（3Dジオメトリのみ）

```sql
SELECT ST_ZMax('LINESTRING Z (0 0 5, 10 10 15)');
-- 結果: 15.0
```

### 3. インデックス作成ドキュメント

Virtual Table の作成方法とベストプラクティスをドキュメント化。

## 使用例

### 基本的な使用パターン

```sql
-- 1. ジオメトリテーブルの作成
CREATE TABLE buildings (
    id INTEGER PRIMARY KEY,
    name TEXT,
    geometry TEXT  -- EWKT形式
);

-- 2. R-treeインデックスの作成
CREATE VIRTUAL TABLE buildings_idx USING rtree(
    id,
    minX, maxX,
    minY, maxY
);

-- 3. データ挿入とインデックス更新
INSERT INTO buildings (id, name, geometry) VALUES
    (1, 'Building A', 'SRID=4326;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');

INSERT INTO buildings_idx (id, minX, maxX, minY, maxY)
    SELECT 
        id,
        ST_XMin(geometry),
        ST_XMax(geometry),
        ST_YMin(geometry),
        ST_YMax(geometry)
    FROM buildings WHERE id = 1;

-- 4. 空間検索クエリ
-- 範囲内のすべてのビルを検索
SELECT b.*
FROM buildings b
JOIN buildings_idx idx ON b.id = idx.id
WHERE idx.minX <= 5 AND idx.maxX >= 5
  AND idx.minY <= 5 AND idx.maxY >= 5;

-- 5. より精密な検索（R-treeで絞り込み後、正確な判定）
SELECT b.*
FROM buildings b
JOIN buildings_idx idx ON b.id = idx.id
WHERE idx.minX <= 5 AND idx.maxX >= 5
  AND idx.minY <= 5 AND idx.maxY >= 5
  AND ST_Contains(b.geometry, ST_MakePoint(5, 5));
```

### トリガーによる自動インデックス更新

```sql
-- INSERT時の自動更新
CREATE TRIGGER buildings_insert_idx
AFTER INSERT ON buildings
BEGIN
    INSERT INTO buildings_idx (id, minX, maxX, minY, maxY)
    VALUES (
        NEW.id,
        ST_XMin(NEW.geometry),
        ST_XMax(NEW.geometry),
        ST_YMin(NEW.geometry),
        ST_YMax(NEW.geometry)
    );
END;

-- UPDATE時の自動更新
CREATE TRIGGER buildings_update_idx
AFTER UPDATE ON buildings
BEGIN
    UPDATE buildings_idx SET
        minX = ST_XMin(NEW.geometry),
        maxX = ST_XMax(NEW.geometry),
        minY = ST_YMin(NEW.geometry),
        maxY = ST_YMax(NEW.geometry)
    WHERE id = NEW.id;
END;

-- DELETE時の自動更新
CREATE TRIGGER buildings_delete_idx
AFTER DELETE ON buildings
BEGIN
    DELETE FROM buildings_idx WHERE id = OLD.id;
END;
```

### 3D空間インデックス

```sql
-- 3D R-treeインデックス
CREATE VIRTUAL TABLE buildings_3d_idx USING rtree(
    id,
    minX, maxX,
    minY, maxY,
    minZ, maxZ
);

-- 3Dデータのインデックス作成
INSERT INTO buildings_3d_idx (id, minX, maxX, minY, maxY, minZ, maxZ)
SELECT 
    id,
    ST_XMin(geometry),
    ST_XMax(geometry),
    ST_YMin(geometry),
    ST_YMax(geometry),
    ST_ZMin(geometry),
    ST_ZMax(geometry)
FROM buildings_3d
WHERE ST_Is3D(geometry);

-- 3D範囲検索
SELECT b.*
FROM buildings_3d b
JOIN buildings_3d_idx idx ON b.id = idx.id
WHERE idx.minX <= 10 AND idx.maxX >= 0
  AND idx.minY <= 10 AND idx.maxY >= 0
  AND idx.minZ <= 50 AND idx.maxZ >= 0;  -- 高さ50m以下の建物
```

## 実装タスク

### タスク1: バウンディングボックス計算の実装
- [ ] GeometryWrapper::get_bbox() メソッド実装
  - 2D: {minX, maxX, minY, maxY}
  - 3D: {minX, maxX, minY, maxY, minZ, maxZ}
- [ ] Boost.Geometryの`envelope`アルゴリズムを使用
- [ ] Point, LineString, Polygon, Multi*型に対応

### タスク2: ST_Envelope関数の実装
- [ ] バウンディングボックスをPOLYGONとして返す
- [ ] 2D/3D対応（3DはZ値は無視）
- [ ] SRID保持

### タスク3: ST_Extent関数の実装
- [ ] "BOX(minX minY, maxX maxY)" 形式の文字列を返す
- [ ] PostGIS互換フォーマット

### タスク4: ST_XMin/XMax/YMin/YMax関数の実装
- [ ] 各座標軸の最小/最大値を返す（4関数）
- [ ] NULLハンドリング

### タスク5: ST_ZMin/ZMax関数の実装
- [ ] 3Dジオメトリ専用
- [ ] 2DジオメトリではNULL返却

### タスク6: ドキュメント作成
- [ ] R-tree使用ガイド
- [ ] トリガー設定例
- [ ] パフォーマンスチューニング
- [ ] ベストプラクティス

### タスク7: テスト作成
- [ ] test_rtree.sql
  - バウンディングボックス計算テスト
  - インデックス作成テスト
  - 検索パフォーマンステスト
  - トリガー動作テスト
- [ ] 大規模データでのベンチマーク

## パフォーマンス目標

### ベンチマーク条件
- データ件数: 10,000〜100,000件
- 検索範囲: 全体の1%〜10%

### 期待される性能
- **R-treeなし**: O(n) - 全件スキャン
- **R-treeあり**: O(log n) - 対数時間

### 実測目標
- 10,000件: < 10ms
- 100,000件: < 50ms
- 1,000,000件: < 200ms

## 技術的な詳細

### Boost.Geometry Envelope API

```cpp
#include <boost/geometry/algorithms/envelope.hpp>

// 2D envelope
box_t bbox;
boost::geometry::envelope(geometry, bbox);

// アクセス
double minX = bg::get<bg::min_corner, 0>(bbox);
double maxX = bg::get<bg::max_corner, 0>(bbox);
double minY = bg::get<bg::min_corner, 1>(bbox);
double maxY = bg::get<bg::max_corner, 1>(bbox);
```

### SQLite R-tree API確認

SQLiteがR-tree拡張を有効にしているか確認:

```sql
-- R-tree拡張の確認
SELECT * FROM pragma_compile_options WHERE compile_options LIKE '%RTREE%';

-- もしくは
CREATE VIRTUAL TABLE test_rtree USING rtree(id, minX, maxX, minY, maxY);
-- エラーが出なければR-tree有効
```

### GeometryWrapperへの追加メソッド

```cpp
struct BoundingBox {
    double minX, maxX;
    double minY, maxY;
    std::optional<double> minZ, maxZ;  // 3Dの場合のみ
};

class GeometryWrapper {
public:
    // 既存メソッド...
    
    // 新規メソッド
    BoundingBox get_bbox() const;
    GeometryWrapper envelope() const;  // バウンディングボックスをPOLYGONとして
};
```

## PostGIS互換性

PostGISの対応する関数:

| SqliteGIS | PostGIS | 互換性 |
|-----------|---------|--------|
| ST_Envelope | ST_Envelope | ✅ 完全互換 |
| ST_Extent | ST_Extent | ✅ 完全互換 |
| ST_XMin | ST_XMin | ✅ 完全互換 |
| ST_XMax | ST_XMax | ✅ 完全互換 |
| ST_YMin | ST_YMin | ✅ 完全互換 |
| ST_YMax | ST_YMax | ✅ 完全互換 |
| ST_ZMin | ST_ZMin | ✅ 完全互換 |
| ST_ZMax | ST_ZMax | ✅ 完全互換 |

## 制限事項

1. **R-tree拡張の依存**: SQLiteがR-tree拡張を有効にしている必要がある
2. **手動インデックス管理**: トリガーを手動で設定する必要がある
3. **ストレージオーバーヘッド**: インデックステーブルが追加のストレージを消費
4. **M座標非対応**: R-treeはXYZ座標のみサポート（M座標は無視）

## 今後の拡張（Phase 5以降）

1. **自動インデックス管理**: 
   - CREATE INDEX構文のサポート
   - 自動トリガー生成

2. **最適化クエリ書き換え**: 
   - SQLiteクエリプランナーとの統合
   - 自動的なR-tree利用

3. **複合インデックス**: 
   - 空間 + 属性の複合インデックス

4. **Quad-tree/Grid**: 
   - 代替インデックス構造

## まとめ

Phase 4では、8個の新規関数（バウンディングボックス関連）を実装し、SQLite R-tree拡張と統合して高速な空間検索を実現します。

**実装関数:** 8個
- ST_Envelope
- ST_Extent
- ST_XMin, ST_XMax
- ST_YMin, ST_YMax
- ST_ZMin, ST_ZMax

**期待される効果:**
- 大規模データでの検索速度向上（10〜1000倍）
- PostGIS互換のインデックス機能
- 実用的なGISアプリケーション構築が可能に

次のステップ: 実装開始！
