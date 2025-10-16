# ============================================================================
# SqliteGIS 簡易テストランナー (主要機能のみ)
# ============================================================================

$ErrorActionPreference = "Continue"

# 設定
$WorkspaceDir = "D:\Work\SqliteGIS\Workspace"
$BuildDir = Join-Path $WorkspaceDir "build\Release"
$TestsDir = Join-Path $WorkspaceDir "tests"
$ExtensionDll = Join-Path $BuildDir "sqlitegis.dll"
$SqliteExe = "D:\vcpkg\installed\x64-windows\tools\sqlite3.exe"

# vcpkg binディレクトリをPATHに追加
$env:PATH = "D:\vcpkg\installed\x64-windows\bin;$env:PATH"

# 拡張機能の存在確認
if (-not (Test-Path $ExtensionDll)) {
    Write-Host "Error: 拡張機能が見つかりません: $ExtensionDll" -ForegroundColor Red
    exit 1
}

Write-Host "====================================" -ForegroundColor Blue
Write-Host "SqliteGIS Core Tests - Windows" -ForegroundColor Blue
Write-Host "====================================" -ForegroundColor Blue
Write-Host ""

# 主要な4つのテストのみ実行
$TestFiles = @(
    "test_constructors.sql",
    "test_accessors.sql",
    "test_measures.sql",
    "test_relations.sql"
)

foreach ($TestFile in $TestFiles) {
    $TestPath = Join-Path $TestsDir $TestFile
    
    if (-not (Test-Path $TestPath)) {
        Write-Host "Warning: $TestFile が見つかりません" -ForegroundColor Yellow
        continue
    }
    
    Write-Host "`n>>> Running: $TestFile <<<" -ForegroundColor Green
    Write-Host "------------------------------------" -ForegroundColor Gray
    
    # テストファイルの内容を読み込んでDLLパスを置換
    $TestContent = Get-Content $TestPath -Raw -Encoding UTF8
    $TestContent = $TestContent -replace "\.load build/sqlitegis\.dylib", ".load $($ExtensionDll -replace '\\', '/')"
    
    # 一時ファイルに書き込み
    $TempFile = [System.IO.Path]::GetTempFileName()
    [System.IO.File]::WriteAllText($TempFile, $TestContent, [System.Text.Encoding]::UTF8)
    
    try {
        # テスト実行
        $Output = Get-Content $TempFile -Encoding UTF8 | & $SqliteExe ":memory:" 2>&1
        
        # 出力を表示（エラーのみ赤で表示）
        $ErrorCount = 0
        $Output | ForEach-Object {
            $line = $_.ToString()
            if ($line -match "^Error:") {
                Write-Host $line -ForegroundColor Red
                $ErrorCount++
            } else {
                Write-Host $line
            }
        }
        
        if ($ErrorCount -gt 0) {
            Write-Host "`n⚠ $ErrorCount errors found" -ForegroundColor Yellow
        } else {
            Write-Host "`n✓ Test completed" -ForegroundColor Green
        }
        
    } catch {
        Write-Host "Exception: $_" -ForegroundColor Red
    } finally {
        Remove-Item $TempFile -ErrorAction SilentlyContinue
    }
}

Write-Host "`n====================================" -ForegroundColor Blue
Write-Host "Core Tests Completed" -ForegroundColor Blue
Write-Host "====================================" -ForegroundColor Blue
