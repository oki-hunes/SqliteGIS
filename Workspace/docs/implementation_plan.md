# SqliteGIS 実装計画

## Phase 1 実装タスク (v0.1)

このドキュメントは、仕様書 (`specification.md`) に基づいた具体的な実装タスクを管理します。

---

## 1. アーキテクチャ設計

### 1.1 モジュール構成
```
include/sqlitegis/
  ├── geometry_types.hpp      // Geometry型定義、WKTパーサ
  ├── geometry_constructors.hpp // ST_GeomFromText, ST_MakePoint等
  ├── geometry_accessors.hpp   // ST_AsText, ST_X, ST_Y等
  ├── geometry_measures.hpp    // ST_Area, ST_Length等
  ├── geometry_relations.hpp   // ST_Intersects, ST_Contains等
  ├── geometry_operations.hpp  // ST_Buffer, ST_Centroid等
  └── geometry_utils.hpp       // ST_IsValid, ST_IsEmpty等

src/
  ├── geometry_types.cpp
  ├── geometry_constructors.cpp
  ├── geometry_accessors.cpp
  ├── geometry_measures.cpp
  ├── geometry_relations.cpp
  ├── geometry_operations.cpp
  ├── geometry_utils.cpp
  └── sqlitegis_extension.cpp  // エントリポイント
```

### 1.2 Geometry内部表現
```cpp
// EWKT文字列をラップする構造体（SRID管理機能付き）
struct GeometryWrapper {
    std::string wkt;       // WKT部分（SRID接頭辞なし）
    int srid = 0;          // SRID値（0=未定義/平面座標）
    
    // EWKT文字列からパース
    static std::optional<GeometryWrapper> from_ewkt(const std::string& ewkt);
    
    // EWKT文字列を生成
    std::string to_ewkt() const;
    
    // WKT文字列を取得（SRID情報なし）
    const std::string& to_wkt() const { return wkt; }
    
    // Boost.Geometry型へのパース（遅延評価）
    template<typename BoostGeomType>
    std::optional<BoostGeomType> as() const;
};

// EWKT パース例: "SRID=4326;POINT(139.69 35.68)"
// → wkt = "POINT(139.69 35.68)", srid = 4326
```

### 1.3 EWKT/EWKB パーサ設計

#### EWKTパーサ
```cpp
// "SRID=4326;POINT(...)" → {wkt: "POINT(...)", srid: 4326}
std::optional<GeometryWrapper> parse_ewkt(const std::string& input) {
    // 正規表現: ^SRID=(\d+);(.+)$
    std::regex ewkt_pattern(R"(^SRID=(\d+);(.+)$)", std::regex::icase);
    std::smatch match;
    
    if (std::regex_match(input, match, ewkt_pattern)) {
        GeometryWrapper geom;
        geom.srid = std::stoi(match[1].str());
        geom.wkt = match[2].str();
        return geom;
    }
    
    // SRID指定がない場合はSRID=0として扱う
    GeometryWrapper geom;
    geom.wkt = input;
    return geom;
}
```

#### EWKBパーサ（Phase 2実装）
```cpp
// バイナリヘッダからSRIDを抽出
struct EWKBHeader {
    uint8_t byte_order;     // 01=Little, 00=Big
    uint32_t wkb_type;      // 0x20000001 = Point + SRID flag
    uint32_t srid;          // SRID値（SRID flagが立っている場合のみ）
    
    bool has_srid() const { return (wkb_type & 0x20000000) != 0; }
    uint32_t base_type() const { return wkb_type & 0xFF; }
};

std::optional<GeometryWrapper> parse_ewkb(const std::vector<uint8_t>& blob);
```

---

## 2. Phase 1 実装関数リスト

### ✅ 実装済み
- [x] `ST_Area(geometry)` - 面積計算
- [x] `ST_Perimeter(geometry)` - 外周長計算
- [x] `ST_Length(geometry)` - 線長計算

### 📝 実装予定（優先度順）

#### 2.1 コンストラクタ (高優先度)
- [ ] `ST_GeomFromText(wkt TEXT)` - WKT → Geometry (SRID=0)
- [ ] `ST_GeomFromText(wkt TEXT, srid INT)` - WKT + SRID → Geometry
- [ ] `ST_GeomFromEWKT(ewkt TEXT)` - EWKT → Geometry (SRID自動抽出)
- [ ] `ST_MakePoint(x REAL, y REAL)` - 座標 → Point (SRID=0)
- [ ] `ST_SetSRID(geometry, srid INT)` - Geometryに任意のSRIDを設定

