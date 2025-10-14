# Phase 6: 座標変換システム (PROJ Integration) 実装計画

## 📋 概要

**目的**: PROJライブラリを統合し、PostGIS互換の座標参照系（CRS）変換機能を提供する。

**バージョン**: v0.6

**実装期間**: 2025年10月

## 🎯 実装する関数

### 優先度: 高 (Phase 6.1 - 必須)

| 関数名 | 説明 | PostGIS互換 |
|--------|------|-------------|
| `ST_Transform(geometry, target_srid)` | ジオメトリを別の座標参照系に変換 | ✅ |
| `ST_SetSRID(geometry, srid)` | SRIDを設定（座標は変換しない） | ✅ |

### 優先度: 中 (Phase 6.2 - 情報取得)

| 関数名 | 説明 | PostGIS互換 |
|--------|------|-------------|
| `ST_SRID(geometry)` | SRIDを取得（既に実装済み） | ✅ |
| `PROJ_Version()` | PROJライブラリのバージョン取得 | 🆕 独自 |
| `PROJ_GetCRSInfo(srid)` | CRS情報の取得 | 🆕 独自 |

### 優先度: 低 (将来実装)

| 関数名 | 説明 | PostGIS互換 |
|--------|------|-------------|
| `ST_TransformPipeline(geometry, pipeline)` | カスタム変換パイプライン | ✅ (PostGIS 3.4+) |
| `ST_AxisSwapped(geometry)` | 軸順序の入れ替え | 🆕 |

## 🔧 PROJ ライブラリについて

### PROJ とは

PROJ は地理座標変換のためのオープンソースライブラリです:

- **EPSG データベース**: 6,000以上の座標参照系定義
- **高精度変換**: 測地系、投影法、座標変換
- **グリッドシフト**: NAD27→NAD83, Tokyo Datum→JGD2000 など
- **PostGISでも使用**: 業界標準

### 主要なSRID例

| SRID | CRS名 | 説明 | 用途 |
|------|-------|------|------|
| 4326 | WGS 84 | GPS標準座標系（緯度経度） | 世界標準、Web地図 |
| 3857 | Web Mercator | Google/OSM投影 | Webマップタイル |
| 2451 | JGD2000 / Japan Plane Rectangular CS IX | 日本測地系2000（平面直角座標系 第IX系） | 日本の測量 |
| 6677 | JGD2011 / Japan Plane Rectangular CS IX | 日本測地系2011（平面直角座標系 第IX系） | 日本の最新測地系 |
| 4612 | JGD2000 | 日本測地系2000（緯度経度） | 日本標準 |
| 32654 | WGS 84 / UTM zone 54N | UTM座標系（日本付近） | 軍事・航空 |

## 🏗️ 技術設計

### 1. PROJ API (C API) の使用

PROJ 6.0+ の新しいC APIを使用:

```cpp
#include <proj.h>

// コンテキストの作成
PJ_CONTEXT* ctx = proj_context_create();

// 座標変換オブジェクトの作成
// EPSG:4326 (WGS84) → EPSG:3857 (Web Mercator)
PJ* transform = proj_create_crs_to_crs(
    ctx,
    "EPSG:4326",  // source CRS
    "EPSG:3857",  // target CRS
    nullptr       // area of use
);

// 座標変換の実行
PJ_COORD coord_in = proj_coord(lon, lat, 0, 0);
PJ_COORD coord_out = proj_trans(transform, PJ_FWD, coord_in);

// クリーンアップ
proj_destroy(transform);
proj_context_destroy(ctx);
```

### 2. ST_Transform 実装戦略

**基本的な流れ**:

```cpp
std::optional<GeometryWrapper> transform_geometry(
    const GeometryWrapper& geom,
    int target_srid
) {
    // 1. ソースSRIDとターゲットSRIDが同じなら変換不要
    if (geom.srid() == target_srid) {
        return geom;
    }
    
    // 2. PROJ変換オブジェクトを作成
    auto transform = create_proj_transform(geom.srid(), target_srid);
    if (!transform) {
        return std::nullopt;  // 変換不可
    }
    
    // 3. ジオメトリの座標を変換
    auto transformed = transform_coordinates(geom, transform);
    
    // 4. 新しいSRIDを設定
    transformed.set_srid(target_srid);
    
    return transformed;
}
```

**座標変換の詳細**:

