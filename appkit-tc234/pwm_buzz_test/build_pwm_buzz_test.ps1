# Build pwm_buzz_test for TC234 (TC23A) with the AURIX GCC toolchain, standalone.
# Produces pwm_buzz_test.hex for flashing via AURIXFlasher.

$ErrorActionPreference = 'Continue'

$proj = Split-Path -Parent $MyInvocation.MyCommand.Path
# Libraries are shared at the board root (appkit-tc234/Libraries).
$libs = Join-Path (Split-Path -Parent $proj) 'Libraries'
$buildDir = Join-Path $env:TEMP 'pwm_buzz_test_build'
$gcc = 'D:\aurixgcc_03_2026\bin\tricore-elf-gcc.exe'
$objcopy = 'D:\aurixgcc_03_2026\bin\tricore-elf-objcopy.exe'

if (Test-Path $buildDir) { Remove-Item $buildDir -Recurse -Force }
New-Item -ItemType Directory -Path $buildDir | Out-Null

# --- Gather include paths from the .cproject (all relative) ---
$cproj = Get-Content (Join-Path $proj '.cproject') -Raw
$cproj = $cproj -replace '&quot;','"' -replace '&amp;','&'
$includeDirs = [regex]::Matches($cproj, '\$\{ProjDirPath\}(/[^"]+)') |
    ForEach-Object { $_.Groups[1].Value } |
    Where-Object { $_ -notmatch '\.opt' -and $_ -ne '/Configurations/Debug' } |
    Sort-Object -Unique |
    ForEach-Object { if ($_ -like '/Libraries*') { "-I`"$libs$($_ -replace '^/Libraries','')`"" } else { "-I`"$proj$_`"" } }

# --- Compile flags ---
$commonFlags = @(
    '-c',
    '-mcpu=tc23xx',
    '-D__HIGHTEC__',
    '-D__TRICORE__',
    '-std=c11',
    '-O1',
    '-g'
) + $includeDirs

# --- Gather all C sources (shared Libraries + project root) ---
$sources = Get-ChildItem $libs -Recurse -Filter '*.c' -ErrorAction SilentlyContinue
$sources += Get-ChildItem $proj -Filter '*.c'

$objList = @()
foreach ($src in $sources) {
    $srcPath = $src.FullName
    if ($srcPath.StartsWith($libs)) {
        $rel = $srcPath.Substring($libs.Length + 1).Replace('\', '_').Replace('.c', '.o')
    } else {
        $rel = $srcPath.Substring($proj.Length + 1).Replace('\', '_').Replace('.c', '.o')
    }
    $obj = Join-Path $buildDir $rel
    New-Item -ItemType Directory -Path (Split-Path $obj) -Force | Out-Null
    Write-Host "CC  $($src.Name)"
    $out = & $gcc @commonFlags "-o" $obj $src.FullName 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "FAILED: $($src.FullName)"
        $out | Select-Object -Last 20
        exit 1
    }
    $objList += $obj
}

# --- Link ---
Write-Host "Linking..."
$ldScript = Join-Path $proj 'Lcf_Gnuc_Tricore_Tc.lsl'
$elf = Join-Path $buildDir 'pwm_buzz_test.elf'
& $gcc "-mcpu=tc23xx" "-T" $ldScript "-nostdlib" "-Wl,--gc-sections" "-Wl,-Map=$buildDir\pwm_buzz_test.map" "-o" $elf @objList "-lgcc" "-lc" "-lnosys" "-lgcc" 2>&1 | Select-Object -Last 20
if (-not (Test-Path $elf)) { Write-Error "Link failed"; exit 1 }
Write-Host "Linked: $elf"

# --- Hex ---
& $objcopy -O ihex $elf (Join-Path $buildDir 'pwm_buzz_test.hex') 2>&1
Write-Host "HEX: $(Join-Path $buildDir 'pwm_buzz_test.hex')"
Write-Host "SIZE:"
& (Join-Path (Split-Path $gcc) 'tricore-elf-size.exe') $elf
