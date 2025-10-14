# SqliteGIS Phase 3 実装計画

## 🎯 Phase 3 目標: 3D座標対応

### 概要
3D座標 (XYZ) をサポートし、3次元空間でのジオメトリ操作を可能にします。PostGISの3D関数に準拠した実装を行います。

### メリット
1. **3次元データ対応**: 建物の高さ、地形データ、航空機の軌跡など
2. **PostGIS互換**: PostGISの3D関数と互換性
3. **WKT/EWKB拡張**: 標準のZ座標フォーマット対応
4. **実用性向上**: GIS/BIMデータの統合処理が可能に

---

## 📋 実装機能

### 1. 3D Geometry型のサポート

#### 対応するジオメトリタイプ
- `POINT Z` - 3D点 (X, Y, Z)
- `LINESTRING Z` - 3D線
- `POLYGON Z` - 3D多角形
- `MULTIPOINT Z` - 3D複数点
- `MULTILINESTRING Z` - 3D複数線
- `MULTIPOLYGON Z` - 3D複数多角形

#### WKT形式 (ISO 13249-3準拠)
```sql
-- 3D Point
'POINT Z (10 20 30)'

-- 3D LineString
'LINESTRING Z (0 0 0, 1 1 1, 2 2 2)'

-- 3D Polygon
'POLYGON Z ((0 0 0, 10 0 5, 10 10 10, 0 10 5, 0 0 0))'

-- 3D Point with SRID
'SRID=4326;POINT Z (139.69 35.68 100)'
```

#### EWKB形式 (PostGIS互換)
```
3D座標用のwkbTypeフラグ:
- wkbZ (0x80000000) - Z座標あり
- 例: Point Z = 0x80000001 (wkbPoint | wkbZ)

EWKB構造 (Little Endian例):
[Byte Order: 1] [Type+Flags: 4] [SRID: 4] [X: 8] [Y: 8] [Z: 8]

例: SRID=4326;POINT Z(10 20 30)
01                    // Little Endian
01 00 00 A0           // wkbPoint | wkbZ | SRID_FLAG
E6 10 00 00           // SRID = 4326
00 00 00 00 00 00 24 40  // X = 10.0
00 00 00 00 00 00 34 40  // Y = 20.0
00 00 00 00 00 00 3E 40  // Z = 30.0
```

---

## 2. 新規実装関数

### 2.1 コンストラクタ (3関数)

| 関数名 | 引数 | 戻り値 | 説明 |
|--------|------|--------|------|
| `ST_MakePointZ` | `x REAL, y REAL, z REAL` | Geometry | 3D点を生成 |
| `ST_MakePointZM` | `x REAL, y REAL, z REAL, m REAL` | Geometry | 4D点を生成 (Z+M座標) |
| `ST_Force3D` | `geom TEXT [, z_default REAL]` | Geometry | 2Dジオメトリを3Dに変換 (Z=0またはz_default) |

**既存関数の拡張**:
- `ST_GeomFromText` - 3D WKT対応
- `ST_GeomFromEWKT` - 3D EWKT対応
- `ST_GeomFromEWKB` - 3D EWKB対応

### 2.2 アクセサ (4関数)

| 関数名 | 引数 | 戻り値 | 説明 |
|--------|------|--------|------|
| `ST_Z` | `point TEXT` | REAL | PointのZ座標を取得 (2Dの場合NULL) |
| `ST_M` | `point TEXT` | REAL | PointのM座標を取得 (測定値、Phase 4候補) |
| `ST_Is3D` | `geom TEXT` | INTEGER | 3Dジオメトリかどうか (1/0) |
| `ST_CoordDim` | `geom TEXT` | INTEGER | 座標次元数 (2/3/4) |

**既存関数の拡張**:
- `ST_AsText` - 3D WKT出力対応
- `ST_AsEWKT` - 3D EWKT出力対応
- `ST_AsEWKB` - 3D EWKB出力対応

### 2.3 計測関数 (4関数)

| 関数名 | 引数 | 戻り値 | 説明 |
|--------|------|--------|------|
| `ST_3DDistance` | `geom1 TEXT, geom2 TEXT` | REAL | 3D空間での最短距離 |
| `ST_3DLength` | `linestring TEXT` | REAL | 3D線の長さ |
| `ST_3DPerimeter` | `polygon TEXT` | REAL | 3D多角形の外周長 |
| `ST_3DArea` | `polygon TEXT` | REAL | 3D多角形の投影面積 (XY平面) |