```cpp
// Point の変換
point_t transform_point(const point_t& pt, PJ* transform) {
    PJ_COORD in = proj_coord(pt.x(), pt.y(), 0, 0);
    PJ_COORD out = proj_trans(transform, PJ_FWD, in);
    return point_t(out.xy.x, out.xy.y);
}

// LineString の変換
linestring_t transform_linestring(const linestring_t& line, PJ* transform) {
    linestring_t result;
    for (const auto& pt : line) {
        result.push_back(transform_point(pt, transform));
    }
    return result;
}

// Polygon の変換
polygon_t transform_polygon(const polygon_t& poly, PJ* transform) {
    polygon_t result;
    
    // 外環の変換
    auto& outer = result.outer();
    for (const auto& pt : poly.outer()) {
        outer.push_back(transform_point(pt, transform));
    }
    
    // 内環の変換
    for (const auto& inner : poly.inners()) {
        auto& result_inner = result.inners();
        linestring_t transformed_inner;
        for (const auto& pt : inner) {
            transformed_inner.push_back(transform_point(pt, transform));
        }
        result_inner.push_back(transformed_inner);
    }
    
    return result;
}
```

### 3. PROJ コンテキストの管理

**スレッドセーフ設計**:

```cpp
class ProjContext {
public:
    static ProjContext& instance() {
        static ProjContext ctx;
        return ctx;
    }
    
    PJ* create_transform(int source_srid, int target_srid) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // キャッシュをチェック
        auto key = std::make_pair(source_srid, target_srid);
        auto it = cache_.find(key);
        if (it != cache_.end()) {
            return it->second.get();  // キャッシュヒット
        }
        
        // 新しい変換を作成
        std::string src = "EPSG:" + std::to_string(source_srid);
        std::string dst = "EPSG:" + std::to_string(target_srid);
        
        PJ* pj = proj_create_crs_to_crs(ctx_, src.c_str(), dst.c_str(), nullptr);
        if (!pj) {
            return nullptr;  // 変換失敗
        }
        
        // キャッシュに保存
        cache_[key] = std::unique_ptr<PJ, ProjDeleter>(pj);
        return pj;
    }
    
private:
    ProjContext() : ctx_(proj_context_create()) {}
    ~ProjContext() { proj_context_destroy(ctx_); }
    
    PJ_CONTEXT* ctx_;
    std::mutex mutex_;
    std::map<std::pair<int, int>, std::unique_ptr<PJ, ProjDeleter>> cache_;
};
```

### 4. エラーハンドリング

```cpp
// PROJエラーの取得
std::string get_proj_error(PJ_CONTEXT* ctx) {
    int error_code = proj_context_errno(ctx);
    if (error_code == 0) return "";
    
    const char* error_msg = proj_errno_string(error_code);
    return std::string(error_msg);
}

// 変換の検証
bool validate_transform(int source_srid, int target_srid) {
    // SRID=-1 (未定義) は変換不可
    if (source_srid == -1 || target_srid == -1) {
        return false;
    }
    
    // テスト変換を試みる
    auto* pj = ProjContext::instance().create_transform(source_srid, target_srid);
    return pj != nullptr;
}
```

## 📁 ファイル構成

### 新規作成ファイル

1. **include/sqlitegis/geometry_transform.hpp**
   - 座標変換関数の宣言
   - ProjContext クラス
   
2. **src/geometry_transform.cpp**
   - ST_Transform 実装
   - ST_SetSRID 実装
   - PROJ_Version, PROJ_GetCRSInfo 実装
   - PROJ API ラッパー関数

### 既存ファイル修正

1. **CMakeLists.txt**
   - PROJ ライブラリの検出と追加
   - `find_package(PROJ REQUIRED)`
   - リンク設定

2. **src/sqlitegis_extension.cpp**
   - `register_transform_functions(db)` 呼び出し追加

3. **include/sqlitegis/geometry_types.hpp** (軽微な変更)
   - 必要に応じてヘルパー関数追加

## 🧪 テスト計画

### テストケース (tests/test_transform.sql)

