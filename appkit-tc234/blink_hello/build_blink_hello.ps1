# Build blink_hello for TC234 (TC23A) with the AURIX GCC toolchain, standalone.
# Produces blink_hello.hex for flashing via AURIXFlasher.
#
# Requirements:
#   - AURIX GCC toolchain (tricore-elf-gcc). Tested with:
#       D:\aurixgcc_03_2026\bin\tricore-elf-gcc.exe
#   - The GCC linker script Lcf_Gnuc_Tricore_Tc.lsl already present in the
#     project root (copied from AURIX Studio bundled artefacts:
#     build_system\bundled-artefacts-repo\project-initializer\tricore-tc2xx\1.11-12\Linker_conf\GnuC\TC23A\)

$ErrorActionPreference = 'Continue'

$proj = Split-Path -Parent $MyInvocation.MyCommand.Path
$buildDir = Join-Path $env:TEMP 'blink_hello_build'
$gcc = 'D:\aurixgcc_03_2026\bin\tricore-elf-gcc.exe'
$objcopy = 'D:\aurixgcc_03_2026\bin\tricore-elf-objcopy.exe'

if (Test-Path $buildDir) { Remove-Item $buildDir -Recurse -Force }
New-Item -ItemType Directory -Path $buildDir | Out-Null

# --- Gather include paths from the TASKING .cproject (they are all relative) ---
$cproj = Get-Content (Join-Path $proj '.cproject') -Raw
$cproj = $cproj -replace '&quot;','"' -replace '&amp;','&'
$includeDirs = [regex]::Matches($cproj, '\$\{ProjDirPath\}(/[^"]+)') |
    ForEach-Object { $_.Groups[1].Value } |
    Where-Object { $_ -notmatch '\.opt' -and $_ -ne '/Configurations/Debug' } |
    Sort-Object -Unique |
    ForEach-Object { "-I`"$proj$_`"" }

# --- Compile flags (matching AURIX GCC conventions) ---
# Note: -D__HIGHTEC__ selects CompilerGnuc.h / the HighTec-style GCC startup.
# -DIFX_CFG_USE_COMPILER_DEFAULT_LINKER must NOT be defined: it disables the
# iLLD startup code (IfxCpu_CStart0.c) which we need.
$commonFlags = @(
    '-c',
    '-mcpu=tc23xx',
    '-D__HIGHTEC__',
    '-D__TRICORE__',
    '-std=c11',
    '-Wall',
    '-O1',
    '-g'
) + $includeDirs

# --- Gather all C sources (Libraries + project root) ---
$sources = Get-ChildItem (Join-Path $proj 'Libraries') -Recurse -Filter '*.c' -ErrorAction SilentlyContinue
$sources += Get-ChildItem $proj -Filter '*.c'

$objList = @()
foreach ($src in $sources) {
    $rel = $src.FullName.Substring($proj.Length + 1).Replace('\', '_').Replace('.c', '.o')
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
# Link order matters: libgcc must also come AFTER libc to resolve the soft-float
# double helpers (__divdf3 etc.) used by newlib's vsprintf.
Write-Host "Linking..."
$ldScript = Join-Path $proj 'Lcf_Gnuc_Tricore_Tc.lsl'
$elf = Join-Path $buildDir 'blink_hello.elf'
& $gcc "-mcpu=tc23xx" "-T" $ldScript "-nostdlib" "-Wl,--gc-sections" "-Wl,-Map=$buildDir\blink_hello.map" "-o" $elf @objList "-lgcc" "-lc" "-lnosys" "-lgcc" 2>&1 | Select-Object -Last 20
if (-not (Test-Path $elf)) { Write-Error "Link failed"; exit 1 }
Write-Host "Linked: $elf"

# --- Hex ---
& $objcopy -O ihex $elf (Join-Path $buildDir 'blink_hello.hex') 2>&1
Write-Host "HEX: $(Join-Path $buildDir 'blink_hello.hex')"
Write-Host "SIZE:"
& (Join-Path (Split-Path $gcc) 'tricore-elf-size.exe') $elf
