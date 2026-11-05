[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$QtDir,
    [Parameter(Mandatory = $true)][string]$MingwDir,
    [string]$OutputDir = 'release',
    [ValidateRange(1, 64)][int]$Jobs = 4
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$projectRoot = Split-Path -Parent $PSScriptRoot
$QtDir = (Resolve-Path -LiteralPath $QtDir).Path
$MingwDir = (Resolve-Path -LiteralPath $MingwDir).Path
$cmake = (Get-Command cmake -ErrorAction Stop).Source
$ninja = (Get-Command ninja -ErrorAction Stop).Source
$compiler = Join-Path $MingwDir 'bin/g++.exe'
$deploy = Join-Path $QtDir 'bin/windeployqt.exe'
foreach ($required in @($compiler, $deploy)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing build tool: $required"
    }
}

$cmakeText = Get-Content -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt') -Raw
$version = [regex]::Match($cmakeText, 'project\(qMplayer VERSION ([0-9.]+)').Groups[1].Value
if (-not $version) { throw 'qMplayer project version was not found.' }
$packageName = "qMplayer-v$version-windows-x64"
if (-not [IO.Path]::IsPathRooted($OutputDir)) { $OutputDir = Join-Path $projectRoot $OutputDir }
$OutputDir = [IO.Path]::GetFullPath($OutputDir)
$packageDir = Join-Path $OutputDir $packageName
$archive = "$packageDir.zip"
foreach ($target in @($packageDir, $archive, "$archive.sha256")) {
    if (Test-Path -LiteralPath $target) { throw "Output already exists; choose a new -OutputDir: $target" }
}
$buildDir = Join-Path $projectRoot 'build/release'
$oldPath = $env:Path
try {
    $env:Path = "$QtDir\bin;$MingwDir\bin;$oldPath"
    & $cmake -S $projectRoot -B $buildDir -G Ninja "-DCMAKE_MAKE_PROGRAM=$ninja" `
        -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$QtDir" "-DCMAKE_CXX_COMPILER=$compiler"
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed: $LASTEXITCODE" }
    & $cmake --build $buildDir --parallel $Jobs
    if ($LASTEXITCODE -ne 0) { throw "CMake build failed: $LASTEXITCODE" }
    & $cmake --install $buildDir --prefix $packageDir
    if ($LASTEXITCODE -ne 0) { throw "CMake install failed: $LASTEXITCODE" }
    & $deploy --release --compiler-runtime --no-translations --dir $packageDir (Join-Path $packageDir 'qMplayer.exe')
    if ($LASTEXITCODE -ne 0) { throw "windeployqt failed: $LASTEXITCODE" }

    "[Paths]`nPrefix=.`nPlugins=.`n" | Set-Content -LiteralPath (Join-Path $packageDir 'qt.conf') -Encoding ASCII
    $licensesDir = Join-Path $packageDir 'licenses'
    New-Item -ItemType Directory -Path $licensesDir -Force | Out-Null
    $qtInstallRoot = Split-Path -Parent (Split-Path -Parent $QtDir)
    $qtLicenses = Join-Path $qtInstallRoot 'Licenses'
    if (Test-Path -LiteralPath $qtLicenses) {
        Copy-Item -LiteralPath $qtLicenses -Destination (Join-Path $licensesDir 'Qt') -Recurse
    }
    foreach ($subdir in @('licenses', 'share/licenses')) {
        $source = Join-Path $MingwDir $subdir
        if (Test-Path -LiteralPath $source) {
            Copy-Item -LiteralPath $source -Destination (Join-Path $licensesDir ('MinGW-' + ($subdir -replace '/', '-'))) -Recurse
        }
    }
    $sbomDir = Join-Path $QtDir 'sbom'
    if (Test-Path -LiteralPath $sbomDir) {
        $sbomTarget = Join-Path $licensesDir 'sbom'
        New-Item -ItemType Directory -Path $sbomTarget -Force | Out-Null
        Get-ChildItem -LiteralPath $sbomDir -Filter '*.spdx.json' | Where-Object {
            $_.Name -match '^(qtbase|qtmultimedia|qtsvg)-'
        } | Copy-Item -Destination $sbomTarget
    }
    Compress-Archive -LiteralPath $packageDir -DestinationPath $archive -CompressionLevel Optimal
    $digest = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
    "$digest  $packageName.zip" | Set-Content -LiteralPath "$archive.sha256" -Encoding ASCII
    Write-Output "PACKAGE: $archive"
    Write-Output "SHA256: $digest"
}
finally {
    $env:Path = $oldPath
}
