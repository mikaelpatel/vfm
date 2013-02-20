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

int vfm_profile(FILE* file, vfm_mod_t *mod)
{ 
  // Basic parameter check
  if (!file) return (VFM_FILE_ERR);
  if (!mod) return (VFM_ERR);

  // Write usage profile: module, used, kernel
  vfm_mod_t* use;
  int i;
  int j;

  // Write non-zero profile values for symbols in module
  if (mod->dict.symbols) {
    for (i = 0; i < mod->dict.count; i++)
      if (mod->dict.symbols[i].refcnt) {
	fprintf(file, "%8d %s::%s\n", 
		mod->dict.symbols[i].refcnt,
		mod->name, 
		mod->dict.symbols[i].name);
      }
  }

  // Write non-zero profile values for symbols in used modules
  for (i = 0; i < mod->use.count; i++) {
    use = mod->use.mod[i];
    if (use->dict.symbols) {
      for (j = 0; j < use->dict.count; j++)
	if (use->dict.symbols[j].refcnt) {
	  fprintf(file, "%8d %s::%s\n", 
		  use->dict.symbols[j].refcnt,
		  use->name, 
		  use->dict.symbols[j].name);
	}
    }
  }

  // Write non-zero profile values for kernel operations
  for (i = 0; i <= VFM_OP_HALT; i++)
    if (vfm_oprefcnt[i]) {
      fprintf(file, "%8d vfm::%s\n", vfm_oprefcnt[i], vfm_opname[i]);
    }

  return (vfm_errno = VFM_NOERR);
}

int vfm_coverage(FILE* file, vfm_mod_t *mod)
{
  // Basic parameter checking
  if (!file) return (VFM_FILE_ERR);
  if (!mod) return (VFM_ERR);

  // Write coverage: module, used, kernel
  vfm_mod_t* use;
  int total;
  int count;
  int i;
  int j;

  // Collect statistics for symbols in module and write coverage
  if (mod->dict.symbols) {
    total = 0;
    count = 0;
    for (i = 0; i < mod->dict.count; i++)
      if (mod->dict.symbols[i].refcnt) {
	total += mod->dict.symbols[i].refcnt;
	count += 1;
      }
    fprintf(file, "%8d %s %d/%d (%d%%)\n",
	    total, mod->name, count, mod->dict.count,
	    count * 100 / mod->dict.count);
  }

  // Collect statistics for symbols in used modules and write coverage
  for (i = 0; i < mod->use.count; i++) {
    use = mod->use.mod[i];
    if (use->dict.symbols) {
      total = 0;
      count = 0;
      for (j = 0; j < use->dict.count; j++)
	if (use->dict.symbols[j].refcnt) {
	  total += use->dict.symbols[j].refcnt;
	  count += 1;
	}
      fprintf(file, "%8d %s %d/%d (%d%%)\n",
	      total, use->name, count, use->dict.count,
	      count * 100 / use->dict.count);
    }
  }

  // Collect statistics for symbols in kernel and write coverage
  count = 0;
  total = 0;
  for (i = 0; i <= VFM_OP_HALT; i++)
    if (vfm_oprefcnt[i]) {
      total += vfm_oprefcnt[i];
      count += 1;
    }
  fprintf(file, "%8d vfm %d/%d (%d%%)\n", 
	  total, count, VFM_OP_HALT, count * 100 / VFM_OP_HALT);

  return (vfm_errno = VFM_NOERR);
}

int vfm_reset_counters(vfm_mod_t *mod)
{
  // Basic parameter checking
  if (!mod) return (VFM_ERR);

  // Reset all counters
  vfm_mod_t* use;
  int i;

  // Reset counters for symbols in module
  if (mod->dict.symbols)
    for (i = 0; i < mod->dict.count; i++)
      mod->dict.symbols[i].refcnt = 0;

  // Reset counters for symbols in used modules
  for (i = 0; i < mod->use.count; i++) {
    use = mod->use.mod[i];
    if (use->dict.symbols) {
      for (i = 0; i < use->dict.count; i++)
	use->dict.symbols[i].refcnt = 0;
    }
  }

  // Reset counters for kernel operations
  for (i = 0; i <= VFM_OP_HALT; i++)
    vfm_oprefcnt[i] = 0;

  return (vfm_errno = VFM_NOERR);
}

