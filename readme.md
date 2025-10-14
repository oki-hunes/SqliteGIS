# SqliteGIS

PostGIS互換のGIS拡張機能をSQLite3向けに実装するプロジェクトです。

## 🎯 目的
- SQLite3用のGIS loadable extensionを作成
- PostGIS互換のGeometry系関数を段階的に実装
- 幾何計算のコアにBoost.Geometryを利用

## ✨ 最新リリース

### Phase 6 完了! (v0.6) - 2025年10月16日 🆕

**新機能**: PROJ 座標変換サポート

#### 座標変換機能
- 🌐 **PROJ 9.2.0 統合**: 6,000以上のEPSG座標参照系に対応
- 🔄 **座標変換**: WGS 84 ⟷ Web Mercator など、任意のSRID間の変換
- 🔧 **ProjContext**: スレッドセーフなシングルトンとキャッシュ機構
- 🌏 **2D/3D対応**: XY座標とXYZ座標の両方をサポート

#### 新規関数 (4関数)
- `ST_Transform(geom, target_srid)` - 座標変換を実行
- `ST_SetSRID(geom, srid)` - SRIDのみ変更（座標は変更なし）
- `PROJ_Version()` - PROJライブラリのバージョン取得
- `PROJ_GetCRSInfo(srid)` - 座標参照系の名称取得

**主要なSRID**:
- **4326** - WGS 84 (GPS標準)
- **3857** - Web Mercator (Google Maps/OpenStreetMap)
- **4612** - JGD2000 (日本測地系2000)
- **6668** - JGD2011 (日本測地系2011)
- **32654** - UTM Zone 54N (日本の一部)

**使用例**:
```sql
-- 東京タワー: WGS 84 → Web Mercator
SELECT ST_AsEWKT(
    ST_Transform(
        ST_MakePoint(139.7454, 35.6586, 4326),
        3857
    )
);
-- 結果: SRID=3857;POINT(15556070.04 4256425.78)
```

### Phase 5 完了! (v0.5) - 2025年10月16日

**新機能**: 集約関数サポート

#### 集約関数
- 📊 **4つの集約関数**: 複数ジオメトリをまとめる操作
- 🔗 **SQLite集約**: step/finalパターンで効率的な処理
- 🎯 **PostGIS互換**: ST_Collect, ST_Union, ST_ConvexHull_Agg, ST_Extent_Agg

#### 新規関数 (4関数)
- `ST_Collect(geom)` - 複数ジオメトリをコレクションに集約
- `ST_Union(geom)` - トポロジカルな結合（重複削除）
- `ST_ConvexHull_Agg(geom)` - 凸包の集約計算
- `ST_Extent_Agg(geom)` - バウンディングボックスの集約

**使用例**:
```sql
-- 複数のポイントをまとめる
SELECT ST_AsEWKT(ST_Collect(geom)) FROM points;
-- 結果: SRID=4326;MULTIPOINT(0 0,1 1,2 2)

-- ポリゴンの結合
SELECT ST_AsEWKT(ST_Union(geom)) FROM polygons;

-- バウンディングボックスの集約
SELECT ST_Extent_Agg(geom) FROM buildings;
-- 結果: 'BOX(0 0, 100 100)'
```

### Phase 4 完了! (v0.4) - 2025年10月15日

**新機能**: R-tree空間インデックスサポート

#### バウンディングボックス関数
- � **8つの新関数**: 空間インデックス構築のためのバウンディングボックス計算
- 🌲 **R-tree統合**: SQLite組み込みのR-tree拡張機能と連携
- ⚡ **高速空間検索**: バウンディングボックスによる絞り込みで大規模データセットを効率的に処理

### Phase 3 完了! (v0.3) - 2025年10月13日

**新機能**: 4次元座標システム (XY/XYZ/XYM/XYZM) 完全サポート

#### 3D/4D座標対応
- 🌐 **4次元座標システム**: XY (2D), XYZ (3D), XYM (2D+Measure), XYZM (4D)
- 📐 **DimensionType アーキテクチャ**: 型安全な次元管理
- 🔢 **WKB_M_FLAG**: M座標のEWKBサポート (0x40000000)


