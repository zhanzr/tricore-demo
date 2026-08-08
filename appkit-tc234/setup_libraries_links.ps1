# Recreate per-project Libraries junctions pointing to the shared board-level
# Libraries folder. Only needed if you open the projects in the AURIX Studio IDE
# (its .cproject references ${ProjDirPath}/Libraries). The CLI build scripts
# reference the shared folder directly and do NOT need these junctions.
#
# Run from this board folder:
#   powershell -ExecutionPolicy Bypass -File setup_libraries_links.ps1

$board = Split-Path -Parent $MyInvocation.MyCommand.Path
$target = Join-Path $board 'Libraries'

if (-not (Test-Path -LiteralPath $target)) {
    Write-Error "Shared Libraries folder not found: $target"
    exit 1
}

$projects = @('blink_hello', 'dhry_200m', 'coremark_200m', 'pwm_buzz_test')

foreach ($p in $projects) {
    $link = Join-Path $board (Join-Path $p 'Libraries')
    if (Test-Path -LiteralPath $link) {
        Write-Host "Exists (leave as-is): $link"
        continue
    }
    cmd /c mklink /J "`"$link`"" "`"$target`"" | Out-Null
    if (Test-Path -LiteralPath $link) {
        Write-Host "Linked: $link -> $target"
    } else {
        Write-Host "FAILED: $link"
    }
}
