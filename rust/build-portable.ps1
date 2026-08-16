param(
    [string]$VcVarsPath = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$cppBuild = Join-Path $repoRoot "out\build\embedded-core-ninja"
$tauriRoot = Join-Path $repoRoot "rust\tauri-app"
$portableRoot = Join-Path $repoRoot "out\portable"

if (-not $VcVarsPath) {
    $candidates = @(
        "D:\vs2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
    )
    $VcVarsPath = $candidates | Where-Object { $_ -and (Test-Path -LiteralPath $_) } | Select-Object -First 1
}

if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue) -and (-not $VcVarsPath -or -not (Test-Path -LiteralPath $VcVarsPath))) {
    throw "未找到 Visual Studio 2022 C++ x64 工具链。请使用 -VcVarsPath 指定 vcvars64.bat。"
}

$nativeCommands = "cmake -S `"$repoRoot`" -B `"$cppBuild`" -G Ninja -DSDL=ON -DCMAKE_BUILD_TYPE=Release && cmake --build `"$cppBuild`" --target jsm_embedded_core -j 6"
# Always load the VS dev environment when a vcvars64.bat was found: cl.exe may
# already be on the ambient PATH (so the "Get-Command cl.exe" check alone would
# skip vcvars), but rc.exe/mt.exe from the Windows SDK only appear after
# vcvars is sourced, and cmake's compiler test needs them.
if ($VcVarsPath -and (Test-Path -LiteralPath $VcVarsPath)) {
    & cmd.exe /d /c "call `"$VcVarsPath`" && $nativeCommands"
} else {
    & cmd.exe /d /c $nativeCommands
}
if ($LASTEXITCODE -ne 0) { throw "C++ 核心编译失败（退出码 $LASTEXITCODE）。" }

Push-Location $tauriRoot
try {
    npm ci
    if ($LASTEXITCODE -ne 0) { throw "前端依赖安装失败（退出码 $LASTEXITCODE）。" }
    npm run tauri -- build --no-bundle
    if ($LASTEXITCODE -ne 0) { throw "Tauri 应用编译失败（退出码 $LASTEXITCODE）。" }
}
finally {
    Pop-Location
}

New-Item -ItemType Directory -Force -Path $portableRoot | Out-Null
$sourceExe = Join-Path $tauriRoot "src-tauri\target\release\joyshockmapper-tauri.exe"
$portableExe = Join-Path $portableRoot "JoyShockMapper.exe"
$rootExe = Join-Path $repoRoot "JoyShockMapper.exe"
$restartInBackground = $false
if (Test-Path -LiteralPath $rootExe) {
    $rootFullPath = (Resolve-Path -LiteralPath $rootExe).Path
    $runningRoot = Get-Process -Name "JoyShockMapper" -ErrorAction SilentlyContinue | Where-Object {
        try { $_.Path -eq $rootFullPath } catch { $false }
    }
    if ($runningRoot) {
        $restartInBackground = $true
        $runningRoot | Stop-Process -Force
        Start-Sleep -Milliseconds 400
    }
}
Copy-Item -LiteralPath $sourceExe -Destination $portableExe -Force
Copy-Item -LiteralPath $sourceExe -Destination $rootExe -Force
if ($restartInBackground) {
    Start-Process -FilePath $rootExe -ArgumentList "--background" -WindowStyle Hidden
}
Write-Host "根目录成品已更新：$rootExe"
Write-Host "发行版副本已生成：$portableExe"