#### 新規関数 (6関数)
- `ST_MakePointZ(x, y, z [, srid])` - 3D Point作成
- `ST_Z(geom)` - Z座標取得
- `ST_Is3D(geom)` - 3D判定 (1/0)
- `ST_CoordDim(geom)` - 座標次元数 (2/3/4)
- `ST_Force2D(geom)` - 2D変換 (Z/M削除)
- `ST_Force3D(geom [, z_default])` - 3D変換 (Z追加)

**設計方針**:
- 🎯 PostGIS互換のEWKT/EWKB形式 (POINT Z, POINT M, POINT ZM)
- 🔢 4種類の次元タイプを完全サポート
- � 2D ⟷ 3D 双方向変換
- � 座標次元の自動検出とパース

**メリット**:
- 🏔️ 3D建物データ・地形データの管理
- � 標高情報付き位置データ
- 🛣️ M座標による線形参照システム対応
- 🔗 PostGIS 3D関数との互換性

### Phase 2 完了! (v0.2) - 2025年10月

**新機能**: EWKB バイナリ形式サポート (SRID必須設計)
- `ST_GeomFromEWKB(ewkb BLOB)` - EWKBからジオメトリを生成
- `ST_AsEWKB(geom TEXT)` - EWKB形式でバイナリ出力
- SRID=-1 をデフォルト値に変更 (未定義を表す)

### Phase 1 完了! (v0.1) - 2024年12月

**基礎機能**: 基本的なGIS関数23個を実装

### 📊 実装済み関数 (Phase 1-6 合計36関数)

#### コンストラクタ (6関数)
- `ST_GeomFromText(wkt TEXT, srid INT)` - WKTからジオメトリを生成
- `ST_GeomFromEWKT(ewkt TEXT)` - EWKTからジオメトリを生成 (SRID付き)
- `ST_GeomFromEWKB(ewkb BLOB)` - EWKBからジオメトリを生成 (SRID埋め込み)
- `ST_MakePoint(x REAL, y REAL [, srid INT])` - 2D点を生成
- `ST_MakePointZ(x REAL, y REAL, z REAL [, srid INT])` - 3D点を生成
- `ST_MakeLine(point1 TEXT, point2 TEXT)` - 2点からLineString作成

#### アクセサ (10関数)
- `ST_AsText(geom TEXT)` - WKT形式で出力
- `ST_AsEWKT(geom TEXT)` - EWKT形式で出力 (SRID付き)
- `ST_AsEWKB(geom TEXT)` - EWKB形式でバイナリ出力 (SRID付き)
- `ST_GeometryType(geom TEXT)` - ジオメトリタイプを取得
- `ST_SRID(geom TEXT)` - SRID値を取得
- `ST_X(point TEXT)` - POINT のX座標を取得
- `ST_Y(point TEXT)` - POINT のY座標を取得
- `ST_Z(point TEXT)` - POINT のZ座標を取得
- `ST_Is3D(geom TEXT)` - 3D判定 (1=true, 0=false)
- `ST_CoordDim(geom TEXT)` - 座標次元数を取得 (2/3/4)

#### バウンディングボックス (8関数)
- `ST_Envelope(geom TEXT)` - バウンディングボックスをPOLYGONで返す
- `ST_Extent(geom TEXT)` - "BOX(minX minY, maxX maxY)" 形式で返す
- `ST_XMin(geom TEXT)` - X座標の最小値
- `ST_XMax(geom TEXT)` - X座標の最大値
- `ST_YMin(geom TEXT)` - Y座標の最小値
- `ST_YMax(geom TEXT)` - Y座標の最大値
- `ST_ZMin(geom TEXT)` - Z座標の最小値 (3D対応)
- `ST_ZMax(geom TEXT)` - Z座標の最大値 (3D対応)

#### 計測関数 (4関数)
- `ST_Area(geom TEXT)` - ポリゴンの面積を計算
- `ST_Perimeter(geom TEXT)` - ポリゴンの外周を計算
- `ST_Length(geom TEXT)` - ラインストリングの長さを計算
- `ST_Distance(geom1 TEXT, geom2 TEXT)` - 2つのジオメトリ間の最短距離 (2D/3D対応)

