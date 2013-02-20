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
#include <unistd.h>
#include <libgen.h>
#include <string.h>

// TODO: Add file search path
// TODO: Add option to allow symbol table optimization
// TODO: Add option for library symbol store (eg strip)

int main(int argc, char* argv[])
{
  FILE* infile;
  FILE* outfile;
  char filename[FILENAME_MAX];
  vfm_mod_t mod;
  char* entry = "main";
  int coverage = 0;
  int profile = 0;
  int object = 1;
  int source = 0;
  int opterr = 0;
  int files = 0;
  int c;
  int i;

  // Check options
  while ((c = getopt(argc, argv, "ce:ops")) != EOF)
    switch (c) {
    case 'c':
      coverage = 1;
      break;
    case 'e':
      entry = optarg;
      break;
    case 'o':
      object = 1;
      break;
    case 'p':
      profile = 1;
      break;
    case 's':
      source = 1;
      break;
    case '?':
    default:
      opterr = 1;
    }

  // Check parameters
  if (optind == argc || opterr) {
    fprintf(stderr, "usage: vfc [-cops][-e entry] file\n");
    fprintf(stderr, "vfm compiler and static analysis tool\n");
    fprintf(stderr, "  -c	static code coverage\n");
    fprintf(stderr, "  -e	define entry (default main)\n");
    fprintf(stderr, "  -o	generate object code, package/file.vfm\n");
    fprintf(stderr, "  -p	static code usage profile\n");
    fprintf(stderr, "  -s	generate c source code, package/file.i\n");
    return (-1);
  }

  // Compile source code files
  files = argc - optind;
  for (i = optind; i < argc; i++) {

    // Open file and compile
    strcpy(filename, argv[i]);
    infile = fopen(filename, "r");
    if (!infile) {
      sprintf(filename, "%s.fpp", argv[i]);
      infile = fopen(filename, "r");
      if (!infile) {
	fprintf(stderr, "%s: error: unknown file\n", argv[i]);
	return (-1);
      }
    }
    if (vfm_compile(infile, filename, entry, &mod)) {
      return (-1);
    }
    fclose(infile);

    // Static analysis, add header for multiple files
    if ((files > 1) && (profile || coverage)) 
      fprintf(stdout, "file: %s\n", filename);
    if (profile) 
      vfm_profile(stdout, &mod);
    if (coverage) 
      vfm_coverage(stdout, &mod);

    // Generate source code
    if (source) {
      strcpy(filename, mod.name);
      vfm_name2path(filename);
      strcat(filename, ".i");
      outfile = fopen(filename, "w");
      if (!outfile) {
	fprintf(stderr, "%s: error: could not create source file\n", filename);
	return (-1);
      }
      vfm_gencode(outfile, &mod);
      fclose(outfile);
    } 

    // Generate object code
    if (object) {
      strcpy(filename, mod.name);
      vfm_name2path(filename);
      strcat(filename, ".vfm");
      outfile = fopen(filename, "w");
      if (!outfile) {
	fprintf(stderr, "%s: error: could not create object file\n", filename);
	return (-1);
      }
      vfm_store(outfile, &mod);
      fclose(outfile);
    }
  }
  return (0);
}
