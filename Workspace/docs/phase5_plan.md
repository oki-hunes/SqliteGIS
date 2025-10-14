# Phase 5: 集約関数 (Aggregate Functions) 実装計画

## 📋 概要

**目的**: PostGIS互換の空間集約関数を実装し、複数のジオメトリから単一のジオメトリを生成する機能を提供する。

**バージョン**: v0.5

**実装期間**: 2025年10月

## 🎯 実装する集約関数

### 優先度: 高 (Phase 5.1)

| 関数名 | 説明 | PostGIS互換 |
|--------|------|-------------|
| `ST_Collect(geometry set)` | ジオメトリセットをMulti*またはGeometryCollectionに変換 | ✅ |
| `ST_Union(geometry set)` | ジオメトリセットの和集合を計算（マージ） | ✅ |

### 優先度: 中 (Phase 5.2)

| 関数名 | 説明 | PostGIS互換 |
|--------|------|-------------|
| `ST_ConvexHull_Agg(geometry set)` | すべてのジオメトリを包含する凸包を計算 | ✅ (PostGIS 3.4+) |
| `ST_Extent_Agg(geometry set)` | すべてのジオメトリを包含するバウンディングボックス | ✅ (PostGIS ではST_Extent) |

### 優先度: 低 (将来実装)

| 関数名 | 説明 | PostGIS互換 |
|--------|------|-------------|
| `ST_MemUnion(geometry set)` | メモリ効率の良い和集合（大規模データ向け） | ✅ |
| `ST_ClusterDBSCAN(geometry, distance)` | DBSCAN法によるクラスタリング | ✅ |

## 🏗️ 技術設計

### 1. SQLite集約関数のアーキテクチャ

SQLiteの集約関数は3つのコールバックで実装:

```cpp
// 1. 初期化: 集約開始時に一度呼ばれる
void step_function(sqlite3_context* ctx, int argc, sqlite3_value** argv);

// 2. 累積処理: 各行ごとに呼ばれる
void step_function(sqlite3_context* ctx, int argc, sqlite3_value** argv);

// 3. 最終化: 集約終了時に結果を返す
void final_function(sqlite3_context* ctx);
```

### 2. 集約状態の管理

```cpp
struct AggregateContext {
    std::vector<GeometryWrapper> geometries;  // 累積されたジオメトリ
    int srid = -1;                            // 統一SRID
    bool has_error = false;                   // エラーフラグ
};
```

### 3. ST_Collect 実装戦略

**目的**: 複数のジオメトリを1つのMultiGeometryまたはGeometryCollectionにまとめる

```sql
-- 使用例
SELECT ST_AsEWKT(ST_Collect(geom)) FROM points;
-- 結果: SRID=4326;MULTIPOINT((0 0), (1 1), (2 2))
```

**実装ロジック**:
1. すべてのジオメトリが同じ型 → 対応するMulti型を生成
   - POINT → MULTIPOINT
   - LINESTRING → MULTILINESTRING
   - POLYGON → MULTIPOLYGON
2. 異なる型が混在 → GEOMETRYCOLLECTION を生成
3. SRID検証: すべて同じSRIDでなければエラー

**Boost.Geometry対応**:
- Multi型はBoost.Geometryで直接サポート
- GeometryCollectionは未サポート → 独自実装が必要

```cpp
// Multiタイプのコンストラクタ例
template<typename MultiType, typename SingleType>
MultiType create_multi(const std::vector<SingleType>& geometries) {
    MultiType multi;
    for (const auto& geom : geometries) {
        multi.push_back(geom);
    }
    return multi;
}
```

### 4. ST_Union 実装戦略

**目的**: 複数のジオメトリをトポロジー的にマージして1つのジオメトリにする

```sql
-- 使用例: 隣接するポリゴンを結合
SELECT ST_AsEWKT(ST_Union(geom)) FROM parcels WHERE district = 'downtown';
-- 結果: SRID=4326;POLYGON(...) または MULTIPOLYGON(...)
```

**実装ロジック**:
1. 2つずつペアワイズでunion演算を実行
2. Boost.Geometry の `boost::geometry::union_()` を使用
3. 結果が単一ポリゴンかMultiPolygonかは自動判定