#### 2.2 アクセサ (高優先度)
- [ ] `ST_AsText(geometry)` - Geometry → WKT文字列（SRID除外）
- [ ] `ST_AsEWKT(geometry)` - Geometry → EWKT文字列（SRID含む）
- [ ] `ST_GeometryType(geometry)` - 型名取得 ("ST_Point", "ST_Polygon"等)
- [ ] `ST_SRID(geometry)` - SRID値取得
- [ ] `ST_X(point)` - Point のX座標
- [ ] `ST_Y(point)` - Point のY座標

#### 2.3 計測 (高優先度)
- [ ] `ST_Distance(geom1, geom2)` - 最短距離

#### 2.4 空間関係 (高優先度)
- [ ] `ST_Intersects(geom1, geom2)` - 交差判定
- [ ] `ST_Contains(geom1, geom2)` - 包含判定
- [ ] `ST_Within(geom1, geom2)` - 内包判定

#### 2.5 空間演算 (中優先度)
- [ ] `ST_Buffer(geometry, distance)` - バッファ領域生成
- [ ] `ST_Centroid(geometry)` - 重心計算

#### 2.6 ユーティリティ (高優先度)
- [ ] `ST_IsValid(geometry)` - 幾何妥当性検証
- [ ] `ST_IsEmpty(geometry)` - 空判定

---

## 3. 実装ガイドライン

### 3.1 共通パターン
各関数の実装は以下のテンプレートに従う：

```cpp
void st_function_name(sqlite3_context* ctx, int argc, sqlite3_value** argv) {
    // 1. 引数チェック
    if (argc != expected_argc) {
        sqlite3_result_error(ctx, "sqlitegis: 引数の数が不正", -1);
        return;
    }
    
    // 2. NULL処理
    if (sqlite3_value_type(argv[0]) == SQLITE_NULL) {
        sqlite3_result_null(ctx);
        return;
    }
    
    // 3. 入力取得
    auto input = read_geometry_argument(argv[0]);
    if (!input) {
        sqlite3_result_error(ctx, "sqlitegis: Geometry読み取り失敗", -1);
        return;
    }
    
    // 4. Boost.Geometryでの処理
    try {
        auto result = boost::geometry::some_algorithm(*input);
        sqlite3_result_double(ctx, result);  // or text, blob等
    } catch (const std::exception& e) {
        sqlite3_result_error(ctx, ("sqlitegis: " + std::string(e.what())).c_str(), -1);
    }
}
```

### 3.2 エラーメッセージ規約
すべてのエラーメッセージは `"sqlitegis: "` で始める。
```cpp
"sqlitegis: ST_Area requires Polygon or MultiPolygon"
"sqlitegis: Invalid WKT format"
"sqlitegis: Argument must be TEXT"
```

### 3.3 型チェック戦略
Boost.Geometryの`boost::geometry::read_wkt()`でパース時に型を判別：
```cpp
// variantで複数型を受け入れる
using geometry_variant = std::variant<
    point_t,
    linestring_t,
    polygon_t,
    multipoint_t,
    multilinestring_t,
    multipolygon_t
>;
```

---

## 4. テスト計画

### 4.1 テストファイル構成
```
tests/
  ├── test_constructors.sql   // ST_GeomFromText, ST_MakePoint
  ├── test_accessors.sql      // ST_AsText, ST_X, ST_Y
  ├── test_measures.sql       // ST_Area, ST_Length, ST_Distance
  ├── test_relations.sql      // ST_Intersects, ST_Contains
  ├── test_operations.sql     // ST_Buffer, ST_Centroid
  ├── test_utils.sql          // ST_IsValid, ST_IsEmpty
  └── run_tests.sh            // テスト実行スクリプト
```

### 4.2 テストケース例
```sql
-- test_constructors.sql
.load ./build/sqlitegis

-- ST_GeomFromText: SRID=0 (デフォルト)
SELECT ST_SRID(ST_GeomFromText('POINT(10 20)')) AS srid;
-- Expected: 0

-- ST_GeomFromText: SRID指定
SELECT ST_SRID(ST_GeomFromText('POINT(10 20)', 4326)) AS srid;
-- Expected: 4326

-- ST_GeomFromEWKT: SRID自動抽出
SELECT ST_SRID(ST_GeomFromEWKT('SRID=4326;POINT(139.69 35.68)')) AS srid;
-- Expected: 4326

-- ST_AsEWKT: EWKT出力
SELECT ST_AsEWKT(ST_GeomFromText('POINT(10 20)', 4326)) AS ewkt;
-- Expected: 'SRID=4326;POINT(10 20)'

-- ST_AsText: WKT出力（SRID除外）
SELECT ST_AsText(ST_GeomFromEWKT('SRID=4326;POINT(139.69 35.68)')) AS wkt;
-- Expected: 'POINT(139.69 35.68)'

-- ST_SetSRID: SRID変更（座標変換なし）
SELECT ST_AsEWKT(ST_SetSRID('POINT(10 20)', 3857)) AS ewkt;
-- Expected: 'SRID=3857;POINT(10 20)'
```

