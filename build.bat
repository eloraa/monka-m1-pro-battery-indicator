@echo off
echo Building Hello Windows Desktop Application...
echo.

REM Check if Visual Studio is available
where msbuild >nul 2>&1
if %errorlevel% neq 0 (
    echo Error: MSBuild not found. Please install Visual Studio or Visual Studio Build Tools.
    echo.
    echo You can also use CMake:
    echo   mkdir build
    echo   cd build
    echo   cmake .. -G "Visual Studio 17 2022" -A x64
    echo   cmake --build . --config Release
    pause
    exit /b 1
)

REM Build the project
echo Building with MSBuild...
msbuild Test.sln /p:Configuration=Release /p:Platform=x64 /verbosity:minimal

if %errorlevel% equ 0 (
    echo.
    echo Build successful!
    echo Executable location: x64\Release\Test.exe
    echo.
    echo To run the application:
    echo   x64\Release\Test.exe
    echo.
) else (
    echo.
    echo Build failed! Please check the error messages above.
    echo.
)

pause