**注意**: `ST_Area`, `ST_Length`, `ST_Perimeter`は3Dジオメトリでも2D計算を行う（PostGIS互換）

### 2.4 変換関数 (3関数)

| 関数名 | 引数 | 戻り値 | 説明 |
|--------|------|--------|------|
| `ST_Force2D` | `geom TEXT` | Geometry | 3DジオメトリをZ座標を削除して2Dに変換 |
| `ST_Force3DZ` | `geom TEXT [, z_default REAL]` | Geometry | 2Dジオメトリに強制的にZ座標を追加 |
| `ST_ZMin` | `geom TEXT` | REAL | ジオメトリの最小Z座標 |
| `ST_ZMax` | `geom TEXT` | REAL | ジオメトリの最大Z座標 |

---

## 3. 内部実装設計

### 3.1 Boost.Geometry の3D対応

Boost.Geometryは3D座標をサポートしています:

```cpp
// 3D Point型の定義
namespace bg = boost::geometry;
using point_3d_t = bg::model::point<double, 3, bg::cs::cartesian>;
using linestring_3d_t = bg::model::linestring<point_3d_t>;
using polygon_3d_t = bg::model::polygon<point_3d_t>;

// 3D距離計算
double dist = bg::distance(point_3d_t(0, 0, 0), point_3d_t(1, 1, 1));
// 結果: sqrt(3) ≈ 1.732
```

### 3.2 GeometryWrapper の拡張

```cpp
class GeometryWrapper {
public:
    std::string wkt_;
    int srid_ = -1;
    bool is_3d_ = false;  // 🆕 3D判定フラグ
    
    // 既存メソッド
    static std::optional<GeometryWrapper> from_ewkt(const std::string& ewkt);
    static std::optional<GeometryWrapper> from_wkt(const std::string& wkt, int srid = -1);
    static std::optional<GeometryWrapper> from_ewkb(const std::vector<uint8_t>& ewkb);
    
    std::string to_wkt() const;
    std::string to_ewkt() const;
    std::vector<uint8_t> to_ewkb() const;
    
    // 🆕 3D判定メソッド
    bool is_3d() const { return is_3d_; }
    int coord_dimension() const { return is_3d_ ? 3 : 2; }
    
    // 🆕 3D座標抽出 (Pointの場合)
    std::optional<double> get_z() const;
    
    // 🆕 2D/3D変換
    GeometryWrapper force_2d() const;
    GeometryWrapper force_3d(double z_default = 0.0) const;
};
```

### 3.3 WKT/EWKBパーサーの拡張

#### 3D WKT パース
```cpp
// "POINT Z (1 2 3)" のパース
std::optional<GeometryWrapper> parse_3d_wkt(const std::string& wkt) {
    // 正規表現: (POINT|LINESTRING|POLYGON) Z \(([^)]+)\)
    std::regex pattern(R"((\w+)\s+Z\s*\((.+)\))", std::regex::icase);
    std::smatch match;
    
    if (std::regex_match(wkt, match, pattern)) {
        GeometryWrapper geom;
        geom.wkt_ = wkt;
        geom.is_3d_ = true;
        return geom;
    }
    return std::nullopt;
}
```

#### 3D EWKB パース
```cpp
constexpr uint32_t WKB_Z_FLAG = 0x80000000;  // 3D座標フラグ

bool has_z_coordinate(uint32_t wkb_type) {
    return (wkb_type & WKB_Z_FLAG) != 0;
}

uint32_t base_geometry_type(uint32_t wkb_type) {
    return wkb_type & 0xFF;  // 下位8bitがジオメトリタイプ
}
```

---

## 4. 実装順序

### Week 1: 基盤整備
- [x] Phase 3計画ドキュメント作成
- [ ] `geometry_types.hpp` の拡張
  - `is_3d_` フラグ追加
  - 3D WKTパーサー実装
  - 3D EWKBパーサー実装
- [ ] Boost.Geometry 3D型の統合

### Week 2: コンストラクタ・アクセサ
- [ ] `ST_MakePointZ` 実装
- [ ] `ST_Z` 実装
- [ ] `ST_Is3D` 実装
- [ ] `ST_CoordDim` 実装
- [ ] `ST_GeomFromText` / `ST_AsText` の3D対応