**パフォーマンス考慮**:
- 大量のジオメトリ（10万件以上）では遅い可能性
- 将来的にはR-treeでの空間的グルーピング最適化を検討

```cpp
// 段階的Union実装
geometry_variant result = geometries[0];
for (size_t i = 1; i < geometries.size(); ++i) {
    std::vector<geometry_variant> output;
    boost::geometry::union_(result, geometries[i], output);
    if (output.size() == 1) {
        result = output[0];
    } else {
        // MultiPolygonに変換
        result = create_multipolygon(output);
    }
}
```

### 5. ST_ConvexHull_Agg 実装戦略

**目的**: すべてのジオメトリを包含する最小の凸多角形を計算

```sql
-- 使用例
SELECT ST_AsEWKT(ST_ConvexHull_Agg(geom)) FROM buildings WHERE city = 'Tokyo';
-- 結果: SRID=4326;POLYGON(...) -- 凸包
```

**実装ロジック**:
1. すべての頂点を収集
2. Boost.Geometry の `boost::geometry::convex_hull()` を実行
3. 結果はPOLYGONで返す

### 6. ST_Extent_Agg 実装戦略

**目的**: すべてのジオメトリを包含するバウンディングボックスを返す

```sql
-- 使用例
SELECT ST_Extent_Agg(geom) FROM cities;
-- 結果: BOX(120.5 30.2, 145.8 45.6)
```

**実装ロジック**:
1. 各ジオメトリのバウンディングボックスを計算（Phase 4の関数を再利用）
2. すべてのボックスを包含する最大のボックスを計算
3. "BOX(minX minY, maxX maxY)" 形式で返す

## 📁 ファイル構成

### 新規作成ファイル

1. **include/sqlitegis/geometry_aggregates.hpp**
   - 集約関数の登録関数宣言
   ```cpp
   void register_aggregate_functions(sqlite3* db);
   ```

2. **src/geometry_aggregates.cpp**
   - 集約関数の実装（step/final コールバック）
   - ST_Collect, ST_Union, ST_ConvexHull_Agg, ST_Extent_Agg

### 既存ファイル修正

1. **include/sqlitegis/geometry_types.hpp**
   - GeometryCollectionサポートの追加（必要に応じて）
   
2. **src/sqlitegis_extension.cpp**
   - `register_aggregate_functions(db)` の呼び出し追加

3. **CMakeLists.txt**
   - `src/geometry_aggregates.cpp` をビルドターゲットに追加

## 🧪 テスト計画

### テストケース (tests/test_aggregates.sql)

```sql
-- ST_Collect テスト
SELECT 'ST_Collect - POINT → MULTIPOINT' as test;
CREATE TABLE test_points (id INT, geom TEXT);
INSERT INTO test_points VALUES
    (1, ST_AsEWKT(ST_MakePoint(0, 0, 4326))),
    (2, ST_AsEWKT(ST_MakePoint(1, 1, 4326))),
    (3, ST_AsEWKT(ST_MakePoint(2, 2, 4326)));
SELECT ST_GeometryType(ST_Collect(geom)) FROM test_points;
-- 期待値: ST_MultiPoint

-- ST_Union テスト
SELECT 'ST_Union - 重なるポリゴン' as test;
CREATE TABLE test_polys (id INT, geom TEXT);
INSERT INTO test_polys VALUES
    (1, 'SRID=4326;POLYGON((0 0, 2 0, 2 2, 0 2, 0 0))'),
    (2, 'SRID=4326;POLYGON((1 1, 3 1, 3 3, 1 3, 1 1))');
SELECT ST_Area(ST_Union(geom)) FROM test_polys;
-- 期待値: 7.0 (重複部分は1回のみカウント)

-- ST_ConvexHull_Agg テスト
SELECT 'ST_ConvexHull_Agg - 点群の凸包' as test;
SELECT ST_GeometryType(ST_ConvexHull_Agg(geom)) FROM test_points;
-- 期待値: ST_Polygon

-- ST_Extent_Agg テスト
SELECT 'ST_Extent_Agg - バウンディングボックス' as test;
SELECT ST_Extent_Agg(geom) FROM test_points;
-- 期待値: BOX(0 0, 2 2)
```

