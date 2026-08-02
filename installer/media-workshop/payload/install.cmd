@echo off
chcp 65001 >nul
title OBS 素材工作台安装程序
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1"
if errorlevel 1 (
  echo.
  echo 安装失败，请查看上方错误信息。
  pause
)
exit /b %ERRORLEVEL%
