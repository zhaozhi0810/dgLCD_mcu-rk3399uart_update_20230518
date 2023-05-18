
@echo off

@echo ===================================
@echo = start generate bin file         =
@echo ===================================

md ..\output >nul 2>nul

for /f "delims=" %%i in ('find "FIRMWARE_VERSION" ..\app\version.h') do (set a=%%i)
set version=%a:~26,-1%

set name=%1
set name=%name:~0,-4%-%version%-%date:~0,4%%date:~5,2%%date:~8,2%%time:~0,2%%time:~3,2%%time:~6,2%.hex
copy ..\project\Objects\%1 ..\output\%name% >nul 2>nul

echo hex file: %name%

@echo ===================================
