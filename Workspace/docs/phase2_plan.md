# SqliteGIS Phase 2 実装計画

## 🎯 Phase 2 目標: WKB/EWKB バイナリ形式対応

### 概要
WKB (Well-Known Binary) および EWKB (Extended Well-Known Binary) 形式のサポートを追加し、バイナリ形式でのジオメトリ保存・読み込みを実現します。

### メリット
1. **パフォーマンス向上**: テキスト形式(WKT)より高速
2. **容量削減**: バイナリ形式はテキストより小さい
3. **PostGIS互換**: PostGISのバイナリ形式と互換性
4. **データベース最適化**: BLOBカラムで効率的に保存

---

## 📋 実装機能

### 1. WKB/EWKB パーサ (geometry_types.hpp/cpp)

#### WKB形式仕様
```
[バイトオーダー(1)] [型(4)] [座標データ(可変)]

例: POINT(10 20)
01              - Little Endian
01 00 00 00     - WKB Type: Point (1)
00 00 00 00 00 00 24 40  - X座標: 10.0 (double)
00 00 00 00 00 00 34 40  - Y座標: 20.0 (double)
```

#### EWKB形式仕様 (PostGIS拡張)
```
[バイトオーダー(1)] [型+SRIDフラグ(4)] [SRID(4)] [座標データ(可変)]

例: SRID=4326;POINT(139.69 35.68)
01              - Little Endian
01 00 00 20     - EWKB Type: Point + SRID flag (0x20000001)
E6 10 00 00     - SRID: 4326
...             - X,Y座標
```

#### WKBタイプコード
| タイプ | コード | 名前 |
|--------|--------|------|
| Point | 1 | wkbPoint |
| LineString | 2 | wkbLineString |
| Polygon | 3 | wkbPolygon |
| MultiPoint | 4 | wkbMultiPoint |
| MultiLineString | 5 | wkbMultiLineString |
| MultiPolygon | 6 | wkbMultiPolygon |
| GeometryCollection | 7 | wkbGeometryCollection |

#### SRIDフラグ
- EWKBの場合、型コードに `0x20000000` をORする
- 例: Point with SRID = `0x20000001`

### 2. 新規関数 (4関数)

#### コンストラクタ (2関数)
- `ST_GeomFromWKB(wkb BLOB [, srid INT])` - WKBからジオメトリを生成
- `ST_GeomFromEWKB(ewkb BLOB)` - EWKBからジオメトリを生成 (SRID含む)

#### アクセサ (2関数)
- `ST_AsBinary(geom TEXT)` - WKB形式で出力 (BLOB)
- `ST_AsEWKB(geom TEXT)` - EWKB形式で出力 (BLOB、SRID含む)

---

## 🔧 実装手順

### Step 1: WKB/EWKBパーサ実装

#### geometry_types.hpp に追加
```cpp
// WKB/EWKB パース・シリアライズ
std::optional<GeometryWrapper> from_wkb(const std::vector<uint8_t>& wkb, int srid = 0);
std::optional<GeometryWrapper> from_ewkb(const std::vector<uint8_t>& ewkb);
std::vector<uint8_t> to_wkb() const;
std::vector<uint8_t> to_ewkb() const;

private:
// WKBヘッダー構造
enum class ByteOrder : uint8_t {
    BigEndian = 0,
    LittleEndian = 1
};

enum class WKBType : uint32_t {
    Point = 1,
    LineString = 2,
    Polygon = 3,
    MultiPoint = 4,
    MultiLineString = 5,
    MultiPolygon = 6
};

static constexpr uint32_t SRID_FLAG = 0x20000000;
```

