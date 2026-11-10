@echo off
REM Compile and upload a stick sketch.
REM Usage:  stick\upload.bat [sketch_folder] [COM port]
REM   default sketch: stick\trivia_stick    default port: COM4
REM   e.g.  stick\upload.bat stick\hello_stick
REM If the upload can't connect: hold the side button ~2 s until the green
REM LED blinks (download mode), then run this again.
set SKETCH=%~1
if "%SKETCH%"=="" set SKETCH=%~dp0trivia_stick
set PORT=%~2
if "%PORT%"=="" set PORT=COM4
set FQBN=m5stack:esp32:m5stack_sticks3

REM src\core is shared with the Windows build; pass it in as a library.
arduino-cli compile --fqbn %FQBN% --library "%~dp0..\src\core" "%SKETCH%"
if errorlevel 1 exit /b 1
arduino-cli upload -p %PORT% --fqbn %FQBN% "%SKETCH%"
