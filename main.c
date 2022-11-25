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
typedef uint64_t  u64;
typedef i32       b32;

#define array_size(X) (sizeof(X) / sizeof(X[0]))

wchar_t cmd_buffer[1024];

int wmain(int arg, wchar_t* argv[]){
  if(arg == 1){
    printf("no args!\n");
    return 0;
  }

  LARGE_INTEGER freq, start, end;
  QueryPerformanceFrequency(&freq);

  i32 cmd_buffer_index = 0;
  for(i32 i = 1; i < arg; i++){
    wchar_t *p = argv[i];
    while(*p)
      cmd_buffer[cmd_buffer_index++] = *p++;
    cmd_buffer[cmd_buffer_index++] = ' ';
  }
  cmd_buffer[cmd_buffer_index] = '\0';
  wprintf(cmd_buffer);

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);


  TIMECAPS time_caps;
  timeGetDevCaps(&time_caps, sizeof(time_caps));

  b32 result = CreateProcessW(NULL, cmd_buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
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

  const char *time_units[] = {"us", "ms", "sec", "min", "hour"};

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

  i32 index;
  for(index = array_size(clock) - 1; index > 0; index--)
    if(clock[index]) break;


  printf("\n---" EXE_NAME "---\n");
  printf("returned:%u\n", exit_code);
  for(i32 i = index; i >= 0; i--)
    printf("%d %s ", clock[i], time_units[i]);
  putchar('\n');

  return 0;
}