```sql
-- test_measures.sql
.load ./build/sqlitegis

-- ST_Area: 正方形（SRID=0）
SELECT ST_Area('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))') AS area;
-- Expected: 100.0

-- ST_Area: EWKT形式（SRID=4326、平面計算）
SELECT ST_Area('SRID=4326;POLYGON((0 0, 1 0, 1 1, 0 1, 0 0))') AS area;
-- Expected: 1.0 (Phase 1では測地系計算未対応)

-- ST_Distance: 2点間距離（SRID一致確認）
SELECT ST_Distance(
    'SRID=0;POINT(0 0)',
    'SRID=0;POINT(3 4)'
) AS dist;
-- Expected: 5.0

-- ST_Distance: SRID不一致エラー（将来実装）
-- SELECT ST_Distance('SRID=4326;POINT(0 0)', 'SRID=3857;POINT(0 0)');
-- Expected: Error (Phase 5で実装)
```

### 4.3 自動テスト実行
```bash
#!/bin/bash
# tests/run_tests.sh

SQLITE3=./third_party/sqlite-install/bin/sqlite3
EXTENSION=./build/sqlitegis.dylib

for test_file in tests/test_*.sql; do
    echo "Running $test_file..."
    $SQLITE3 ":memory:" < $test_file
    if [ $? -ne 0 ]; then
        echo "FAILED: $test_file"
        exit 1
    fi
done

echo "All tests passed!"
```

---

## 5. ビルドシステム改善

### 5.1 CMakeLists.txt の更新
```cmake
# ローカルビルドしたSQLiteを優先使用
set(SQLITE3_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite-install/include")
set(SQLITE3_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite-install/lib/libsqlite3.a")

# ソースファイル追加
add_library(sqlitegis SHARED
    src/sqlitegis_extension.cpp
    src/geometry_types.cpp
    src/geometry_constructors.cpp
    src/geometry_accessors.cpp
    src/geometry_measures.cpp
    src/geometry_relations.cpp
    src/geometry_operations.cpp
    src/geometry_utils.cpp
)

# テストターゲット追加
enable_testing()
add_test(NAME sqlitegis_tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh
)
```

---

## 6. 次回実装スケジュール

### Week 1: 基盤整備
- [ ] `geometry_types.hpp/cpp` の実装
  - `GeometryWrapper` 構造体（SRID管理機能）
  - EWKTパーサ（`SRID=xxxx;` 接頭辞の分離）
  - WKTパーサ統一（Boost.Geometryラッパー）
  - Geometry型判別ユーティリティ
- [ ] エラーハンドリング共通化

### Week 2: コンストラクタ・アクセサ
- [ ] `ST_GeomFromText` (1引数版: SRID=0)
- [ ] `ST_GeomFromText` (2引数版: SRID指定)
- [ ] `ST_GeomFromEWKT` (EWKT文字列からSRID自動抽出)
- [ ] `ST_SetSRID` (既存GeometryにSRID設定)
- [ ] `ST_MakePoint`
- [ ] `ST_AsText` (WKT出力)
- [ ] `ST_AsEWKT` (EWKT出力)
- [ ] `ST_GeometryType`
- [ ] `ST_SRID`
- [ ] `ST_X`, `ST_Y`

### Week 3: 計測・空間関係
- [ ] `ST_Distance`
- [ ] `ST_Intersects`
- [ ] `ST_Contains`, `ST_Within`

### Week 4: 空間演算・テスト
- [ ] `ST_Buffer`
- [ ] `ST_Centroid`
- [ ] 全関数のテストケース作成
- [ ] CI/CD設定（GitHub Actions）

---

## 7. 備考

### 参考リソース
- [PostGIS Documentation](https://postgis.net/docs/)
- [Boost.Geometry Documentation](https://www.boost.org/doc/libs/release/libs/geometry/)
- [OGC Simple Features](https://www.ogc.org/standards/sfa)

### 既知の課題
- Boost.Geometryの`buffer()`は複雑な設定が必要 → 別途調査
- **SRID管理**: Phase 1ではメモリ内でSRID値を保持、異なるSRID間の演算は警告なしで平面計算
  - Phase 5で`spatial_ref_sys`テーブル導入とSRID検証を実装
- **座標変換**: `ST_Transform()`はPhase 5でPROJ.4連携時に実装
- 集約関数はSQLiteの`sqlite3_create_function_v2()`の集約版を使用
- **EWKB実装**: Phase 2で優先実装、バイトオーダー対応が必要
