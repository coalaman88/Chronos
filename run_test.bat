@echo OFF
if not exist test.exe (cl /O2 test.c /Fetest.exe Winmm.lib)

echo -running some simple tests-
call Chronos.exe test.exe 10
echo -----------------
call Chronos.exe test.exe 100
echo -----------------
call Chronos.exe test.exe 1000
echo -----------------
call Chronos.exe test.exe 10000
echo -----------------
REM call Chronos.exe test.exe 60000
