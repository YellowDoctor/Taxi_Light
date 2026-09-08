@echo off
chcp 65001 >nul
cd /d "%~dp0"
title Обновление Такси Шашки до v1.3.7

:: Проверка наличия PowerShell скрипта рядом
if exist "%~dp0update_shashka.ps1" (
    powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0update_shashka.ps1"
) else (
    echo [ERROR] Файл update_shashka.ps1 не найден рядом с этим батником!
    pause
)
