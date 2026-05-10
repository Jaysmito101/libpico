@echo off
setlocal EnableExtensions EnableDelayedExpansion

where clang-format >nul 2>&1
if errorlevel 1 (
    echo [ERROR] clang-format was not found in PATH.
    echo Install clang-format and try again.
    exit /b 1
)

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set /a COUNT=0
set /a FAIL=0

call :format_dir "%ROOT%\include" 0
if errorlevel 1 set /a FAIL=1

call :format_dir "%ROOT%\examples" 1
if errorlevel 1 set /a FAIL=1

echo.
echo Formatted !COUNT! file(s).
if !FAIL! neq 0 (
    echo Completed with errors.
    exit /b 1
)

echo Done.
exit /b 0

:format_dir
set "DIR=%~1"
set "SKIP_VENDOR=%~2"
if "%SKIP_VENDOR%"=="" set "SKIP_VENDOR=0"

if not exist "%DIR%" (
    echo [WARN] Skipping missing directory: %DIR%
    exit /b 0
)

for /R "%DIR%" %%F in (*.c *.cc *.cpp *.cxx *.h *.hh *.hpp *.hxx *.inl *.ipp) do (
    set "FILE=%%~fF"
    set "DO_FORMAT=1"

    if /I "!SKIP_VENDOR!"=="1" (
        set "NO_VENDOR=!FILE:\vendor\=!"
        if /I not "!NO_VENDOR!"=="!FILE!" set "DO_FORMAT=0"
    )

    if "!DO_FORMAT!"=="1" (
        clang-format -i -style=file "!FILE!" >nul 2>&1
        if errorlevel 1 (
            echo [ERROR] Failed to format: !FILE!
            exit /b 1
        )
        set /a COUNT+=1
        echo [OK] !FILE!
    ) else (
        echo [SKIP] !FILE!
    )
)

exit /b 0
