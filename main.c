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
#include <assert.h>

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

static inline i32 wparse_i32(const wchar_t *s, b32 *error){
  assert(error);
  if(!s){
    *error = true;
    return 0;
  }

  i32 value = 0;
  i32 sign = *s == '-'? s++, -1 : 1;

  while(*s >= '0' && *s <= '9'){
    value *= 10;
    value += *s - '0';
    s++;
  }

  *error = *s != 0;
  return value * sign;
}

static inline u32 wappend(wchar_t *d, u32 d_size, const wchar_t *s){
  u32 i = 0;
  while(s[i]){
    assert(i < d_size);
    d[i] = s[i]; i++;
  }
  return i;
}

i32 wmain(i32 args, wchar_t* argv[]){

  u32 config_flags = show_time | show_logo | show_return_value | show_return_tag;
  DWORD cp_priority_flag = 0; // default priority class of the calling process
  DWORD cp_options_flag  = 0;

  // parse args
  i32 arg;
  for(arg = 1; arg < args; arg++){
    if(argv[arg][0] != '-') break;

    if(wcscmp(argv[arg], L"-h") == 0){
      puts("Usage:\n-f: print in h:min:sec.ms,us formart\n"
           "-nologo: suppresses logo\n"
           "-noreturn: suppresses return value of the called program\n"
           "-notag: suppresses 'return:' of returned value\n"
           "-hex: show returned value in hexdecimal\n"
           "-console: create new console for the exe\n"
           "-silent: detach exe (no output is shown)\n"
           "-notime: supresses time\n"
           "-priority: start exe with specified priority level -2 to 3 (low to high). See win32 priority class flags for more info.");
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
    } else if(wcscmp(argv[arg], L"-priority") == 0){
      if(++arg >= args){
        printf("incomplete argument! use -h for help");
        return 1;
      }

      const DWORD win_priority_flags[6] = {IDLE_PRIORITY_CLASS, BELOW_NORMAL_PRIORITY_CLASS, NORMAL_PRIORITY_CLASS,
        ABOVE_NORMAL_PRIORITY_CLASS, HIGH_PRIORITY_CLASS, REALTIME_PRIORITY_CLASS};

      b32 error;
      const wchar_t *level_arg = argv[arg];
      const i32 level = wparse_i32(level_arg, &error);
      if(error){
        wprintf(L"invalid '%s' priority level, integer expected!\n", argv[arg]);
        return 2;
      }

      if(level < -2 || level > 3){
        printf("out of range priority level '%d'! -priority [-2, 3]\n", level);
        return 3;
      }

      cp_priority_flag = win_priority_flags[2 + level];
    } else if(wcscmp(argv[arg], L"-console") == 0){
      cp_options_flag = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE;
    } else if(wcscmp(argv[arg], L"-silent") == 0){
      cp_options_flag  = NORMAL_PRIORITY_CLASS | DETACHED_PROCESS;
    }else {
      puts("Bad argument! Use -h for help");
      return 4;
    }
  }

  if(arg >= args){
    puts("no input exe! Format: " EXE_NAME " [-args] [exe] [exe args]");
    return 5;
  }

  // copying args
  u32 CmdBuffer_index = 0;
  const u32 buff_len = array_size(CmdBuffer);
  for(i32 i = arg; i < args; i++){
    CmdBuffer_index += wappend(CmdBuffer + CmdBuffer_index, buff_len, argv[i]);
    CmdBuffer_index += wappend(CmdBuffer + CmdBuffer_index, buff_len, L" ");
  }
  CmdBuffer[CmdBuffer_index] = '\0';

  LARGE_INTEGER freq, start, end;
  QueryPerformanceFrequency(&freq);

  // start process
  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);

  b32 result = CreateProcessW(NULL, CmdBuffer, NULL, NULL, FALSE, cp_options_flag | cp_priority_flag, NULL, NULL, &si, &pi);
  if(!result){
    wprintf(L"failed to start '%s'!\n", argv[arg]);
    return 6;
  }

  // wait for process to finish
  QueryPerformanceCounter(&start);
  WaitForSingleObject(pi.hProcess, INFINITE);
  QueryPerformanceCounter(&end);

  DWORD exit_code;
  GetExitCodeProcess(pi.hProcess, &exit_code);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  // process time
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
