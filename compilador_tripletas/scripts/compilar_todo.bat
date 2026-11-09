@echo off
rem ---------------------------------------------------------------------------
rem Compila el motor nativo y el proyecto Java en un solo paso.
rem Uso: scripts\compilar_todo.bat
rem ---------------------------------------------------------------------------
setlocal
cd /d "%~dp0.."

call scripts\compilar_motor.bat
if errorlevel 1 exit /b 1

call scripts\compilar_java.bat
if errorlevel 1 exit /b 1

echo.
echo [OK] Proyecto listo. Ejecute scripts\ejecutar.bat
endlocal
