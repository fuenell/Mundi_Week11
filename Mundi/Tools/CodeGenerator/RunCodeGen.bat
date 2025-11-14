@echo off
REM Visual Studio Pre-Build Event용 코드 생성 스크립트

setlocal

REM 프로젝트 루트 Python 경로
set "EMBEDDED_PYTHON=%~dp0..\..\ThirdParty\Python\python.exe"

REM 1. 프로젝트 내장 Python 체크
if exist "%EMBEDDED_PYTHON%" (
    "%EMBEDDED_PYTHON%" "%~dp0generate.py" %*
    exit /b %ERRORLEVEL%
)

REM 2. 시스템 Python 체크
py --version >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    py "%~dp0generate.py" %*
    exit /b %ERRORLEVEL%
)

REM 3. Python 없음 - 자동 다운로드 시도
echo [CodeGen] Python not found. Attempting automatic setup...
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0EnsurePython.ps1"

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Failed to setup Python automatically.
    echo Please install Python manually or check network connection.
    exit /b 1
)

REM 4. 다운로드된 Python으로 재시도
if exist "%EMBEDDED_PYTHON%" (
    "%EMBEDDED_PYTHON%" "%~dp0generate.py" %*
    exit /b %ERRORLEVEL%
)

echo [ERROR] Python setup succeeded but executable not found!
exit /b 1
