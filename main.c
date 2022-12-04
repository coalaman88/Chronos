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
  show_return_tag   = 0x20,
  show_full_time    = 0x40,
  use_exit_code     = 0x80
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

void build_clock(i32 clock[5], u64 time){
  // decimal parts
  for(i32 i = 0; i < 2; i++){
    clock[i] = time % 1000;
    time /= 1000;
  }

  // sexagenary
  for(i32 i = 2; i < 5; i++){
    clock[i] = time % 60;
    time /= 60;
  }
}

static inline u64 filetime_to_microsecs(FILETIME f){
  u64 t = (u64)f.dwHighDateTime << 32 | (u64)f.dwLowDateTime;
  return t / 10; // 100-nanosecs to microsecs
}

void print_nice_format_clock(const char *tag, i32 clock[5]){
  // find nonzero start time unit
  i32 index;
  for(index = 4; index > 0; index--)
    if(clock[index]) break;

  const char *time_units[] = {"us", "ms", "sec", "min", "hour"};

  printf(tag);
  for(i32 i = index; i >= 0; i--)
    printf("%d %s ", clock[i], time_units[i]);
  putchar('\n');
}

i32 wmain(i32 args, wchar_t* argv[]){

  // Prepare console to accept Virtual Terminal Sequences
  HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
  assert(out);
  DWORD console_mode;
  assert(GetConsoleMode(out, &console_mode));
  if(!(console_mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)){
    console_mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    assert(SetConsoleMode(out, console_mode));
  }

  // flags
  u32 config_flags = show_time | show_logo | show_return_value | show_return_tag;
  DWORD cp_priority_flag = 0; // default priority class of the calling process
  DWORD cp_options_flag  = 0;

  // parse args
  i32 arg;
  for(arg = 1; arg < args; arg++){
    if(argv[arg][0] != '-') break;

    if(wcscmp(argv[arg], L"-h") == 0){
      puts("Usage:\n-f: print in h:min:sec.ms,us formart\n"
           "-c: show total user and kernel time\n"
           "-nologo: suppresses logo\n"
           "-noreturn: suppresses return value of the called program\n"
           "-z: use return value of called program as exit code\n"
           "-notag: suppresses 'return:' of returned value\n"
           "-hex: show returned value in hexdecimal\n"
           "-console: create new console for the exe\n"
           "-silent: detach exe (no output is shown)\n"
           "-notime: supresses time\n"
           "-priority: start exe with specified priority level -2 to 3 (low to high). Default is inherited. See win32 priority class flags for more info.");
      return 0;
    } else if(wcscmp(argv[arg], L"-f") == 0){
      config_flags |= timer_mode;
    } else if(wcscmp(argv[arg], L"-c") == 0){
      config_flags |= show_full_time;
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
    } else if(wcscmp(argv[arg], L"-z") == 0){
      config_flags |= use_exit_code;
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

  FILETIME creation, exit, kernel, user;
  assert(GetProcessTimes(pi.hProcess, &creation, &exit, &kernel, &user));

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  // process time
  const u64 micro_freq = freq.QuadPart / 1000000;
  const u64 elapsed_time = (end.QuadPart - start.QuadPart) / micro_freq;

  // alternative elapsed time from GetProcessTimes
  // u64 getproctimes_elapsed = filetime_to_microsecs(exit) - filetime_to_microsecs(creation);

  i32 elapsed_clock[5], kernel_clock[5], user_clock[5];
  build_clock(elapsed_clock, elapsed_time);
  build_clock(user_clock, filetime_to_microsecs(user));
  build_clock(kernel_clock, filetime_to_microsecs(kernel));

  // display results
  if(config_flags & show_logo)
    printf("---" EXE_NAME "---\n");

  if(config_flags & show_return_value){
    printf(exit_code? "\x1b[1;31m" : "\x1b[1;32m"); // foreground = red : gren
    if(config_flags & show_return_tag) printf("returned: ");
    const char *return_format = config_flags & show_hex? "0x%x\n\x1b[0m" : "%u\n\x1b[0m"; // reset foreground
    printf(return_format, exit_code);
  }

  i32 success_code = (config_flags & use_exit_code)? exit_code : 0;

  if((config_flags & show_time) == 0) return success_code;

  if(config_flags & timer_mode){
    const i32 *c = elapsed_clock, *u = user_clock, *k = kernel_clock;
    printf("elapsed: %02d:%02d:%02d.%03d,%03d\n", c[4], c[3], c[2], c[1], c[0]);
    if(config_flags & show_full_time){
      printf("user:    %02d:%02d:%02d.%03d,%03d\n", u[4], u[3], u[2], u[1], u[0]);
      printf("kernel:  %02d:%02d:%02d.%03d,%03d\n", k[4], k[3], k[2], k[1], k[0]);
    }
    return success_code;
  }

  print_nice_format_clock("elapsed: ", elapsed_clock);
  if(config_flags & show_full_time){
    print_nice_format_clock("user:    ", user_clock);
    print_nice_format_clock("kernel:  ", kernel_clock);
  }

  return success_code;
}
