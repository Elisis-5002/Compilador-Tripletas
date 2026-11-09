@echo off
rem ---------------------------------------------------------------------------
rem Compila el proyecto Java con javac y genera un JAR ejecutable.
rem Uso: scripts\compilar_java.bat
rem ---------------------------------------------------------------------------
setlocal
cd /d "%~dp0.."

where javac >nul 2>&1
if errorlevel 1 (
    echo [ERROR] No se encontro javac en el PATH.
    echo Instale un JDK 17 o superior.
    exit /b 1
)

if exist target\classes rmdir /s /q target\classes
mkdir target\classes

rem Se citan las rutas para tolerar carpetas con espacios.
> "%TEMP%\calculadora_fuentes.txt" (
    for /r "src\main\java" %%f in (*.java) do @echo "%%f"
)
javac -encoding UTF-8 -d target\classes @"%TEMP%\calculadora_fuentes.txt"
if errorlevel 1 (
    echo [ERROR] La compilacion Java fallo.
    exit /b 1
)

jar --create --file target\CalculadoraTripletas.jar --main-class com.calculadora.Main -C target\classes .
if errorlevel 1 (
    echo [ERROR] No se pudo crear el JAR.
    exit /b 1
)

echo [OK] JAR generado en target\CalculadoraTripletas.jar
endlocal