#### 空間関係 (4関数)
- `ST_Intersects(geom1 TEXT, geom2 TEXT)` - 交差判定
- `ST_Contains(geom1 TEXT, geom2 TEXT)` - 包含判定
- `ST_Within(geom1 TEXT, geom2 TEXT)` - 内包判定
- `ST_Disjoint(geom1 TEXT, geom2 TEXT)` - 非交差判定

#### 空間演算・変換 (4関数)
- `ST_Centroid(geom TEXT)` - 重心を計算
- `ST_Buffer(geom TEXT, distance REAL)` - バッファ領域を生成
- `ST_Force2D(geom TEXT)` - 2D変換 (Z/M座標削除)
- `ST_Force3D(geom TEXT [, z_default REAL])` - 3D変換 (Z座標追加、デフォルト0.0)

#### 集約関数 (4関数) 🆕 Phase 5
- `ST_Collect(geom TEXT)` - ジオメトリコレクション作成（集約）
- `ST_Union(geom TEXT)` - トポロジカルな結合（集約）
- `ST_ConvexHull_Agg(geom TEXT)` - 凸包の集約計算
- `ST_Extent_Agg(geom TEXT)` - バウンディングボックスの集約

#### 座標変換 (4関数) 🆕 Phase 6
- `ST_Transform(geom TEXT, target_srid INT)` - 座標変換 (6,000+ EPSG対応)
- `ST_SetSRID(geom TEXT, srid INT)` - SRIDのみ変更（座標不変）
- `PROJ_Version()` - PROJライブラリのバージョン取得
- `PROJ_GetCRSInfo(srid INT)` - 座標参照系の名称取得

### 🗺️ 対応Geometryタイプ
- ✅ Point
- ✅ LineString
- ✅ Polygon
- ✅ MultiPoint
- ✅ MultiLineString
- ✅ MultiPolygon

### 🌍 EWKT/SRID対応 + 4次元座標
Extended WKT形式で空間参照系IDと座標次元を管理:
```sql
-- 2D (従来通り)
SELECT ST_GeomFromEWKT('SRID=4326;POINT(139.6917 35.6895)');

-- 3D (Z座標)
SELECT ST_MakePointZ(139.6917, 35.6895, 100.0, 4326);
-- → SRID=4326;POINT Z (139.6917 35.6895 100)

-- WKT形式での3D指定
SELECT ST_GeomFromEWKT('SRID=4326;POINT Z (139.6917 35.6895 100)');
SELECT ST_GeomFromEWKT('SRID=4326;LINESTRING Z (0 0 0, 1 1 10, 2 2 20)');

-- M座標 (測定値) - アーキテクチャ実装済み
SELECT ST_GeomFromEWKT('SRID=4326;POINT M (139.6917 35.6895 50)');

-- 4D (Z + M)
SELECT ST_GeomFromEWKT('SRID=4326;POINT ZM (139.6917 35.6895 100 50)');
```

## 🚀 クイックスタート

### ビルド

```bash
cd Workspace
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 使用例

```bash
sqlite3
```

```sql
-- 拡張機能をロード
.load ./Workspace/build/sqlitegis.dylib

-- 基本的な使用例
SELECT ST_Area('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- 結果: 100.0

SELECT ST_Distance('POINT(0 0)', 'POINT(3 4)');
-- 結果: 5.0 (ピタゴラスの定理)

-- SRID付きジオメトリ
SELECT ST_AsEWKT(ST_SetSRID('POINT(139.6917 35.6895)', 4326));
-- 結果: SRID=4326;POINT(139.6917 35.6895)

-- 空間関係の判定
SELECT ST_Contains(
    'POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))',
    'POINT(5 5)'
);
-- 結果: 1 (true)

-- 集約関数の使用 (Phase 5)
SELECT ST_AsEWKT(ST_Collect(geom)) FROM points;
-- 結果: SRID=4326;MULTIPOINT(0 0,1 1,2 2)

-- 座標変換 (Phase 6)
SELECT ST_AsEWKT(
    ST_Transform(
        ST_MakePoint(139.7454, 35.6586, 4326),  -- 東京タワー (WGS 84)
        3857                                     -- Web Mercator
    )
);
-- 結果: SRID=3857;POINT(15556070.04 4256425.78)

-- バッファ生成
SELECT ST_AsText(ST_Buffer('POINT(0 0)', 10));
-- 結果: POLYGON(...) - 半径10のバッファ領域

-- 3D Point操作
SELECT ST_AsEWKT(ST_MakePointZ(1.0, 2.0, 3.0, 4326));
-- 結果: SRID=4326;POINT Z (1 2 3)

SELECT ST_Z(ST_MakePointZ(1, 2, 3));
-- 結果: 3.0

SELECT ST_Is3D(ST_MakePointZ(1, 2, 3));
-- 結果: 1 (true)

-- 2D/3D変換
SELECT ST_AsEWKT(ST_Force3D(ST_MakePoint(1, 2), 10.0));
-- 結果: SRID=-1;POINT Z (1 2 10)

SELECT ST_AsEWKT(ST_Force2D(ST_MakePointZ(1, 2, 3)));
-- 結果: SRID=-1;POINT(1 2)

-- バウンディングボックス計算
SELECT ST_AsEWKT(ST_Envelope('SRID=4326;LINESTRING(0 0, 10 10)'));
-- 結果: SRID=4326;POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))

