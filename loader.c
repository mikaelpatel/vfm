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
#include <string.h>
#include <stdlib.h>

// TODO: Allow memory management control
// TODO: Better support runtime and compile use cases
// TODO: Enhance loading modes; full (0), object (1) or symbols (-1)

#define LOADED_MAX 256
#define NAME_MAX 256

static int load(FILE* file, int debug, vfm_mod_t *mod, vfm_arc_t *arc)
{
  static vfm_mod_t* loaded[LOADED_MAX] = { 0 };
  vfm_code_t* code;
  vfm_symb_t* symb;
  char name[NAME_MAX];
  int timestamp;
  int count;
  int entry;
  int load;
  int size;
  int offset;
  int mode;
  int res;
  int i;
  int j;

  // Reset error number
  vfm_errno = VFM_NOERR;

  // Read header and check magic string
  
  if (!fgets(name, sizeof(name), file) || strcmp(name, VFM_OBJ_MAGIC)) 
    return (vfm_errno = VFM_MAGIC_ERR);

  // Read module name, version and timestamp
  fgetstr(name, file);
  mod->name = strdup(name);
  fgetstr(name, file);
  mod->ident = strdup(name);
  fgetstr(name, file);
  mod->version = strdup(name);
  fgetint(&timestamp, file);
  mod->timestamp = (time_t) timestamp;

  // Read use module list and load modules when needed
  fgetint(&count, file);
  mod->use.count = mod->use.size = count;
  mod->use.mod = 0;
  if (count > 0) {
    vfm_mod_t** use;
    char filename[FILENAME_MAX];
    FILE* usefile;

    use = (vfm_mod_t**) malloc(sizeof(vfm_mod_t*) * (count + 1));
    if (!use) return (vfm_errno = VFM_MALLOC_ERR);

    // Read module name, check if already loaded
    for (i = 0; i < count; i++) {

      // Read full module name
      fgetstr(name, file);

      // Check if already loaded
      for (j = 0; j < LOADED_MAX && loaded[j]; j++)
	if (!strcmp(loaded[j]->name, name))
	  break;
      if (j == LOADED_MAX) {
	fprintf(stderr, "error: module limit exceeded\n");
	return (vfm_errno = VFM_MODULE_LIMIT_ERR);
      }
      if (loaded[j]) {
	use[i] = loaded[j];
	fgetint(&timestamp, file);
	continue;
      }

      // Load the module data
      loaded[j] = use[i] = (vfm_mod_t*) malloc(sizeof(vfm_mod_t));
      if (!use[i]) return (vfm_errno = VFM_MALLOC_ERR);

      // Check for archive based loading, fallback to file loading
      load = 1;
      if (arc) {
	int pos = ftell(file);
	if (!vfm_arc_load(file, name, debug, use[i], arc)) {
	  load = 0;
	}
	fseek(file, pos, SEEK_SET);
      }

      // Normal file loading. Load module into allocated (loaded[j])
      if (load) {
	sprintf(filename, "%s.vfm", vfm_name2path(name));
	usefile = fopen(filename, "r");
	if (!usefile) {
	  fprintf(stderr, "%s: error: missing module file\n", filename);
	  return (vfm_errno = VFM_MODULE_LOOKUP_ERR);
	}

	// Load module file and cascade errors
	if (vfm_load(usefile, debug, use[i]) != 0) {
	  fclose(usefile);
	  return (vfm_errno);
	}
	fclose(usefile);
      }

      // Check compile timestamp
      fgetint(&timestamp, file);
      if (use[i]->timestamp != timestamp) {
	fprintf(stderr, "%s: error: compile timestamp\n", filename);
	return (vfm_errno = VFM_MODULE_TIMESTAMP_ERR);
      }
    }
    mod->use.mod = use;
  }
  // Read code area with entry and size
  fgetint(&entry, file);
  fgetint(&size, file);
  code = (vfm_code_t*) malloc(sizeof(vfm_code_t) * size);
  if (!code) return (vfm_errno = VFM_MALLOC_ERR);
  res = fread(code, size, 1, file);

  // Setup module segment data
  mod->segment.size = size;
  mod->segment.count = size;
  mod->segment.code = code;
  mod->segment.entry = (entry != 0 ? code + entry : 0);

  // Initiate empty dictionary
  mod->dict.count = 0;
  mod->dict.size = 0;
  mod->dict.symbols = 0;

  // Check for non debug mode and skip symbol table load
  if (!debug) return (0);

  // Read symbol table; count and symbols
  fgetint(&count, file);
  symb = (vfm_symb_t*) malloc(sizeof(vfm_symb_t) * count);
  if (!symb) return (vfm_errno = VFM_MALLOC_ERR);
  mod->dict.count = count;
  mod->dict.size = count;
  mod->dict.symbols = symb;
  for(i = 0; i < count; i++, symb++) {
    fgetstr(name, file);
    fgetint(&offset, file);
    fgetint(&mode, file);
    symb->name = strdup(name);
    symb->code = code + offset;
    symb->mode = mode;
    symb->refcnt = 0;
  }

  return (0);
}

int vfm_load(FILE* file, int debug, vfm_mod_t *mod)
{
  return (load(file, debug, mod, 0));
}

int vfm_arc_map_load(FILE* file, vfm_arc_t* arc)
{
  vfm_map_t* map;
  char name[NAME_MAX];
  char magic[256];
  char ident[64];
  char version[64];
  int timestamp;
  int objects;
  int size;
  int pos;
  int i;

  // Read magic signature and check
  arc->count = 0;
  if (!fgets(magic, sizeof(magic), file) || strcmp(magic, VFM_LIB_MAGIC)) 
    return (vfm_errno = VFM_MAGIC_ERR);

  // Read number of object files and names in archive header
  fgetint(&objects, file);
  arc->size = arc->count = objects;
  map = arc->map = (vfm_map_t*) malloc(sizeof(vfm_map_t) * objects);
  for (i = 0; i < objects; i++) {
    fgetstr(name, file);
    fgetstr(ident, file);
    fgetstr(version, file);
    fgetint(&timestamp, file);
    fgetint(&size, file);
    fgetint(&pos, file);
    map[i].name = strdup(name);
    map[i].ident = strdup(ident);
    map[i].version = strdup(version);
    map[i].timestamp = timestamp;
    map[i].size = size;
    map[i].pos = pos;
  }

  // Save position of first object file
  arc->pos = ftell(file);

  return (vfm_errno = VFM_NOERR);
}

int vfm_arc_load(FILE* file, char* name, int debug, vfm_mod_t *mod, vfm_arc_t* arc)
{
  int i;

  // Check that the archive header map is loaded
  if (!arc->map && vfm_arc_map_load(file, arc))
    return (vfm_errno);

  // Search through map and if found load object code
  for (i = 0; i < arc->count; i++) 
    if (!strcmp(name, arc->map[i].name)) {
      fseek(file, arc->pos + arc->map[i].pos, SEEK_SET);
      load(file, debug, mod, arc);
      return (vfm_errno);
    }

  // Opps! Not found in archive
  return (vfm_errno = VFM_ARC_SEARCH_ERR);
}