### Week 3: 計測関数
- [ ] `ST_3DDistance` 実装
- [ ] `ST_3DLength` 実装
- [ ] `ST_3DPerimeter` 実装

### Week 4: 変換関数・テスト
- [ ] `ST_Force2D` / `ST_Force3D` 実装
- [ ] `ST_ZMin` / `ST_ZMax` 実装
- [ ] テストケース作成 (`tests/test_3d.sql`)
- [ ] ドキュメント更新

---

## 5. テスト計画

### 5.1 テストケース

```sql
-- 3D Point生成
SELECT ST_AsEWKT(ST_MakePointZ(10, 20, 30));
-- 期待値: 'SRID=-1;POINT Z (10 20 30)'

-- 3D距離計算
SELECT ST_3DDistance('POINT Z (0 0 0)', 'POINT Z (1 1 1)');
-- 期待値: 1.7320508075688772 (sqrt(3))

-- 2D/3D判定
SELECT ST_Is3D('POINT Z (1 2 3)');  -- 1
SELECT ST_Is3D('POINT (1 2)');      -- 0

-- 3D→2D変換
SELECT ST_AsText(ST_Force2D('POINT Z (1 2 3)'));
-- 期待値: 'POINT (1 2)'

-- 2D→3D変換
SELECT ST_AsText(ST_Force3D('POINT (1 2)', 100));
-- 期待値: 'POINT Z (1 2 100)'

-- Z座標範囲
SELECT ST_ZMin('LINESTRING Z (0 0 10, 1 1 20, 2 2 5)');  -- 5
SELECT ST_ZMax('LINESTRING Z (0 0 10, 1 1 20, 2 2 5)');  -- 20
```

---

## 6. PostGIS互換性

### 6.1 関数名と挙動の互換性

| SqliteGIS関数 | PostGIS互換 | 挙動の差異 |
|--------------|------------|----------|
| `ST_MakePointZ(x,y,z)` | ✅ 同じ | なし |
| `ST_Z(point)` | ✅ 同じ | なし |
| `ST_Is3D(geom)` | ✅ 同じ | なし |
| `ST_3DDistance(g1,g2)` | ✅ 同じ | なし |
| `ST_3DLength(line)` | ✅ 同じ | なし |
| `ST_Force2D(geom)` | ✅ 同じ | なし |
| `ST_Force3DZ(geom,z)` | ⚠️ 類似 | PostGISは`ST_Force3D`、引数順序が異なる |

### 6.2 制限事項

1. **M座標未対応**: Phase 3ではZ座標のみ対応、M座標はPhase 4で検討
2. **測地系計算**: 3D距離も平面座標系のみ、測地系はPhase 5
3. **3D空間演算**: `ST_3DIntersection`等はBoost.Geometryの制限により未対応

---

## 7. 期待される効果

### 7.1 利用シーン
- **建築/BIM**: 建物の高さ情報を含む3Dモデル
- **地形解析**: DEMデータ (Digital Elevation Model)
- **航空/ドローン**: 飛行経路の3D記録
- **地下構造**: トンネル、配管などの3D位置情報

### 7.2 パフォーマンス
- Boost.Geometryの3Dアルゴリズムを活用
- メモリオーバーヘッド: 座標1点あたり +8バイト (double 1個分)

---

## 8. 次のステップ (Phase 4候補)

Phase 3完了後の候補機能:
1. **M座標対応** (測定値、時刻など)
2. **GeoJSON 3D対応** (RFC 7946拡張)
3. **3D空間インデックス** (R-Tree 3D版)
4. **3D可視化サポート** (Cesium/three.js連携用出力)

---

## 9. 参考資料

- [PostGIS 3D Functions](https://postgis.net/docs/reference.html#PostGIS_3D_Functions)
- [ISO 13249-3:2016 (SQL/MM Part 3)](https://www.iso.org/standard/60343.html)
- [Boost.Geometry 3D Examples](https://www.boost.org/doc/libs/release/libs/geometry/doc/html/geometry/reference.html)
- [OGC Simple Features 3D](https://www.ogc.org/standards/sfa)

---

**実装開始日**: 2025年10月13日  
**目標完了日**: 2025年10月末  
**実装者**: AI Assistant + User
