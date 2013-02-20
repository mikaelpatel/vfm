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

// TODO: Allow stack size as option

#define RETURN_STACK_SIZE 128
#define DATA_STACK_SIZE 256
#define DATA_HEAP_SIZE 32 * 1024

int main(int argc, char* argv[])
{
  FILE* file;
  vfm_env_t env;
  vfm_arc_t arc;
  vfm_mod_t mod;
  vfm_code_t catch[] = { VFM_OP_HALT };
  vfm_code_t* rp0[RETURN_STACK_SIZE] = { catch };
  vfm_data_t sp0[DATA_STACK_SIZE];
  vfm_data_t dp0[DATA_HEAP_SIZE];
  int status = VFM_NORMAL_STATUS;
  char* modulename = 0;
  char* entryname = 0;
  char* archive = 0;
  char* object = 0;
  int benchmark = 0;
  int coverage = 0;
  int profile = 0;
  int debug = 1;
  int recursive = 0;
  int symbols = 0;
  int opterr = 0;
  int times = 1;
  int errno;
  int c;

  // Check options
  while ((c = getopt(argc, argv, "b:cde:l:npsrt")) != EOF)
    switch (c) {
    case 'b':
      benchmark = 1;
      times = atoi(optarg);
      break;
    case 'c':
      coverage = 1;
      status |= VFM_PROFILING_STATUS;
      break;
    case 'd':
      debug = 1;
      break;
    case 'e':
      entryname = optarg;
      break;
    case 'l':
      archive = optarg;
      break;
    case 'n':
      debug = 0;
      break;
    case 'p':
      status |= VFM_PROFILING_STATUS;
      profile = 1;
      break;
    case 'r':
      recursive = 1;
      symbols = 1;
      break;
    case 's':
      symbols = 1;
      break;
    case 't':
      status |= VFM_TRACING_STATUS;
      break;
    case '?':
    default:
      opterr = 1;
    }

  // Check parameters
  if ((!archive && (argc != optind + 1)) || opterr) {
    fprintf(stderr, "usage: vfm [-cdnpt][-b times][-e entry][-l library] object\n");
    fprintf(stderr, "vfm virtual forth machine run-time and dynamic analysis tool\n");
    fprintf(stderr, "  -b 	measure execution, number of times\n");
    fprintf(stderr, "  -c	measure code coverage when profiling\n");
    fprintf(stderr, "  -d	load symbols with module (default)\n");
    fprintf(stderr, "  -e 	start symbol (default main)\n");
    fprintf(stderr, "  -l	load object code files from library\n");
    fprintf(stderr, "  -n	skip loading of symbols\n");
    fprintf(stderr, "  -p	profile execution\n");
    fprintf(stderr, "  -s	dump object symbols\n");
    fprintf(stderr, "  -r	dump all object symbols\n");
    fprintf(stderr, "  -t	trace execution\n");
    return (-1);
  }

  // Check benchmarking parameters
  if (benchmark && times <= 0) {
    fprintf(stderr, "error: illegal benchmark\n");
    return (-1);
  }
  if (!debug && (profile || coverage)) {
    fprintf(stderr, "warning: symbols needed\n");
    debug = 1;
  }

  // Initiate run-time
  vfm_init();
  mod.segment.entry = 0;

  // Check for library loading
  if (archive) {
    if (!entryname) {
      fprintf(stderr, "error: undefined entry\n");
      return (-1);
    }
    if (optind != argc) {
      fprintf(stderr, "warning: parameters ignored\n");
    }
    file = vfm_fopen_arc_file(archive);
    if (!file || vfm_arc_map_load(file, &arc)) {
      fprintf(stderr, "%s: error: unknown or illegal archive file\n", archive);
      return (-1);
    }
    modulename = entryname;
    entryname = vfm_parse_entry(entryname);
    if (vfm_arc_load(file, modulename, debug, &mod, &arc)) {
      fprintf(stderr, "%s: error: failed to load\n", modulename);
      return (-1);
    }
    fclose(file);
  } else {
    object = argv[optind];
    file = vfm_fopen_obj_file(object);
    if (!file || vfm_load(file, debug, &mod)) {
      fprintf(stderr, "%s: error: unknown or illegal object file\n", object);
      return (-1);
    }
    fclose(file);
  }

  // Check for dump of symbols
  if (symbols) {
    if (status || entryname || profile || coverage || benchmark)
      fprintf(stderr, "warning: options ignored\n");
    vfm_dump_module(stdout, recursive, &mod);
    return (0);
  }

  // Check for entry symbol
  if (entryname) {
    vfm_symb_t* symb = vfm_name2symb(entryname, &mod.dict);
    if (!symb) {
      fprintf(stderr, "%s: error: unknown entry\n", entryname);
      return (-1);
    }
    mod.segment.entry = symb->code;
  }
  if (!mod.segment.entry) {
    fprintf(stderr, "error: undefined entry\n");
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
