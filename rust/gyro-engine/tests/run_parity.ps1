$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$crate = Join-Path $root 'rust\gyro-engine'
$output = Join-Path $crate 'target\parity'
$vcvars = 'D:\vs2022\Community\VC\Auxiliary\Build\vcvars64.bat'

New-Item -ItemType Directory -Path $output -Force | Out-Null
$cppSource = Join-Path $PSScriptRoot 'cpp_reference.cpp'
$cppExe = Join-Path $output 'cpp_reference.exe'
cmd /d /s /c "`"$vcvars`" >nul && cl /nologo /EHsc /std:c++17 /O2 /Fe:`"$cppExe`" `"$cppSource`""
if ($LASTEXITCODE -ne 0) { throw 'C++ reference build failed' }

$frames = Join-Path $PSScriptRoot 'gyro_frames.csv'
$cppCsv = Join-Path $output 'cpp.csv'
$rustCsv = Join-Path $output 'rust.csv'
& $cppExe $frames | Set-Content $cppCsv
cargo run --quiet --manifest-path (Join-Path $crate 'Cargo.toml') --bin gyro-parity -- $frames | Set-Content $rustCsv
if ($LASTEXITCODE -ne 0) { throw 'Rust parity runner failed' }

$cpp = Import-Csv $cppCsv
$rust = Import-Csv $rustCsv
if ($cpp.Count -ne $rust.Count) { throw "Frame count differs: C++=$($cpp.Count), Rust=$($rust.Count)" }
$maxVelocityError = 0.0
$maxDeltaError = 0.0
$pixelMismatches = 0
for ($index = 0; $index -lt $cpp.Count; $index++) {
    $maxVelocityError = [math]::Max($maxVelocityError, [math]::Abs([double]$cpp[$index].velocity_x - [double]$rust[$index].velocity_x))
    $maxVelocityError = [math]::Max($maxVelocityError, [math]::Abs([double]$cpp[$index].velocity_y - [double]$rust[$index].velocity_y))
    $maxDeltaError = [math]::Max($maxDeltaError, [math]::Abs([double]$cpp[$index].delta_x - [double]$rust[$index].delta_x))
    $maxDeltaError = [math]::Max($maxDeltaError, [math]::Abs([double]$cpp[$index].delta_y - [double]$rust[$index].delta_y))
    if ($cpp[$index].pixel_x -ne $rust[$index].pixel_x -or $cpp[$index].pixel_y -ne $rust[$index].pixel_y) { $pixelMismatches++ }
}

[pscustomobject]@{
    Frames = $cpp.Count
    MaxVelocityError = $maxVelocityError
    MaxDeltaError = $maxDeltaError
    PixelMismatches = $pixelMismatches
}
if ($maxVelocityError -gt 0.00001 -or $maxDeltaError -gt 0.00001 -or $pixelMismatches -ne 0) { exit 1 }
