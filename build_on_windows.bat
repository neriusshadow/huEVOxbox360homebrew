@echo off
chcp 65001 >nul
title huEVO Xbox 360 Build Script
echo ========================================================
echo   huEVO Xbox 360 Homebrew - Сборка default.xex
echo ========================================================
echo.

set VSWHERE="%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

REM Проверяем наличие установленной Visual Studio с поддержкой C++
if exist %VSWHERE% (
    for /f "usebackq tokens=*" %%i in (`%VSWHERE% -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
        set MSBUILD="%%i"
    )
)

if not defined MSBUILD (
    if exist "%SystemRoot%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe" (
        set MSBUILD="%SystemRoot%\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe"
    )
)

echo [1/3] Поиск компилятора MSBuild...
if defined MSBUILD (
    echo Найден MSBuild: %MSBUILD%
    echo.
    echo [2/3] Запуск сборки huEVO.sln для Xbox 360...
    %MSBUILD% huEVO.sln /p:Configuration=Release /p:Platform="Xbox 360"
    if %ERRORLEVEL% EQU 0 (
        echo.
        echo [3/3] СБОРКА УСПЕШНА!
        echo Файл default.xex создан в папке bin\Release\default.xex
        echo Скопируйте его на Xbox 360 в Hdd1:\Homebrew\huEVO\
    ) else (
        echo.
        echo [ОШИБКА] Сборка завершилась с ошибкой.
        echo Убедитесь, что установлен Xbox 360 SDK (XDK 21256) в Visual Studio.
    )
) else (
    echo [!] MSBuild не найден.
    echo Откройте huEVO.sln вручную в Visual Studio 2010 и нажмите Сборка (Build).
)

echo.
pause