#### geometry_types.cpp に実装
```cpp
std::optional<GeometryWrapper> GeometryWrapper::from_wkb(
    const std::vector<uint8_t>& wkb, int srid) {
    
    if (wkb.empty()) return std::nullopt;
    
    // バイトオーダー確認
    ByteOrder order = static_cast<ByteOrder>(wkb[0]);
    
    // 型コード読み取り (エンディアン考慮)
    uint32_t type = read_uint32(wkb, 1, order);
    
    // ジオメトリタイプごとにパース
    // ...
    
    return GeometryWrapper(parsed_geometry, srid);
}

std::vector<uint8_t> GeometryWrapper::to_wkb() const {
    std::vector<uint8_t> buffer;
    
    // バイトオーダー (Little Endian)
    buffer.push_back(static_cast<uint8_t>(ByteOrder::LittleEndian));
    
    // 型コード
    write_uint32(buffer, static_cast<uint32_t>(get_wkb_type()));
    
    // 座標データ
    // ...
    
    return buffer;
}
```

### Step 2: コンストラクタ関数

#### geometry_constructors.hpp/cpp
```cpp
// ST_GeomFromWKB(wkb BLOB [, srid INT])
void st_geom_from_wkb(sqlite3_context* ctx, int argc, sqlite3_value** argv);

// ST_GeomFromEWKB(ewkb BLOB)
void st_geom_from_ewkb(sqlite3_context* ctx, int argc, sqlite3_value** argv);
```

### Step 3: アクセサ関数

#### geometry_accessors.hpp/cpp
```cpp
// ST_AsBinary(geom TEXT) -> BLOB
void st_as_binary(sqlite3_context* ctx, int argc, sqlite3_value** argv);

// ST_AsEWKB(geom TEXT) -> BLOB
void st_as_ewkb(sqlite3_context* ctx, int argc, sqlite3_value** argv);
```

### Step 4: 拡張機能登録

#### sqlitegis_extension.cpp
```cpp
int register_binary_functions(sqlite3* db) {
    // ST_GeomFromWKB (1 or 2 args)
    sqlite3_create_function(db, "ST_GeomFromWKB", 1, ...);
    sqlite3_create_function(db, "ST_GeomFromWKB", 2, ...);
    
    // ST_GeomFromEWKB
    sqlite3_create_function(db, "ST_GeomFromEWKB", 1, ...);
    
    // ST_AsBinary
    sqlite3_create_function(db, "ST_AsBinary", 1, ...);
    
    // ST_AsEWKB
    sqlite3_create_function(db, "ST_AsEWKB", 1, ...);
    
    return SQLITE_OK;
}
```

---

## 🧪 テスト計画

### test_wkb.sql

```sql
-- WKB基本テスト
SELECT hex(ST_AsBinary('POINT(10 20)'));
-- Expected: 0101000000000000000000244000000000000034400

-- EWKB基本テスト
SELECT hex(ST_AsEWKB('SRID=4326;POINT(139.69 35.68)'));
-- Expected: 0101000020E6100000... (SRID=4326を含む)

-- Roundtrip テスト
SELECT ST_AsText(ST_GeomFromWKB(ST_AsBinary('POINT(10 20)')));
-- Expected: POINT(10 20)

-- EWKB Roundtrip
SELECT ST_AsEWKT(ST_GeomFromEWKB(ST_AsEWKB('SRID=4326;POINT(10 20)')));
-- Expected: SRID=4326;POINT(10 20)
```

---

## 📊 成功基準

- [ ] WKBパーサが全ジオメトリタイプをサポート
- [ ] EWKBでSRIDを正しく保存・復元
- [ ] エンディアン変換が正しく動作 (Big/Little Endian)
- [ ] PostGISのWKB/EWKBと互換性
- [ ] 全テストケースが通過
- [ ] ドキュメント更新完了

---

## 🎯 Phase 2 後の状態

**関数数**: 27関数 (Phase 1の23 + 4)

### 関数一覧
- コンストラクタ: 7関数 (5 + 2)
- アクセサ: 8関数 (6 + 2)
- 計測: 4関数
- 空間関係: 4関数
- 空間演算: 2関数
- ユーティリティ: 2関数

---

## 🔜 Phase 3 への準備

Phase 2完了後、以下を検討:
1. 3D座標(Z値)のサポート
2. M値(測定値)のサポート
3. WKB拡張タイプ (wkbPointZ = 1001等)
