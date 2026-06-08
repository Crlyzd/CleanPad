# build.ps1 - CleanPad standalone build script
# Usage:  .\build.ps1           -> Release build -> output\CleanPad.exe
#         .\build.ps1 -Debug    -> Debug build   -> output\CleanPad.exe
# Requirements: Visual Studio Build Tools 2019 or later (any edition)

param([switch]$Debug)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# Paths
$ScriptDir = $PSScriptRoot
$SrcDir    = Join-Path $ScriptDir "src"
$ResFile   = Join-Path $ScriptDir "CleanPad.rc"
$OutDir    = Join-Path $ScriptDir "output"
$OutExe    = Join-Path $OutDir "CleanPad.exe"
$OutRes    = Join-Path $OutDir "CleanPad.res"

# Find MSVC via vswhere
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Please install Visual Studio Build Tools."
}

$vsPath = (& $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
if (-not $vsPath) {
    throw "No Visual Studio with C++ tools found."
}

$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) {
    throw "vcvars64.bat not found at: $vcvars"
}

Write-Host "VS toolchain: $vsPath" -ForegroundColor Cyan

# Auto-discover all .cpp source files
$cppFiles = Get-ChildItem -Path $SrcDir -Recurse -Filter "*.cpp" | Select-Object -ExpandProperty FullName
if ($cppFiles.Count -eq 0) { throw "No .cpp files found in $SrcDir" }

Write-Host "Sources found: $($cppFiles.Count) file(s)" -ForegroundColor Cyan
$cppFiles | ForEach-Object { Write-Host "  $_" }

# Create output dir
if (-not (Test-Path $OutDir)) { New-Item -ItemType Directory -Path $OutDir | Out-Null }

# Compiler flags
$CppVer  = "/std:c++17"
$Defs    = "/DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00"
$Incs    = '/I"' + $ScriptDir + '"'
$Warn    = "/W3 /EHsc"
$Libs    = "user32.lib gdi32.lib comctl32.lib uxtheme.lib dwmapi.lib shell32.lib shlwapi.lib comdlg32.lib"

if ($Debug) {
    $Opt   = "/Od /Zi /MDd"
    $Link  = "/DEBUG"
    Write-Host "Mode: DEBUG" -ForegroundColor Yellow
} else {
    $Opt   = "/O2 /GL /MT"
    $Link  = "/LTCG /OPT:REF /OPT:ICF"
    Write-Host "Mode: RELEASE" -ForegroundColor Green
}

# Build command strings using concatenation to avoid PS escaping issues
$srcJoined = ($cppFiles | ForEach-Object { '"' + $_ + '"' }) -join " "
$feFlag    = '/Fe"' + $OutExe + '"'
$foFlag    = '/Fo"' + $OutDir + '\\"'
$resArg    = '"' + $OutRes + '"'

$rcCmd = 'rc.exe /nologo /fo "' + $OutRes + '" /I "' + $ScriptDir + '" "' + $ResFile + '"'
$clCmd = "cl.exe /nologo $CppVer $Defs $Incs $Warn $Opt $feFlag $foFlag $srcJoined $resArg /link $Link /SUBSYSTEM:WINDOWS $Libs"

# Helper: run a command inside the VS environment via a temp .bat file
function Invoke-VsCmd($cmd) {
    $bat = Join-Path $env:TEMP "cleanpad_build_$([System.Guid]::NewGuid().ToString('N')).bat"
    $content = "@echo off`r`ncall `"$vcvars`" > nul 2>&1`r`n$cmd`r`nexit /b %ERRORLEVEL%"
    [System.IO.File]::WriteAllText($bat, $content, [System.Text.Encoding]::ASCII)
    $proc = Start-Process -FilePath "cmd.exe" -ArgumentList "/c", "`"$bat`"" -Wait -PassThru -NoNewWindow
    Remove-Item $bat -ErrorAction SilentlyContinue
    return $proc.ExitCode
}

Write-Host "`nStep 1: Compiling resources..." -ForegroundColor Cyan
$code = Invoke-VsCmd $rcCmd
if ($code -ne 0) { throw "Resource compilation failed (exit $code)" }

Write-Host "Step 2: Compiling and linking..." -ForegroundColor Cyan
$code = Invoke-VsCmd $clCmd
if ($code -ne 0) { throw "Compilation failed (exit $code)" }

Write-Host ""
Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Output: $OutExe" -ForegroundColor Green
