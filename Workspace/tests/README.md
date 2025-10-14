# SqliteGIS Test Suite

このディレクトリには、SqliteGIS拡張機能の全機能をテストするSQLテストケースが含まれています。

## テストファイル

| ファイル | 内容 | テスト数 |
|---------|------|---------|
| `test_constructors.sql` | ジオメトリ生成関数 (ST_GeomFromText, ST_MakePoint等) | 25 |
| `test_accessors.sql` | アクセサ関数 (ST_AsText, ST_X, ST_Y等) | 33 |
| `test_measures.sql` | 計測関数 (ST_Area, ST_Length, ST_Distance等) | 34 |
| `test_relations.sql` | 空間関係関数 (ST_Intersects, ST_Contains等) | 33 |
| `test_operations.sql` | 空間演算・ユーティリティ (ST_Buffer, ST_Centroid等) | 40 |

**合計**: 約165個のテストケース

## テスト実行方法

### 全テストを実行

```bash
cd /Users/okimiyuki/Projects/SqliteGIS/Workspace
./tests/run_tests.sh
```

### 個別テストを実行

```bash
cd /Users/okimiyuki/Projects/SqliteGIS/Workspace
sqlite3 :memory: < tests/test_constructors.sql
```

または、SQLite3の組み込みビルドを使用:

```bash
third_party/sqlite-install/bin/sqlite3 :memory: < tests/test_constructors.sql
```

## テスト結果の見方

各テストには期待される結果がコメントとして記載されています:

```sql
SELECT ST_AsText(ST_MakePoint(10, 20));
-- Expected: POINT(10 20)
```

実行時は、SQLの実行結果と`Expected:`コメントを比較して検証してください。

## 前提条件

- SQLite3がインストールされていること
- `sqlitegis.dylib`が`build/`ディレクトリにビルドされていること
- SQLite3がloadable extensionをサポートしていること

### SQLite3のバージョン確認

```bash
sqlite3 --version
```

### 拡張機能のビルド確認

```bash
ls -lh build/sqlitegis.dylib
```

## トラブルシューティング

### エラー: "not authorized to use load_extension"

SQLite3が拡張機能の読み込みをサポートしていません。以下を試してください:

1. 組み込みのSQLite3を使用:
   ```bash
   third_party/sqlite-install/bin/sqlite3 :memory: < tests/test_constructors.sql
   ```

2. またはHomebrewでインストール:
   ```bash
   brew install sqlite3
   ```

### エラー: "extension not found"

拡張機能をビルドしてください:

```bash
cd Workspace
cmake --build build
```

## テストの追加

新しいテストを追加する場合:

1. 適切なテストファイルに追加 (または新規作成)
2. SQLコマンドと期待される結果をコメントで記述
3. `run_tests.sh`の`TEST_FILES`配列に追加 (新規ファイルの場合)

例:

```sql
.print "Test N: Description"
SELECT function_to_test(args);
-- Expected: expected_result
```

## 継続的インテグレーション (CI)

将来的には、以下を追加予定:

- [ ] GitHub Actions統合
- [ ] テスト結果の自動検証
- [ ] カバレッジレポート
- [ ] パフォーマンスベンチマーク

## 参考情報

- [Phase 1 実装サマリー](../docs/phase1_summary.md)
- [仕様書](../docs/specification.md)
- [実装計画](../docs/implementation_plan.md)
