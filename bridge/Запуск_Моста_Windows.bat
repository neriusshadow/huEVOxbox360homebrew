@echo off
title huEVO Companion Bridge for Xbox 360
color 0b
echo ===================================================================
echo   huEVO Xbox 360 Companion Bridge (Universal Autonomous Edition)
echo   Подключение консоли Xbox 360 к социальной сети https://huevo.su
echo ===================================================================
echo.
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [!] Ошибка: Python 3 не найден в системе.
    echo Пожалуйста, установите Python 3 с сайта https://www.python.org/ (при установке поставьте галочку "Add to PATH").
    echo.
    pause
    exit /b
)
echo [*] Запуск моста и UDP авто-обнаружения для Xbox 360...
python "%~dp0bridge.py"
pause
