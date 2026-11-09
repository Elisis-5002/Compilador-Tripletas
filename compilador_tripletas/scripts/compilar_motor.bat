@echo off
rem ---------------------------------------------------------------------------
rem Compila el motor nativo en C para Windows usando MinGW-w64.
rem Uso: scripts\compilar_motor.bat
rem ---------------------------------------------------------------------------
setlocal
cd /d "%~dp0.."

where gcc >nul 2>&1
if errorlevel 1 (
    echo [ERROR] No se encontro gcc en el PATH.
    echo Instale MinGW-w64 y agregue su carpeta bin al PATH.
    echo Ejemplo: C:\mingw64\bin
    exit /b 1
)

if not exist native (
    echo [ERROR] No existe la carpeta native.
    exit /b 1
)

echo Compilando native\motor.c ...
gcc -O2 -Wall -Wextra -std=c11 -o native\motor.exe native\motor.c
if errorlevel 1 (
    echo [ERROR] La compilacion del motor fallo.
    exit /b 1
)

echo [OK] Motor compilado en native\motor.exe
endlocal