```sql
-- Test 1: WGS84 → Web Mercator
.print 'Test 1: ST_Transform - WGS84 to Web Mercator'
SELECT ST_AsEWKT(
    ST_Transform(
        ST_MakePoint(139.6917, 35.6895, 4326),  -- 東京駅 (WGS84)
        3857  -- Web Mercator
    )
);
-- 期待値: SRID=3857;POINT(15543763.xx 4250209.xx) (近似値)

-- Test 2: Web Mercator → WGS84 (逆変換)
.print 'Test 2: ST_Transform - Web Mercator to WGS84'
SELECT ST_AsEWKT(
    ST_Transform(
        'SRID=3857;POINT(15543763.7 4250209.2)',
        4326
    )
);
-- 期待値: SRID=4326;POINT(139.6917 35.6895)

-- Test 3: 日本測地系 → WGS84
.print 'Test 3: ST_Transform - JGD2000 to WGS84'
SELECT ST_AsEWKT(
    ST_Transform(
        ST_MakePoint(139.0, 35.0, 4612),  -- JGD2000
        4326  -- WGS84
    )
);

-- Test 4: UTM変換
.print 'Test 4: ST_Transform - WGS84 to UTM Zone 54N'
SELECT ST_AsEWKT(
    ST_Transform(
        ST_MakePoint(140.0, 36.0, 4326),
        32654  -- UTM Zone 54N
    )
);

-- Test 5: LineString の変換
.print 'Test 5: ST_Transform - LineString'
SELECT ST_AsEWKT(
    ST_Transform(
        'SRID=4326;LINESTRING(139.0 35.0, 140.0 36.0)',
        3857
    )
);

-- Test 6: Polygon の変換
.print 'Test 6: ST_Transform - Polygon'
SELECT ST_AsEWKT(
    ST_Transform(
        'SRID=4326;POLYGON((139 35, 140 35, 140 36, 139 36, 139 35))',
        3857
    )
);

-- Test 7: ST_SetSRID (座標変換なし)
.print 'Test 7: ST_SetSRID - Change SRID without transformation'
SELECT ST_AsEWKT(
    ST_SetSRID(
        ST_MakePoint(139.0, 35.0),  -- SRID=-1
        4326
    )
);
-- 期待値: SRID=4326;POINT(139 35) (座標は変わらない)

-- Test 8: 同じSRID (変換スキップ)
.print 'Test 8: ST_Transform - Same SRID (no-op)'
SELECT ST_AsEWKT(
    ST_Transform(
        ST_MakePoint(139.0, 35.0, 4326),
        4326
    )
);
-- 期待値: 入力と同じ

-- Test 9: エラーハンドリング - 無効なSRID
.print 'Test 9: ST_Transform - Invalid SRID'
SELECT ST_Transform(
    ST_MakePoint(139.0, 35.0, 4326),
    999999  -- 存在しないSRID
);
-- 期待値: NULL または エラー

-- Test 10: PROJ_Version
.print 'Test 10: PROJ_Version'
SELECT PROJ_Version();
-- 期待値: "9.2.0" など

-- Test 11: PROJ_GetCRSInfo
.print 'Test 11: PROJ_GetCRSInfo'
SELECT PROJ_GetCRSInfo(4326);
-- 期待値: "WGS 84" など

-- Test 12: 3D Point の変換
.print 'Test 12: ST_Transform - 3D Point'
SELECT ST_AsEWKT(
    ST_Transform(
        ST_MakePointZ(139.0, 35.0, 100.0, 4326),
        3857
    )
);
-- 期待値: SRID=3857;POINT Z (...) (Z座標も保持)

-- Test 13: MultiPoint の変換
.print 'Test 13: ST_Transform - MultiPoint'
SELECT ST_AsEWKT(
    ST_Transform(
        'SRID=4326;MULTIPOINT((139 35), (140 36))',
        3857
    )
);
```

### パフォーマンステスト

```sql
-- 10,000点の一括変換
CREATE TEMP TABLE test_points AS
WITH RECURSIVE cnt(x) AS (
    SELECT 0
    UNION ALL
    SELECT x+1 FROM cnt WHERE x < 9999
)
SELECT 
    x as id,
    ST_AsEWKT(ST_MakePoint(
        135.0 + (x % 100) * 0.01,
        35.0 + (x / 100) * 0.01,
        4326
    )) as geom
FROM cnt;

.timer on
SELECT COUNT(*) FROM (
    SELECT ST_Transform(geom, 3857) as transformed
    FROM test_points
);
.timer off
```

## 📊 期待される成果

### 新規関数

- **ST_Transform**: 座標参照系の変換（メイン機能）
- **ST_SetSRID**: SRID設定（座標変換なし）
- **PROJ_Version**: PROJバージョン取得
- **PROJ_GetCRSInfo**: CRS情報取得

### 関数総数

- Phase 5: 32関数
- Phase 6: **36関数** (+4)