### パフォーマンステスト

```sql
-- 大量データでのテスト (10万件)
CREATE TABLE perf_test AS
SELECT 
    row_number() OVER () as id,
    ST_AsEWKT(ST_MakePoint(
        random() % 180 - 90,
        random() % 90 - 45,
        4326
    )) as geom
FROM generate_series(1, 100000);

.timer on
SELECT ST_AsEWKT(ST_Collect(geom)) FROM perf_test;
SELECT ST_AsEWKT(ST_Union(geom)) FROM perf_test LIMIT 1000;  -- Unionは重い
SELECT ST_Extent_Agg(geom) FROM perf_test;
```

## 📊 期待される成果

### 新規関数

- **ST_Collect**: ジオメトリ集約（Multi型生成）
- **ST_Union**: トポロジーマージ（和集合）
- **ST_ConvexHull_Agg**: 凸包計算
- **ST_Extent_Agg**: 範囲集約

### 関数総数

- Phase 4: 28関数
- Phase 5: **32関数** (+4)

### ユースケース

1. **データ可視化**: 複数の建物をまとめて1つのポリゴンとして表示
2. **空間分析**: 地区ごとの範囲計算
3. **データ集約**: 点群を1つのMultiPointに変換してストレージ削減

## ⚠️ 制約事項

1. **GeometryCollection制限**: Boost.GeometryがGeometryCollectionを直接サポートしていないため、WKT文字列での独自実装が必要
2. **3D Union未対応**: Boost.Geometryの制約により、ST_Unionは2Dのみ対応
3. **大規模データ性能**: ST_Unionは計算量が多いため、10万件以上では遅延の可能性
4. **メモリ使用量**: すべてのジオメトリをメモリに保持するため、大量データでは注意

## 🚀 実装ステップ

### Step 1: ファイル作成とヘッダー定義
- [ ] `include/sqlitegis/geometry_aggregates.hpp` 作成
- [ ] `src/geometry_aggregates.cpp` 作成
- [ ] CMakeLists.txt 更新

### Step 2: ST_Collect 実装
- [ ] AggregateContext 構造体定義
- [ ] collect_step() 関数実装
- [ ] collect_final() 関数実装
- [ ] Multi型生成ロジック

### Step 3: ST_Union 実装
- [ ] union_step() 関数実装
- [ ] union_final() 関数実装（boost::geometry::union_使用）
- [ ] ペアワイズUnionロジック

### Step 4: ST_ConvexHull_Agg 実装
- [ ] convex_hull_step() 関数実装
- [ ] convex_hull_final() 関数実装
- [ ] boost::geometry::convex_hull 使用

### Step 5: ST_Extent_Agg 実装
- [ ] extent_step() 関数実装（Phase 4関数再利用）
- [ ] extent_final() 関数実装
- [ ] BOX文字列生成

### Step 6: 登録とビルド
- [ ] register_aggregate_functions() 実装
- [ ] sqlitegis_extension.cpp に登録呼び出し追加
- [ ] ビルドテスト

### Step 7: テストとドキュメント
- [ ] tests/test_aggregates.sql 作成
- [ ] 各関数のテストケース実装
- [ ] phase5_summary.md 作成
- [ ] README.md 更新

## 📈 成功基準

- ✅ 4つの集約関数が正常に動作
- ✅ PostGIS互換のAPI（関数名、引数、戻り値）
- ✅ SRID情報の正しい継承
- ✅ 2D/3D両対応（Unionは2Dのみ）
- ✅ エラーハンドリング（SRID不一致、NULL値処理）
- ✅ パフォーマンステスト完了（1万件規模）
- ✅ 包括的なテストカバレッジ

## 📚 参考資料

- [PostGIS Aggregate Functions](https://postgis.net/docs/reference.html#Aggregate_Functions)
- [SQLite Aggregate Functions](https://www.sqlite.org/appfunc.html)
- [Boost.Geometry Union](https://www.boost.org/doc/libs/1_71_0/libs/geometry/doc/html/geometry/reference/algorithms/union_.html)
- [Boost.Geometry Convex Hull](https://www.boost.org/doc/libs/1_71_0/libs/geometry/doc/html/geometry/reference/algorithms/convex_hull.html)
