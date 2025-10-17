# SqliteGIS GDAL Driver

このディレクトリには、GDALからSqliteGISファイル(.sqlitegis)を読み書きするためのOGRドライバが含まれています。

## 概要

SqliteGIS GDAL Driverは、QGISやその他のGDAL対応アプリケーションでSqliteGISデータベースを使用できるようにします。

- テーブルを1レイヤーとして取り扱う
- 拡張子は `.sqlitegis` とする

### 主な機能

- `.sqlitegis`拡張子のSQLiteデータベースファイルの読み取り・書き込み
- 各テーブルを個別のレイヤーとして認識
- EWKB形式のジオメトリの自動パース
- SRID(座標参照系)のサポート
- 空間インデックス対応

## アーキテクチャ

ドライバは以下の3つの主要クラスで構成されています:

### 1. OGRSqliteGISDriver
- ドライバの登録と識別
- ファイル拡張子(.sqlitegis)によるデータソースの判定
- データソースのオープンと作成

### 2. OGRSqliteGISDataSource
- SQLiteデータベースへの接続管理
- SqliteGIS拡張機能のロード
- テーブルからレイヤーの自動検出
- レイヤーの作成と管理

### 3. OGRSqliteGISLayer
- テーブルデータの読み取り
- EWKB形式のジオメトリパース
- フィーチャの作成・更新・削除
- 空間フィルタと属性フィルタのサポート

## 現在の状況

**注意**: このドライバは現在開発中です。GDAL全体をビルドする必要があるため、現在はヘッダーファイルと実装のみが含まれています。

### 実装済みのファイル

- `include/ogrsqlitegisdriver.h` - ドライバクラスのヘッダー
- `include/ogrsqlitegisgdatasource.h` - データソースクラスのヘッダー
- `include/ogrsqlitegisgislayer.h` - レイヤークラスのヘッダー
- `src/ogrsqlitegisdriver.cpp` - ドライバの実装
- `src/ogrsqlitegisgdatasource.cpp` - データソースの実装
- `src/ogrsqlitegisgislayer.cpp` - レイヤーの実装
- `CMakeLists.txt` - ビルド設定

## ビルド方法(将来の手順)

### 前提条件

1. **GDAL 3.7.x以上**がシステムにインストールされていること
2. **SQLite3**ライブラリ
3. **CMake 3.20以上**
4. **Visual Studio 2022**(Windows)または**GCC/Clang**(Linux/macOS)
5. **SqliteGIS拡張**がビルド済みであること

### Windows (Visual Studio 2022)

```powershell
# ビルドディレクトリを作成
cd Driver
mkdir build
cd build

# CMakeで設定(GDAL_DIR環境変数を設定する必要があります)
$env:GDAL_DIR="C:\Path\To\GDAL"
cmake .. -G "Visual Studio 17 2022" -A x64

# ビルド
cmake --build . --config Release
```

### Linux / macOS

```bash
# ビルドディレクトリを作成
cd Driver
mkdir build
cd build

# CMakeで設定
cmake .. -DCMAKE_BUILD_TYPE=Release

# ビルド
make

# インストール(オプション)
sudo make install
```

## 使用方法(将来)

### gdalinfoでデータベース情報を表示

```bash
gdalinfo test.sqlitegis
```

### ogrinfo でレイヤー情報を表示

```bash
ogrinfo test.sqlitegis
```

### ogr2ogrでデータ変換

ShapefileからSqliteGISへ:
```bash
ogr2ogr -f SqliteGIS output.sqlitegis input.shp
```

SqliteGISからGeoJSONへ:
```bash
ogr2ogr -f GeoJSON output.geojson input.sqlitegis
```

### QGISでの使用

1. QGISを起動
2. **レイヤー** → **レイヤを追加** → **ベクタレイヤを追加**
3. **ソースタイプ**: ファイル
4. `.sqlitegis`ファイルを選択

## EWKBフォーマット

SqliteGISはPostGIS互換のEWKB(Extended Well-Known Binary)形式でジオメトリを格納します:

```
[Byte Order][Type+Flags][SRID?][WKB Data]

Flags:
- 0x20000000: SRID_FLAG (SRID付き)
- 0x80000000: WKB_Z_FLAG (Z座標付き)
- 0x40000000: WKB_M_FLAG (M値付き)
```

## 開発状況

### 実装済み機能
- ✅ ドライバ登録とファイル識別
- ✅ データソースのオープン/作成
- ✅ レイヤーの自動検出
- ✅ ジオメトリの読み取り(EWKB→OGRGeometry)
- ✅ フィーチャの作成(ICreateFeature)
- ✅ スキーマ定義の読み取り
- ✅ 高速カウント(GetFeatureCount)

### 今後の実装予定
- ⏳ ビルド環境の構築(GDALライブラリの依存関係解決)
- ⏳ フィーチャの更新(ISetFeature)
- ⏳ フィーチャの削除(DeleteFeature)
- ⏳ 空間インデックスの活用
- ⏳ トランザクションサポート
- ⏳ SQL実行(ExecuteSQL)

## 参考資料

- [GDAL Vector Driver Tutorial](https://gdal.org/development/dev_documentation.html)
- [OGR Architecture](https://gdal.org/development/rfc/rfc10_ograrchitecture.html)
- [SqliteGIS Documentation](../Workspace/docs/)
