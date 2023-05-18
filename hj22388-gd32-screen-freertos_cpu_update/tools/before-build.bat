
@echo off

@echo ===================================
@echo = start generate version info     =
@echo ===================================

del /s /q /f ..\tools\version_info.h >nul 2>nul

@echo #ifndef __VERSION_INFO_H >> ..\tools\version_info.h
@echo #define __VERSION_INFO_H >> ..\tools\version_info.h

@echo #define COMPILE_DATE  "%date:~0,10%" >> ..\tools\version_info.h
@echo #define COMPILE_TIME  "%time%" >> ..\tools\version_info.h

for /f "delims=" %%i in ('git rev-parse HEAD') do (set a=%%i)
echo #define GIT_COMMIT_HASH "%a%" >> ..\tools\version_info.h
echo git hash: %a%

for /f "delims=" %%i in ('git symbolic-ref --short HEAD') do (set a=%%i)
echo #define GIT_BRANCH "%a%" >> ..\tools\version_info.h
echo git brach: %a%

for /f "delims=" %%i in ('git log --pretty^=format:%%an HEAD -1') do (set a=%%i)
echo #define GIT_AUTHOR "%a%" >> ..\tools\version_info.h
echo git author: %a%

for /f "delims=" %%i in ('git log --pretty^=format:%%ai HEAD -1') do (set a=%%i)
echo #define GIT_COMMIT_DATE "%a%" >> ..\tools\version_info.h
echo git date: %a%

for /f "delims=" %%i in ('git log --pretty^=format:%%s HEAD -1') do (set a=%%i)
echo #define GIT_COMMIT_STR "%a%" >> ..\tools\version_info.h
echo git commit: %a%

@echo #endif >> ..\tools\version_info.h

copy ..\tools\version_info.h ..\app\version_info.h >nul 2>nul
echo generate app\version_info.h

@echo ===================================
