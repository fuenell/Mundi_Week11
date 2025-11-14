# Python Embeddable Package 자동 다운로드 및 설정
# 프로젝트 빌드 시 필요한 Python 환경을 자동으로 구성합니다.

param(
    [string]$PythonVersion = "3.11.9",
    [string]$ProjectRoot = (Split-Path $PSScriptRoot -Parent | Split-Path -Parent)
)

$ErrorActionPreference = "Stop"

$PythonDir = Join-Path $ProjectRoot "ThirdParty\Python"
$PythonExe = Join-Path $PythonDir "python.exe"

# Python이 이미 설치되어 있는지 확인
if (Test-Path $PythonExe) {
    Write-Host "[OK] Python already exists: $PythonExe"
    exit 0
}

Write-Host "[Setup] Downloading Python $PythonVersion (embeddable)..."

# Python embeddable package URL (Windows x64)
$PythonUrl = "https://www.python.org/ftp/python/$PythonVersion/python-$PythonVersion-embed-amd64.zip"
$ZipFile = Join-Path $env:TEMP "python-embed.zip"

try {
    # 디렉토리 생성
    New-Item -ItemType Directory -Force -Path $PythonDir | Out-Null

    # Python 다운로드
    Write-Host "[Download] $PythonUrl"
    Invoke-WebRequest -Uri $PythonUrl -OutFile $ZipFile -UseBasicParsing

    # 압축 해제
    Write-Host "[Extract] Extracting to $PythonDir"
    Expand-Archive -Path $ZipFile -DestinationPath $PythonDir -Force

    # get-pip.py 다운로드
    $GetPipUrl = "https://bootstrap.pypa.io/get-pip.py"
    $GetPipPath = Join-Path $PythonDir "get-pip.py"
    Write-Host "[Download] get-pip.py"
    Invoke-WebRequest -Uri $GetPipUrl -OutFile $GetPipPath -UseBasicParsing

    # python311._pth 파일 수정 (site-packages 활성화)
    $PthFile = Get-ChildItem -Path $PythonDir -Filter "python*._pth" | Select-Object -First 1
    if ($PthFile) {
        $PthContent = Get-Content $PthFile.FullName
        $PthContent = $PthContent -replace "^#import site", "import site"
        $PthContent | Set-Content $PthFile.FullName
        Write-Host "[Config] Enabled site-packages in $($PthFile.Name)"
    }

    # pip 설치
    Write-Host "[Install] Installing pip..."
    & $PythonExe $GetPipPath --no-warn-script-location 2>&1 | Out-Null

    # 필요한 패키지 설치
    $RequirementsFile = Join-Path $PSScriptRoot "requirements.txt"
    if (Test-Path $RequirementsFile) {
        Write-Host "[Install] Installing packages from requirements.txt..."
        & $PythonExe -m pip install -r $RequirementsFile --no-warn-script-location 2>&1 | Out-Null
    }

    # 임시 파일 정리
    Remove-Item $ZipFile -Force -ErrorAction SilentlyContinue
    Remove-Item $GetPipPath -Force -ErrorAction SilentlyContinue

    Write-Host "[OK] Python setup complete: $PythonExe"
    exit 0
}
catch {
    Write-Error "[ERROR] Failed to setup Python: $_"
    exit 1
}
