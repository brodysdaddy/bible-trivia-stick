@echo off
REM Builds trivia.exe with whichever compiler is available.
REM   - MinGW / MSYS2 g++   (run from a normal terminal with g++ on PATH)
REM   - Visual Studio cl    (run from "Developer Command Prompt for VS")
REM Optional: build.bat other_name.exe   to choose the output file.

set OUT=%~1
if "%OUT%"=="" set OUT=trivia.exe

set SRC=src\core\questions.cpp src\core\trivia_game.cpp src\console\main.cpp src\console\sound_windows.cpp

where g++ >nul 2>nul
if %errorlevel%==0 (
    g++ -std=c++17 -Wall -Wextra -O2 %SRC% -o "%OUT%"
    goto done
)

where cl >nul 2>nul
if %errorlevel%==0 (
    cl /nologo /std:c++17 /EHsc /W4 /O2 %SRC% /Fe:"%OUT%"
    goto done
)

echo No C++ compiler found. Install MSYS2 (g++) or Visual Studio Build Tools.
exit /b 1

:done
set RESULT=%errorlevel%
del *.obj >nul 2>nul
if not %RESULT%==0 (
    echo BUILD FAILED. If it says "cannot open file", close any running %OUT% and try again.
    exit /b %RESULT%
)
echo Built %OUT% - run it with: %OUT%
