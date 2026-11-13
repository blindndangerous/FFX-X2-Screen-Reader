# Prints the FINAL FANTASY FFX&FFX-2 HD Remaster folder and exits 0.
# Order: FFX_GAME_DIR, then every Steam library. Exits 1 with one ERROR line
# when nothing is found. Writes nothing to disk.
$ErrorActionPreference = 'Stop'
$name = 'FINAL FANTASY FFX&FFX-2 HD Remaster'

function Test-Game([string]$dir) {
    return $dir -and (Test-Path -LiteralPath (Join-Path $dir 'FFX.exe'))
}

if ($env:FFX_GAME_DIR -and (Test-Game $env:FFX_GAME_DIR)) {
    Write-Output $env:FFX_GAME_DIR
    exit 0
}

$steam = $null
foreach ($key in @(
        @{ Path = 'HKCU:\Software\Valve\Steam'; Name = 'SteamPath' },
        @{ Path = 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam'; Name = 'InstallPath' })) {
    $value = (Get-ItemProperty -LiteralPath $key.Path -ErrorAction SilentlyContinue).($key.Name)
    if ($value) { $steam = $value; break }
}

$libraries = @()
if ($steam) {
    $libraries += $steam
    $vdf = Join-Path $steam 'steamapps\libraryfolders.vdf'
    if (Test-Path -LiteralPath $vdf) {
        foreach ($m in [regex]::Matches((Get-Content -LiteralPath $vdf -Raw), '"path"\s+"([^"]+)"')) {
            $libraries += $m.Groups[1].Value.Replace('\', '\')
        }
    }
}

foreach ($lib in ($libraries | Select-Object -Unique)) {
    $candidate = Join-Path $lib "steamapps\common\$name"
    if (Test-Game $candidate) {
        Write-Output ([System.IO.Path]::GetFullPath($candidate))
        exit 0
    }
}

Write-Output 'ERROR: game folder not found. Set FFX_GAME_DIR.'
exit 1
