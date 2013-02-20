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
#include <stdlib.h>

// TODO: Options for add, remove and replace objects
// TODO: Allow multiple versions of the same module in archive

#define MOD_MAX 256

int main(int argc, char* argv[])
{
  FILE* infile;
  FILE* outfile;
  vfm_map_t map[MOD_MAX];
  vfm_arc_t arc;
  vfm_mod_t mod[MOD_MAX];
  char filename[FILENAME_MAX];
  char* archive;
  char* object;
  int debug = 1;
  int recursive = 0;
  int source = 0;
  int objects = 0;
  int opterr = 0;
  int listing = 0;
  int symbols = 0;
  int pos;
  int c;
  int i;
  int j;

  // Check options
  while ((c = getopt(argc, argv, "clrs")) != EOF)
    switch (c) {
    case 'c':
      source = 1;
      break;
    case 'l':
      listing = 1;
      break;
    case 'r':
      recursive = 1;
      symbols = 1;
      break;
    case 's':
      symbols = 1;
      break;
    case '?':
    default:
      opterr = 1;
    }

  // Check parameters
  if (optind == argc || opterr) {
    fprintf(stderr, "usage: vfa [-clrs] archive object...\n");
    fprintf(stderr, "vfm object code archiver\n");
    fprintf(stderr, "  -c	generate c source code, file.i\n");
    fprintf(stderr, "  -l	list archive object modules\n");
    fprintf(stderr, "  -r	list all symbols for object file(s)\n");
    fprintf(stderr, "  -s	list symbols for object file(s)\n");
    return (-1);
  }

  // Check for archive file name and number of object files
  archive = argv[optind++];
  objects = argc - optind;
  if (objects > MOD_MAX) {
    fprintf(stderr, "error: number of files exceeded (max %d)\n", MOD_MAX);
    return (-1);
  }

  // Check for archive listing
  if (listing) {
    if (objects) fprintf(stderr, "warning: arguments ignored\n");
    infile = vfm_fopen_arc_file(archive);
    if (!infile || vfm_arc_map_load(infile, &arc)) {
      fprintf(stderr, "%s: error: unknown or illegal archive file\n", archive);
      return (-1);
    }
    for (i = 0; i < arc.count; i++) {
      printf("%7d %5d %.19s %s %s %s\n", 
	     arc.map[i].pos, 
	     arc.map[i].size, 
	     ctime(&arc.map[i].timestamp),
	     arc.map[i].name,
	     arc.map[i].ident,
	     arc.map[i].version);
    }
    fclose(infile);
    return (0);
  }

  // Check for parameters
  if (objects <= 0) return  (0);

  // Check for compile source or symbol listing
  if (source || symbols) {
    infile = vfm_fopen_arc_file(archive);
    if (!infile || vfm_arc_map_load(infile, &arc)) {
      fprintf(stderr, "%s: error: unknown or illegal archive file\n", archive);
      return (-1);
    }
    for (i = optind; i < argc; i++) {
      object = argv[i];
      if (vfm_arc_load(infile, object, debug, &mod[0], &arc)) {
	fprintf(stderr, "%s: not in archive file\n", object);
	return (-1);
      }
      if (source) vfm_gencode(stdout, &mod[0]);
      if (symbols) vfm_dump_module(stdout, recursive, &mod[0]);
    }
    return (0);
  }

  // Normal mode: Load object files and collect file size
  arc.size = arc.count = objects;
  arc.map = map;
  for (j = 0, i = optind; i < argc; i++, j++) {
    object = argv[i];
    infile = vfm_fopen_obj_file(object);
    if (!infile || vfm_load(infile, 1, &mod[j])) {
      fprintf(stderr, "%s: error: unknown or illegal object file\n", object);
      return (-1);
    }
    fseek(infile, 0, SEEK_END);
    arc.map[j].size = ftell(infile);
    arc.map[j].name = mod[j].name;
    arc.map[j].timestamp = mod[j].timestamp;
    arc.map[j].ident = mod[j].ident;
    arc.map[j].version = mod[j].version;
    fclose(infile);
  }

  // Generate relative position in archive 
  pos = 0;
  for (i = 0; i < objects; i++) {
    arc.map[i].pos = pos;
    pos = pos + arc.map[i].size;
  }

  // Write archive header
  sprintf(filename, "%s.vfa", archive);
  outfile = fopen(filename, "w");
  if (!outfile) {
    fprintf(stderr, "%s: error: could not create archive file\n", filename);
    return (-1);
  }
  fputs(VFM_LIB_MAGIC, outfile);
  fputint(objects, outfile);
  for (i = 0; i < objects; i++) {
    fputstr(arc.map[i].name, outfile);
    fputstr(arc.map[i].ident, outfile);
    fputstr(arc.map[i].version, outfile);
    fputint(arc.map[i].timestamp, outfile);
    fputint(arc.map[i].size, outfile);
    fputint(arc.map[i].pos, outfile);
  }
  arc.pos = ftell(outfile);

  // Store object files in sequence - the simple solution
  for (i = 0; i < objects; i++) {
    vfm_store(outfile, &mod[i]);
  }

  return (0);
}
