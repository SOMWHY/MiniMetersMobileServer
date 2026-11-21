<#
.SYNOPSIS
    Installs MiniMetersMobile (VST3 + CLAP) into the system plugin folders, or removes it.

.DESCRIPTION
    Copies the plugin bundles shipped next to this script into the standard Windows
    plugin paths, which is what nearly every DAW scans:

        VST3   C:\Program Files\Common Files\VST3\MiniMetersMobile.vst3
        CLAP   C:\Program Files\Common Files\CLAP\MiniMetersMobile.clap

    The standalone executable is NOT installed; run it straight out of this folder.

    Elevation is required because both targets live under Program Files. The script
    re-launches itself elevated if it is not already running as administrator.

.PARAMETER Uninstall
    Remove the installed plugin instead of installing it.

.PARAMETER SkipFirewall
    Do not add the inbound TCP 8422 rule. The phone cannot connect without it, so
    only use this if you manage the firewall yourself.

.EXAMPLE
    .\Install-MiniMetersMobile.ps1

.EXAMPLE
    .\Install-MiniMetersMobile.ps1 -Uninstall
#>
[CmdletBinding()]
param(
    [switch] $Uninstall,
    [switch] $SkipFirewall
)

$ErrorActionPreference = 'Stop'

$Port = 8422
$RuleName = 'MiniMetersMobile (TCP 8422)'
$Vst3Dir = Join-Path $env:ProgramFiles 'Common Files\VST3'
$ClapDir = Join-Path $env:ProgramFiles 'Common Files\CLAP'
$Vst3Target = Join-Path $Vst3Dir 'MiniMetersMobile.vst3'
$ClapTarget = Join-Path $ClapDir 'MiniMetersMobile.clap'

# --- elevation --------------------------------------------------------------
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator
)

if (-not $isAdmin) {
    Write-Host 'Administrator rights are required to write to Program Files. Asking for elevation...'
    $argList = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"")
    if ($Uninstall) { $argList += '-Uninstall' }
    if ($SkipFirewall) { $argList += '-SkipFirewall' }
    try {
        Start-Process -FilePath 'powershell.exe' -ArgumentList $argList -Verb RunAs -Wait
    } catch {
        Write-Host ''
        Write-Host 'Elevation was cancelled or refused. Nothing was installed.' -ForegroundColor Yellow
        Write-Host 'You can also copy the plugin by hand - see INSTALL.md.' -ForegroundColor Yellow
        exit 1
    }
    exit 0
}

function Remove-Path($Path) {
    if (-not (Test-Path -LiteralPath $Path)) { return }
    # A .clap is a file; a .vst3 is a bundle directory. They need different calls, and
    # Remove-Item -Recurse is unreliable inside Program Files.
    if (Test-Path -LiteralPath $Path -PathType Container) {
        [IO.Directory]::Delete($Path, $true)
    } else {
        [IO.File]::Delete($Path)
    }
}

function Copy-Bundle($Source, $Target) {
    if (-not (Test-Path -LiteralPath $Source)) { throw "Missing from this folder: $Source" }
    if (Test-Path -LiteralPath $Target) {
        # Replacing in place would nest the bundle inside itself, so drop the old one first.
        Remove-Path $Target
    }
    if (Test-Path -LiteralPath $Source -PathType Container) {
        Copy-Item -LiteralPath $Source -Destination $Target -Recurse -Force
    } else {
        Copy-Item -LiteralPath $Source -Destination $Target -Force
    }
}

# --- uninstall --------------------------------------------------------------
if ($Uninstall) {
    Write-Host 'Removing MiniMetersMobile...'
    Remove-Path $Vst3Target
    Write-Host "  removed $Vst3Target"
    Remove-Path $ClapTarget
    Write-Host "  removed $ClapTarget"

    if (-not $SkipFirewall) {
        Remove-NetFirewallRule -DisplayName $RuleName -ErrorAction SilentlyContinue
        Write-Host "  removed firewall rule '$RuleName'"
    }

    Write-Host ''
    Write-Host 'Done. Rescan plugins in your DAW to drop it from the list.'
    exit 0
}

# --- install ----------------------------------------------------------------
Write-Host 'Installing MiniMetersMobile...'
Write-Host ''

New-Item -ItemType Directory -Path $Vst3Dir -Force | Out-Null
Copy-Bundle (Join-Path $PSScriptRoot 'MiniMetersMobile.vst3') $Vst3Target
Write-Host "  VST3 -> $Vst3Target"

if (Test-Path -LiteralPath (Join-Path $PSScriptRoot 'MiniMetersMobile.clap')) {
    New-Item -ItemType Directory -Path $ClapDir -Force | Out-Null
    Copy-Bundle (Join-Path $PSScriptRoot 'MiniMetersMobile.clap') $ClapTarget
    Write-Host "  CLAP -> $ClapTarget"
} else {
    Write-Host '  CLAP skipped (not present in this folder)'
}

if (-not $SkipFirewall) {
    # Without this the plugin reports "Network: unavailable" and the phone never connects.
    $existing = Get-NetFirewallRule -DisplayName $RuleName -ErrorAction SilentlyContinue
    if (-not $existing) {
        New-NetFirewallRule -DisplayName $RuleName -Direction Inbound -Action Allow `
            -Protocol TCP -LocalPort $Port -Profile Private | Out-Null
        Write-Host "  firewall: allowed inbound TCP $Port on private networks"
    } else {
        Write-Host "  firewall: rule already present"
    }
}

Write-Host ''
Write-Host 'Installed. Next steps:' -ForegroundColor Green
Write-Host '  1. Rescan / rescan plugins in your DAW (or restart it).'
Write-Host '  2. Insert MiniMetersMobile on a track, or run MiniMetersMobile.exe.'
Write-Host '  3. Scan the QR code with the phone app.'
Write-Host ''
Write-Host 'To remove later:  .\Install-MiniMetersMobile.ps1 -Uninstall' -ForegroundColor DarkGray
