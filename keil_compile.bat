@echo off
setlocal enabledelayedexpansion

git reset

:: 1. Input custom name
set /p "custom_name=Enter version name (press Enter to skip): "
if "%custom_name%"=="" (echo No custom name entered) else echo Using name: %custom_name%
echo.

:: 2. Compile project

:: Delete all .txt files in the log directory
del "%LOG_DIR%\*.txt" 2>nul

:: Delete all .bin files in current and subdirectories
del /s /q *.bin

:: Set the log directory path
set "LOG_DIR=code\keil\Template\Keil5_project"

:: Print message for cleaning
echo Cleaning project...

:: Execute the clean operation
UV4.exe -j0 -c "%LOG_DIR%\Project.uvprojx" -o clean_log.txt && (

    :: Print message for compiling
    echo Compiling project...

    :: Execute the build operation
    UV4.exe -j0 -r "%LOG_DIR%\Project.uvprojx" -t GD32F30X_HD -o build_log.txt || (
        echo Compilation failed
        type "%LOG_DIR%\clean_log.txt" "%LOG_DIR%\build_log.txt" > "%LOG_DIR%\compile_log.txt"
        vim "%LOG_DIR%\compile_log.txt"
        exit /b 1
    )
)

:: Merge logs and output the result
type "%LOG_DIR%\clean_log.txt" "%LOG_DIR%\build_log.txt" > "%LOG_DIR%\compile_log.txt"
echo Compilation successful
:: 3. Record source code changes
for /f %%i in ('powershell -command "Get-Date -Format 'yyyyMMddHHmmss'"') do set "datetime=%%i"
set "target_dir=..\bin\dcdc\%datetime%"
if defined custom_name set "target_dir=%target_dir%_%custom_name%"

:: Ensure directory exists
if not exist "%target_dir%" (
    mkdir "%target_dir%"
    if errorlevel 1 (
        echo Failed to create directory: %target_dir%
        exit /b 1
    )
)

:: Generate diff.txt
(
    echo ===== Current Commit ID =====
    git rev-parse HEAD
    echo.
    echo ===== Source Code Changes =====
    git -c color.diff=never diff -- "*.c" "*.h"
    echo.
    echo ===== File List =====
    git diff --name-only -- "*.c" "*.h" || echo No changes
) > "%target_dir%\diff.txt"
echo Change record generated: %target_dir%\diff.txt
echo.

:: 4. Process bin files
echo Processing bin files...
set "count=0"
for /r . %%f in (*.bin) do (
    set /a "count+=1"
    set "original_name=%%~nxf"
    set "new_name=%datetime%"
    if defined custom_name set "new_name=%datetime%_%custom_name%"
    set "new_name=!new_name!_!original_name!"
    
    echo Original file: %%f
    echo New filename: !new_name!
    
    copy "%%f" "%target_dir%\!new_name!" >nul && (
        echo Successfully copied to: %target_dir%\!new_name!
    ) || (
        echo Copy failed: %%f
    )
    echo.
)

if %count% equ 0 (
    echo No .bin files found
    echo Checking path: %cd%
    dir /s *.bin 2>nul || echo (No .bin files found)
)
echo.

:: 5. Copy compile_log.txt to target directory (new feature)
if exist "%LOG_DIR%\compile_log.txt" (
    copy "%LOG_DIR%\compile_log.txt" "%target_dir%\compile_log.txt" >nul && (
        echo Log copied: %target_dir%\compile_log.txt
    ) || (
        echo [Error] Log copy failed
    )
) else (
    echo Original log file not found: %LOG_DIR%\compile_log.txt
)
echo.

git add -u *.c *.h

:: 6. Open log
if exist "%LOG_DIR%\compile_log.txt" (
    vim "%LOG_DIR%\compile_log.txt"
)
exit /b 0
