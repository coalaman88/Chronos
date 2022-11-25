@echo OFF

set name=Chronos.exe
set "compiler=cl"
set files=main.c
set libs=Winmm.lib
set warnings=/WX /W4 /wd4996 /wd4201
set debug_warnings=/wd4189 /wd4101
set debugger=/Zi
set defines=/DEXE_NAME=\"%name%\"

if /I [%1]==[debug] (
  echo -DEBUG build-
  (call "%compiler%" /nologo %debugger% %warnings% %debug_warnings% %files% %libs% /Fe%name% %defines% /DDEBUG_BUILD && (call %name%))
  exit /b
)

if /I [%1]==[release] (
  echo -RELEASE build-
  (call "%compiler%" /nologo %warnings% /O2 %files% %libs% /Fe%name% %defines% /DRELEASE_BUILD && (call %name%))
  exit /b
)

echo No build configuration!
exit /b
