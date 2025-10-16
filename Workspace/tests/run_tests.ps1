# ============================================================================
# SqliteGIS Test Runner (Windows PowerShell)
# ============================================================================
# 
# このスクリプトはSqliteGIS拡張機能の全テストスイートを実行します。
# 使用方法: .\tests\run_tests.ps1
#
# 必要条件:
# - SQLite3がインストールされていること (vcpkg)
# - sqlitegis.dllがbuild/Release/にビルドされていること
# ============================================================================

# エラー時に停止
$ErrorActionPreference = "Continue"

# 設定
$WorkspaceDir = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $WorkspaceDir "build\Release"
$TestsDir = Join-Path $WorkspaceDir "tests"
$ExtensionDll = Join-Path $BuildDir "sqlitegis.dll"
$SqliteExe = "D:\vcpkg\installed\x64-windows\tools\sqlite3.exe"

# vcpkg binディレクトリをPATHに追加（依存DLL用）
$env:PATH = "D:\vcpkg\installed\x64-windows\bin;$env:PATH"

# 拡張機能の存在確認
if (-not (Test-Path $ExtensionDll)) {
    Write-Host "Error: 拡張機能が見つかりません: $ExtensionDll" -ForegroundColor Red
    Write-Host "ビルドを実行してください: cmake --build build --config Release" -ForegroundColor Yellow
    exit 1
}

# SQLite3の存在確認
if (-not (Test-Path $SqliteExe)) {
    Write-Host "Error: SQLite3が見つかりません: $SqliteExe" -ForegroundColor Red
    exit 1
}

Write-Host "====================================" -ForegroundColor Blue
Write-Host "SqliteGIS Test Suite - Windows" -ForegroundColor Blue
Write-Host "====================================" -ForegroundColor Blue
Write-Host ""
Write-Host "Extension: $ExtensionDll" -ForegroundColor Cyan
Write-Host "SQLite3:   $SqliteExe" -ForegroundColor Cyan
Write-Host ""

# テストファイルリスト
# 注: test_wkb.sql は除外（WKB削除済み、EWKB統合）
$TestFiles = @(
    "test_constructors.sql",
    "test_accessors.sql",
    "test_measures.sql",
    "test_relations.sql",
    "test_operations.sql",
    # "test_wkb.sql",      # WKBは削除済み（EWKB統合）
    "test_3d.sql",
    "test_aggregates.sql"
)

$TotalTests = 0
$PassedTests = 0
$FailedTests = 0

# 各テストファイルを実行
foreach ($TestFile in $TestFiles) {
    $TestPath = Join-Path $TestsDir $TestFile
    
    if (-not (Test-Path $TestPath)) {
        Write-Host "Warning: テストファイルが見つかりません: $TestFile" -ForegroundColor Yellow
        continue
    }
    
    Write-Host "Running: $TestFile" -ForegroundColor Green
    Write-Host "------------------------------------" -ForegroundColor Gray
    
    # テストファイルの内容を読み込んでDLLパスを置換
    $TestContent = Get-Content $TestPath -Raw
    $TestContent = $TestContent -replace "\.load build/sqlitegis\.dylib", ".load $($ExtensionDll -replace '\\', '/')"
    
    # 一時ファイルに書き込み
    $TempFile = [System.IO.Path]::GetTempFileName()
    $TestContent | Out-File -FilePath $TempFile -Encoding UTF8
    
    try {
        # テスト実行
        $Output = Get-Content $TempFile | & $SqliteExe ":memory:" 2>&1
        
        # 出力を表示
        $Output | ForEach-Object {
            if ($_ -match "Error:") {
                Write-Host $_ -ForegroundColor Red
                $script:FailedTests++
            } else {
                Write-Host $_
            }
        }
        
        # テストカウント（Expectedコメント数を数える）
        $ExpectedCount = (Get-Content $TestPath | Select-String "-- Expected:").Count
        $TotalTests += $ExpectedCount
        
        # エラーがなければ成功としてカウント
        if ($LASTEXITCODE -eq 0) {
            $PassedTests += $ExpectedCount
        }
        
    } catch {
        Write-Host "エラーが発生しました: $_" -ForegroundColor Red
        $FailedTests++
    } finally {
        # 一時ファイル削除
        Remove-Item $TempFile -ErrorAction SilentlyContinue
    }
    
    Write-Host ""
}

# 結果サマリー
Write-Host "====================================" -ForegroundColor Blue
Write-Host "テスト結果サマリー" -ForegroundColor Blue
Write-Host "====================================" -ForegroundColor Blue
Write-Host "総テスト数: $TotalTests" -ForegroundColor Cyan
Write-Host "成功: $PassedTests" -ForegroundColor Green
Write-Host "失敗: $FailedTests" -ForegroundColor $(if ($FailedTests -gt 0) { "Red" } else { "Green" })
Write-Host ""

if ($FailedTests -eq 0) {
    Write-Host "✓ すべてのテストが成功しました!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "✗ いくつかのテストが失敗しました" -ForegroundColor Red
    exit 1
}
