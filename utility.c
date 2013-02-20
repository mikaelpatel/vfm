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
#include <endian.h>

int fgetint(int* x, FILE* file)
{
  int n;
  int r;

  r = fread(&n, sizeof(n), 1, file);
  *x = be32toh(n);
  return (r);
}

int fputint(int n, FILE* file)
{
  int x = htobe32(n);
  return (fwrite(&x, sizeof(x), 1, file));
}

int fgetstr(char* buf, FILE* file)
{
  char* bp = buf;
  int c;

  while ((c = getc(file)) > 0) *bp++ = c;
  *bp = 0;

  return (*buf != 0);
}

int fputstr(char* str, FILE* file)
{
  return (fwrite(str, strlen(str) + 1, 1, file));
}

char* vfm_parse_entry(char* name)
{
  if (!name) return (0);

  char* tp;

  for (tp = name; *tp != 0; tp++) {
    if (*tp == ':' && *(tp + 1) == ':') {
      *tp = 0;
      tp = tp + 2;
      break;
    }
  }
  return ((*tp == 0) ? 0 : tp);
}

// TODO: Support port to DOS and Windows path delimiter

#define VFM_DIR_CHAR '/'

char* vfm_name2path(char* name)
{
  if (!name) return (0);

  char* tp;

  for (tp = name; *tp != 0; tp++) {
    if (*tp == '.') {
      *tp = VFM_DIR_CHAR;
    }
  }
  return (name);
}

vfm_symb_t* vfm_name2symb(char* name, vfm_dict_t *dict)
{
  if (!dict || !name || !*name) return (0);

  vfm_symb_t *symbol = dict->symbols + dict->count - 1;
  int i;

  for(i = 0; i < dict->count; i++, symbol--)
    if (!strcmp(name, symbol->name))
      return (symbol);

  return (0);
}

int vfm_name2dict(char* name, vfm_dict_t **dict, vfm_mod_t *mod)
{
  int i;

  for (i = 0; i < mod->use.count; i++)
    if (!strcmp(name, mod->use.mod[i]->name)) {
      *dict = &mod->use.mod[i]->dict;
      return (i);
    }
  *dict = 0;
  return (-1);
}

// NB: Performance enhancement
// NB: O(n) => O(log2 n) => O(C)
// #define LINEAR_SEARCH
// #define BINARY_SEARCH
// #define NO_SEARCH
// NB: NO_SEARCH has a compiler dependency (symbol index inline at head)
// NB: And a restriction on the number of symbols (256)

#define NO_SEARCH

vfm_symb_t* vfm_addr2symb(vfm_code_t* addr, vfm_dict_t *dict)
{
  if (!dict) return (0);

  vfm_symb_t *symbol = dict->symbols;

#if defined(NO_SEARCH)
  unsigned i = (unsigned) *(addr - 1);
  if (i > dict->count)
    return (0);
  return (&symbol[i]);

#elif defined(LINEAR_SEARCH)
  int i;

  for(i = 0; i < dict->count; i++, symbol++)
    if (addr == symbol->code)
      return (symbol);
  return (0);

#elif defined(BINARY_SEARCH)
  int low = 0;
  int high = dict->count;
  
  while (low < high) {
    int mid = low + ((high - low) /2);
    if (symbol[mid].code < addr) 
      low = mid + 1;
    else
      high = mid;
  }
  if ((low < dict->count) && (symbol[low].code == addr))
    return (&symbol[low]);
  return (0);
#endif
}

int vfm_dump_module(FILE* file, int recursive, vfm_mod_t *mod)
{
  if (!mod) return (vfm_errno = VFM_ERR);

  vfm_dict_t *dict = &mod->dict;
  vfm_symb_t *symbol = dict->symbols;
  int i;

  for(i = 0; i < dict->count; i++, symbol++) {
    fprintf(file, "%5d %p%s%s::%s\n",
	    symbol->code - mod->segment.code, 
	    symbol->code, (symbol->code == mod->segment.entry) ? "*" : " ",
	    mod->name, symbol->name);
  }

  if (recursive) {
    vfm_use_t *use = &mod->use;
    for (i = 0; i < use->count; i++)
      vfm_dump_module(file, recursive, use->mod[i]);
  }

  return (vfm_errno = VFM_NOERR);
}

int vfm_lookup_module(char* fullname, vfm_symb_t** symb, vfm_mod_t *mod)
{
  char* modulename = fullname;
  char* entryname = vfm_parse_entry(fullname);
  int i;

  // Assume no errors
  vfm_errno = VFM_NOERR;

  // Check for simple entry name without module prefix
  if (!entryname) {
    entryname = fullname;
    *symb = vfm_name2symb(entryname, &mod->dict);
    if (*symb) return (0);

    // Check for symbol in used modules
    for (i = 0; i < mod->use.count; i++) {
      *symb = vfm_name2symb(entryname, &mod->use.mod[i]->dict);
      if (*symb) return (i + 1);
    }
    return (vfm_errno = VFM_ERR);
  }

  // Prefixed entry name: Check module name first
  if (!strcmp(mod->name, modulename)) {
    *symb = vfm_name2symb(entryname, &mod->dict);
    if (*symb != 0) return (0);
  }

  // Locate module in use list
  vfm_dict_t* dict;
  i = vfm_name2dict(modulename, &dict, mod);
  if (i < 0) return (vfm_errno = VFM_ERR);
  *symb = vfm_name2symb(entryname, dict);
  if (*symb) return (i + 1);

  // Not found
  return (vfm_errno = VFM_ERR);
}

// TODO: Add environment path list variable

FILE* vfm_fopen_obj_file(char* object)
{
  char filename[FILENAME_MAX];
  FILE* infile;
  
  strcpy(filename, object);
  infile = fopen(filename, "r");
  if (infile) return (infile);

  sprintf(filename, "%s.vfm", object);
  infile = fopen(filename, "r");
  if (infile) return (infile);

  vfm_name2path(object);
  sprintf(filename, "%s.vfm", object);
  infile = fopen(filename, "r");
  return (infile);
}

FILE* vfm_fopen_arc_file(char* archive)
{
  char filename[FILENAME_MAX];
  FILE* infile;

  sprintf(filename, "lib%s.vfa", archive);
  infile = fopen(filename, "r");
  if (infile) return (infile);

  sprintf(filename, "%s.vfa", archive);
  infile = fopen(filename, "r");
  if (infile) return (infile);

  sprintf(filename, "%s", archive);
  infile = fopen(filename, "r");
  return (infile);
}

