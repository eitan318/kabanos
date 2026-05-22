$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Error "This script MUST be executed from an Administrator PowerShell session."
    Exit 1
}

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "   KabanOS Windows Host Environment Setup" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan

if (Get-Command python -ErrorAction SilentlyContinue) {
    Write-Host "[v] Python is already installed." -ForegroundColor Green
} else {
    Write-Host "[*] Python not detected. Installing via Winget..." -ForegroundColor Yellow
    winget install --id Python.Python.3.11 --silent --accept-package-agreements --accept-source-agreements
}

if (Get-Command docker -ErrorAction SilentlyContinue) {
    Write-Host "[v] Docker Desktop is already installed." -ForegroundColor Green
} else {
    Write-Host "[*] Docker Desktop not detected. Installing via Winget..." -ForegroundColor Yellow
    winget install --id Docker.DockerDesktop --silent --accept-package-agreements --accept-source-agreements
    Write-Host "[!] NOTE: Docker Desktop may require a machine restart to initialize its hypervisor backend." -ForegroundColor DarkYellow
}

if (Get-Command qemu-system-i386 -ErrorAction SilentlyContinue) {
    Write-Host "[v] QEMU is already installed and accessible." -ForegroundColor Green
} else {
    Write-Host "[*] QEMU not detected. Installing..." -ForegroundColor Yellow
    winget install --id SoftwareFreedomConservancy.QEMU --silent --accept-package-agreements --accept-source-agreements
    
    Write-Host "[*] Configuring global machine paths for QEMU..." -ForegroundColor Gray
    $targetDir = "C:\Program Files\qemu"
    $currentMachinePath = [Environment]::GetEnvironmentVariable("Path", "Machine")
    if ($currentMachinePath -notlike "*$targetDir*") {
        [Environment]::SetEnvironmentVariable("Path", ($currentMachinePath + ";" + $targetDir), "Machine")
    }

    $env:Path += ";$targetDir"
}

if (Test-Path "scripts/requirements.txt") {
    Write-Host "[*] Installing Python project requirements..." -ForegroundColor Yellow
    python -m pip install --upgrade pip --quiet
    python -m pip install -r scripts/requirements.txt --quiet
}

Write-Host "`n[v] Host environment ready! Run 'make build' or 'make run' to boot the OS." -ForegroundColor Green