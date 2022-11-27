/*
------------------------------------------------------------------------------
MIT License
------------------------------------------------------------------------------
Copyright (c) 2022 Coalaman88
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef int32_t   i32;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef i32       b32;

#define true  1
#define false 0

#define array_size(X) (sizeof(X) / sizeof(X[0]))

wchar_t CmdBuffer[1024];

enum ConfigFlags{
  timer_mode = 0x1,
  show_logo  = 0x2,
  show_hex   = 0x4,
  show_time  = 0x8,
  show_return_value = 0x10,
  show_return_tag   = 0x20
};

int wmain(int args, wchar_t* argv[]){

  u32 config_flags = show_time | show_logo | show_return_value | show_return_tag;
  DWORD creation_flag = 0;

  //parse args
  i32 arg;
  for(arg = 1; arg < args; arg++){
    if(argv[arg][0] != '-') break;

    if(wcscmp(argv[arg], L"-h") == 0){
      puts("Usage:\n-f: print in h:min:sec.ms,us formart\n"
           "-nologo: suppresses logo\n"
           "-noreturn: suppresses return value of the called program\n"
           "-notag: suppresses 'return:' of returned value\n"
           "-hex: show returned value in hexdecimal logo\n"
           "-console: create new console for the exe\n"
           "-silent: detach exe (no output is shown)\n"
           "-notime: supresses time");
      return 0;
    } else if(wcscmp(argv[arg], L"-f") == 0){
      config_flags |= timer_mode;
    } else if(wcscmp(argv[arg], L"-hex") == 0){
      config_flags |= show_hex;
    } else if(wcscmp(argv[arg], L"-nologo") == 0){
      config_flags &= ~show_logo;
    } else if(wcscmp(argv[arg], L"-notime") == 0){
      config_flags &= ~show_time;
    } else if(wcscmp(argv[arg], L"-noreturn") == 0){
      config_flags &= ~show_return_value;
    } else if(wcscmp(argv[arg], L"-notag") == 0){
      config_flags &= ~show_return_tag;
    } else if(wcscmp(argv[arg], L"-console") == 0){
      creation_flag = CREATE_NEW_CONSOLE;
    } else if(wcscmp(argv[arg], L"-silent") == 0){
      creation_flag = DETACHED_PROCESS;
    }else {
      puts("Bad argument! Use -h for help");
      return 1;
    }
  }
  if(arg >= args){
    puts("no input exe! Format: " EXE_NAME " [-args] [exe] [exe args]");
    return 2;
  }

  LARGE_INTEGER freq, start, end;
  QueryPerformanceFrequency(&freq);

  i32 CmdBuffer_index = 0;
  for(i32 i = arg; i < args; i++){
    wchar_t *p = argv[i];
    while(*p)
      CmdBuffer[CmdBuffer_index++] = *p++;
    CmdBuffer[CmdBuffer_index++] = ' ';
  }
  CmdBuffer[CmdBuffer_index] = '\0';

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);

  b32 result = CreateProcessW(NULL, CmdBuffer, NULL, NULL, FALSE, creation_flag, NULL, NULL, &si, &pi);
  if(!result){
    wprintf(L"\nfailed to start '%s'!\n", argv[1]);
    return 1;
  }

  QueryPerformanceCounter(&start);
  WaitForSingleObject(pi.hProcess, INFINITE);
  QueryPerformanceCounter(&end);

  DWORD exit_code;
  GetExitCodeProcess(pi.hProcess, &exit_code);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  u64 micro_freq = freq.QuadPart / 1000000;
  u64 current_time = (end.QuadPart - start.QuadPart) / micro_freq;

  i32 clock[5];

  // decimal parts
  for(i32 i = 0; i < 2; i++){
    clock[i] = current_time % 1000;
    current_time /= 1000;
  }

  // sexagenary
  for(i32 i = 2; i < array_size(clock); i++){
    clock[i] = current_time % 60;
    current_time /= 60;
  }

  if(config_flags & show_logo)
    printf("---" EXE_NAME "---\n");

  if(config_flags & show_return_value){
    if(config_flags & show_return_tag) printf("returned:");
    const char *return_format = config_flags & show_hex? "0x%x\n" : "%u\n";
    printf(return_format, exit_code);
  }

  if((config_flags & show_time) == 0) return 0;

  if(config_flags & timer_mode){
    printf("%02d:%02d:%02d.%d,%d\n", clock[4], clock[3], clock[2], clock[1], clock[0]);
    return 0;
  }

  const char *time_units[] = {"us", "ms", "sec", "min", "hour"};

  i32 index;
  for(index = array_size(clock) - 1; index > 0; index--)
    if(clock[index]) break;

  for(i32 i = index; i >= 0; i--)
    printf("%d %s ", clock[i], time_units[i]);
  putchar('\n');

  return 0;
}
