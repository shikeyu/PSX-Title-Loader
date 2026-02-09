@echo off
set PSYQ_PATH=C:\psyq
set PATH=%PSYQ_PATH%\bin;%PATH%
set LIBRARY_PATH=%PSYQ_PATH%\lib
set C_INCLUDE_PATH=%PSYQ_PATH%\include

REM 1. Compile using ccpsx
echo Compiling...
REM We create a dummy 'lib' file to satisfy the broken psyq.ini
if not exist lib type nul > lib

REM Use -v to see what's happening
ccpsx -O2 -Xo$80100000 main.c -omain.cpe,main.sym,mem.map -llibapi.lib -llibgpu.lib -llibgte.lib -llibetc.lib -llibcd.lib -llibpress.lib

if exist main.cpe (
    echo Converting to EXE...
    cpe2x main.cpe
    echo Build Successful!
) else (
    echo Build Failed!
)
pause