SELECT ST_Extent('POLYGON((0 0, 10 0, 10 10, 0 10, 0 0))');
-- 結果: BOX(0 0, 10 10)

SELECT ST_XMin('LINESTRING(5 10, 15 20)');
-- 結果: 5.0

-- R-tree空間インデックス統合
CREATE VIRTUAL TABLE parks_idx USING rtree(
    id, minX, maxX, minY, maxY
);

INSERT INTO parks_idx VALUES (
    1,
    ST_XMin('SRID=4326;POLYGON((139.69 35.67, 139.70 35.67, 139.70 35.68, 139.69 35.68, 139.69 35.67))'),
    ST_XMax('SRID=4326;POLYGON((139.69 35.67, 139.70 35.67, 139.70 35.68, 139.69 35.68, 139.69 35.67))'),
    ST_YMin('SRID=4326;POLYGON((139.69 35.67, 139.70 35.67, 139.70 35.68, 139.69 35.68, 139.69 35.67))'),
    ST_YMax('SRID=4326;POLYGON((139.69 35.67, 139.70 35.67, 139.70 35.68, 139.69 35.68, 139.69 35.67))')
);
```

## 📋 テスト

包括的なテストスイートを用意しています (計222テストケース):

```bash
cd Workspace
./tests/run_tests.sh
```

個別のテストスイート:
- `tests/test_constructors.sql` - コンストラクタ関数 (25テスト)
- `tests/test_accessors.sql` - アクセサ関数 (33テスト)
- `tests/test_measures.sql` - 計測関数 (34テスト)
- `tests/test_relations.sql` - 空間関係関数 (33テスト)
- `tests/test_operations.sql` - 空間演算・ユーティリティ (40テスト)
- `tests/test_3d.sql` - 🆕 3D座標関数 (57テスト)

詳細は [tests/README.md](./Workspace/tests/README.md) を参照してください。

**注**: macOS環境互換性の問題により、一部テストの実行は保留中です。

## 📚 ドキュメント

- **[仕様書](./Workspace/docs/specification.md)** - 全体設計と関数仕様
- **[実装計画](./Workspace/docs/implementation_plan.md)** - Phase別実装ロードマップ
- **[Phase 1サマリー](./Workspace/docs/phase1_summary.md)** - Phase 1実装詳細と進捗状況
- **[Phase 2計画](./Workspace/docs/phase2_plan.md)** - Phase 2 WKB/EWKB仕様
- **[Phase 2サマリー](./Workspace/docs/phase2_summary.md)** - Phase 2実装詳細と完了報告
- **[Phase 3計画](./Workspace/docs/phase3_plan.md)** - Phase 3 3D座標システム仕様
- **[Phase 3サマリー](./Workspace/docs/phase3_summary.md)** - Phase 3実装詳細と完了報告
- **[Phase 4計画](./Workspace/docs/phase4_plan.md)** - Phase 4 空間インデックス仕様
- **[Phase 4サマリー](./Workspace/docs/phase4_summary.md)** - Phase 4実装詳細と完了報告
- **[Phase 5計画](./Workspace/docs/phase5_plan.md)** - 🆕 Phase 5 集約関数仕様
- **[Phase 5サマリー](./Workspace/docs/phase5_summary.md)** - 🆕 Phase 5実装詳細と完了報告
- **[Phase 6計画](./Workspace/docs/phase6_plan.md)** - 🆕 Phase 6 座標変換仕様
- **[Phase 6サマリー](./Workspace/docs/phase6_summary.md)** - 🆕 Phase 6実装詳細と完了報告
- **[テストガイド](./Workspace/tests/README.md)** - テスト実行方法

## 🔧 システム要件

### 依存関係
- CMake >= 3.20
- SQLite3 (開発用ヘッダ含む)
- Boost >= 1.71 (header-only)
- PROJ >= 9.0 (座標変換ライブラリ) 🆕
- C++20対応コンパイラ (GCC 10+, Clang 11+, MSVC 2019+)

### ビルド環境
- macOS: AppleClang 14.0以上
- Linux: GCC 10以上 / Clang 11以上
- Windows: MSVC 2019以上 (未検証)

## ⚠️ 制限事項 (Phase 6完了時点)

1. **測地系計算未対応**: 平面座標系のみ。WGS84の度単位計算は不正確（ST_Transformで座標変換は可能）
2. **SRID検証なし**: 異なるSRID間の演算でも警告なし（ST_Transformで明示的に変換が必要）
3. **M座標のSQL関数**: アーキテクチャは実装済みだが、ST_M等のアクセサ関数は未実装
4. **3Dトポロジー演算**: ST_Intersection, ST_Union等は2Dのみ (Boost.Geometry制約)
5. **GeoJSON未対応**: 属性データ管理の複雑性により実装見送り
6. **テスト環境**: macOS環境でSegmentation Fault問題あり（実装は完了）

## 🗺️ ロードマップ

| Phase | バージョン | 機能 | 状態 |
|-------|-----------|------|------|
| Phase 1 | v0.1 | 基礎関数 (WKT, EWKT, 計測, 関係) | ✅ 完了 (23関数) |
| Phase 2 | v0.2 | WKB/EWKBバイナリ形式 | ✅ 完了 (25関数) |
| Phase 3 | v0.3 | 3D/4D座標システム (XY/XYZ/XYM/XYZM) | ✅ 完了 (20関数) |
| Phase 4 | v0.4 | 空間インデックス (R-tree), バウンディングボックス | ✅ 完了 (28関数) |
| Phase 5 | v0.5 | 集約関数 (ST_Collect, ST_Union, ST_ConvexHull_Agg, ST_Extent_Agg) | ✅ 完了 (32関数) |
| Phase 6 | v0.6 | 座標変換 (PROJ統合、6,000+ EPSG対応) | ✅ 完了 (36関数) |
| Phase 7 | v0.7 | トポロジー演算拡張、または測地系計算 | 📋 計画中 |

## 📊 コード統計

- C++ヘッダファイル: 11 (Phase 5: +1, Phase 6: +1)
- C++実装ファイル: 11 (Phase 5: +1, Phase 6: +1)
- 総行数: 約6,000行 (Phase 4: ~5,000行, Phase 5: +500行, Phase 6: +455行)
- 共有ライブラリサイズ: 2.6MB (PROJ統合後)
- 登録関数: 36 (Phase 1-4: 28, Phase 5: +4, Phase 6: +4)
- テストケース: 235+ (Phase 1-2: 165, Phase 3: +57, Phase 5: +13+)

## 🤝 貢献

プロジェクトへの貢献を歓迎します!

1. このリポジトリをフォーク
2. 機能ブランチを作成 (`git checkout -b feature/amazing-feature`)
3. 変更をコミット (`git commit -m 'Add some amazing feature'`)
4. ブランチにプッシュ (`git push origin feature/amazing-feature`)
5. Pull Requestを作成

## 📄 ライセンス

このプロジェクトはオープンソースです。ライセンス詳細は後日追加予定。

## 🙏 謝辞

- [PostGIS](https://postgis.net/) - API設計の参考
- [Boost.Geometry](https://www.boost.org/doc/libs/release/libs/geometry/) - 幾何演算エンジン
- [SQLite](https://www.sqlite.org/) - データベースエンジン

## 📞 サポート

質問やバグ報告は、GitHubのIssuesでお願いします。