@echo off
rem ---------------------------------------------------------------------------
rem Compila (si hace falta) y ejecuta la aplicacion.
rem Requiere que native\motor.exe exista.
rem Uso: scripts\ejecutar.bat
rem ---------------------------------------------------------------------------
setlocal
cd /d "%~dp0.."

if not exist target\CalculadoraTripletas.jar (
    call scripts\compilar_java.bat
    if errorlevel 1 exit /b 1
)

if not exist native\motor.exe (
    echo [AVISO] No existe native\motor.exe.
    echo Compilelo con: scripts\compilar_motor.bat
)

java -jar target\CalculadoraTripletas.jar
endlocal
