/* Copyright 2009, Mikael Patel
   This file is part of vfm, virtual forth machine project.
 
   vfm is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
 
   vfm is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with vfm.  If not, see <http://www.gnu.org/licenses/>. */

#include "vfm.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#define RETURN_STACK_SIZE 128
#define DATA_STACK_SIZE 256
#define DATA_HEAP_SIZE 32 * 1024

// NB: Change module to include to change test program (vft)

#include "test/test3.i"

#define mod test_test3_mod

int main(int argc, char* argv[])
{
  vfm_env_t env;
  vfm_code_t catch[] = { VFM_OP_HALT };
  vfm_code_t* rp0[RETURN_STACK_SIZE] = { catch };
  vfm_data_t sp0[DATA_STACK_SIZE];
  vfm_data_t dp0[DATA_HEAP_SIZE];
  int status = VFM_NORMAL_STATUS;
  char* entry = 0;
  int benchmark = 0;
  int coverage = 0;
  int profile = 0;
  int opterr = 0;
  int times = 1;
  int errno;
  int c;

  // Check options
  while ((c = getopt(argc, argv, "b:ce:pt")) != EOF)
    switch (c) {
    case 'b':
      benchmark = 1;
      times = atoi(optarg);
      break;
    case 'c':
      coverage = 1;
      status |= VFM_PROFILING_STATUS;
      break;
    case 'e':
      entry = optarg;
      break;
    case 'p':
      status |= VFM_PROFILING_STATUS;
      profile = 1;
      break;
    case 't':
      status |= VFM_TRACING_STATUS;
      break;
    case '?':
    default:
      opterr = 1;
    }

  // Check parameters
  if ((argc != optind) || opterr) {
    fprintf(stderr, "usage: vft [-cpt][-b times][-e entry]\n");
    fprintf(stderr, "vfm test program, demo source compile and include\n");
    fprintf(stderr, "  -b 	measure execution time (times)\n");
    fprintf(stderr, "  -c	measure code coverage when profiling\n");
    fprintf(stderr, "  -e 	start symbol (entry)\n");
    fprintf(stderr, "  -p	profile execution\n");
    fprintf(stderr, "  -t	trace execution\n");
    return (-1);
  }
  if (benchmark && times <= 0) {
    fprintf(stderr, "error: illegal benchmark times\n");
    return (-1);
  }

  // Initiate run-time
  vfm_init();

  // Check for entry symbol
  if (entry) {
    vfm_symb_t* symb = vfm_name2symb(entry, &mod.dict);
    if (!symb) {
      fprintf(stderr, "%s: unknown entry\n", entry);
      return (-1);
    }
    mod.segment.entry = symb->code;
  }
  if (!mod.segment.entry) {
    fprintf(stderr, "error: no entry defined\n");
    return (-1);
  }

  // Run entry
  errno = 0;
  if (benchmark) {
    struct timeval start;
    struct timeval stop;
    gettimeofday(&start, NULL);
    while (times--) {
      env.status = status;
      env.sp = env.sp0 = sp0; 
      env.rp = env.rp0 = rp0; 
      env.dp = env.dp0 = dp0; 
      env.mp = &mod; 
      env.ip = mod.segment.entry;
      errno = vfm_run(&env);
    }
    gettimeofday(&stop, NULL);
    printf("%5.f ms\n", 
	   ((stop.tv_sec - start.tv_sec) * 1000.0) + 
	   ((stop.tv_usec - start.tv_usec)/ 1000.0));
  } else {
    env.status = status;
    env.sp = env.sp0 = sp0; 
    env.rp = env.rp0 = rp0; 
    env.dp = env.dp0 = dp0; 
    env.mp = &mod; 
    env.ip = mod.segment.entry;
    errno = vfm_run(&env);
  }
  if (profile) vfm_profile(stdout, &mod);
  if (coverage) vfm_coverage(stdout, &mod);

  return (errno);
}