### ユースケース

1. **Web地図統合**: WGS84 (GPS) ↔ Web Mercator (Google Maps)
2. **測量データ変換**: 日本測地系 ↔ WGS84
3. **精密測位**: UTM座標系での高精度計算
4. **国際データ統合**: 異なる国の座標系を統一

## ⚠️ 制約事項と注意点

1. **PROJ依存**
   - PROJライブラリが必須（インストール必要）
   - データベースファイル（proj.db）のパスが正しく設定されている必要がある

2. **精度の問題**
   - 地球楕円体の違いによる誤差（数メートル～数十メートル）
   - 古い測地系では大きな誤差が発生する可能性

3. **パフォーマンス**
   - 座標変換は計算コストが高い
   - 大量データでは事前変換を推奨

4. **Z座標の扱い**
   - 標高（楕円体高 vs ジオイド高）の変換は複雑
   - 単純なZ座標コピーのみ実装（高度変換は未対応）

5. **SRID=-1 の扱い**
   - 未定義SRIDからの変換は不可
   - 明示的にSRIDを設定する必要あり

6. **軸順序の問題**
   - EPSG定義では latitude, longitude の順
   - PostGISとの互換性のため longitude, latitude で実装

## 🚀 実装ステップ

### Step 1: PROJ統合とビルド設定
- [ ] CMakeLists.txt に PROJ の検出追加
- [ ] ビルドテスト

### Step 2: ヘッダーとコンテキスト
- [ ] geometry_transform.hpp 作成
- [ ] ProjContext クラス実装（シングルトン）
- [ ] エラーハンドリング

### Step 3: ST_Transform 実装
- [ ] transform_point 実装
- [ ] transform_linestring 実装
- [ ] transform_polygon 実装
- [ ] transform_multi* 実装
- [ ] SQL関数ラッパー

### Step 4: ST_SetSRID 実装
- [ ] 単純なSRID設定（座標変換なし）

### Step 5: ユーティリティ関数
- [ ] PROJ_Version 実装
- [ ] PROJ_GetCRSInfo 実装

### Step 6: 登録とビルド
- [ ] register_transform_functions() 実装
- [ ] sqlitegis_extension.cpp に登録追加
- [ ] ビルドとリンクテスト

### Step 7: テストとドキュメント
- [ ] tests/test_transform.sql 作成
- [ ] 各関数のテストケース実装
- [ ] phase6_summary.md 作成
- [ ] README.md 更新

## 📈 成功基準

- ✅ PROJ 9.2.0 との正常なリンク
- ✅ ST_Transform が主要なSRID (4326, 3857, 4612) で動作
- ✅ 全ジオメトリ型（Point, LineString, Polygon, Multi*）対応
- ✅ 3D座標の保持
- ✅ エラーハンドリング（無効SRID、変換失敗）
- ✅ パフォーマンステスト（10,000点/秒以上）
- ✅ PostGIS互換のAPI

## 📚 参考資料

- [PROJ Documentation](https://proj.org/)
- [PROJ C API](https://proj.org/development/reference/cpp/index.html)
- [PostGIS ST_Transform](https://postgis.net/docs/ST_Transform.html)
- [EPSG Registry](https://epsg.org/)
- [Coordinate Systems](https://spatialreference.org/)

## 🌐 よく使うSRID一覧

### 世界標準
- **4326**: WGS 84 (GPS標準、緯度経度)
- **3857**: Web Mercator (Google Maps, OpenStreetMap)
- **4979**: WGS 84 (3D地心座標系)

### 日本
- **4612**: JGD2000 (日本測地系2000)
- **6668**: JGD2011 (日本測地系2011)
- **2451**: JGD2000 / Japan Plane Rectangular CS IX (平面直角座標系)
- **6677**: JGD2011 / Japan Plane Rectangular CS IX

### アメリカ
- **4269**: NAD83 (北米測地系)
- **4267**: NAD27 (旧北米測地系)
- **2163**: US National Atlas Equal Area

### ヨーロッパ
- **4258**: ETRS89 (欧州測地系)
- **3035**: ETRS89 / LAEA Europe

### UTM (Universal Transverse Mercator)
- **32654**: WGS 84 / UTM zone 54N (日本付近)
- **32655**: WGS 84 / UTM zone 55N
- **32601-32660**: WGS 84 / UTM 北半球
- **32701-32760**: WGS 84 / UTM 南半球
