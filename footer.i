#include <stdio.h>
#include <time.h>

// Pointer to next (normal, trace and/or tracing)

#define VFM_USE_NEXT_POINTER

// Inline the next (when pointer to next is undefined only)

#define VFM_USE_INLINE_NEXT

// Magic strings for object and library files

#define VFM_OBJ_MAGIC "!vfm:token:obj:0.1\n"
#define VFM_LIB_MAGIC "!vfm:token:lib:0.1\n"

// Data and code definition

typedef long int vfm_data_t;
typedef long long int vfm_data2_t;
typedef char vfm_code_t;

// TODO: Structure mode and introduce run-time entry, local, global

#define VFM_NORMAL_MODE 0
#define VFM_IMMEDIATE_MODE 1
#define VFM_EXECUTION_MODE 2
#define VFM_COMPILATION_MODE 4
#define VFM_HIDDEN_MODE 6
#define VFM_PRIVATE_MODE 8

// NB: Order dependent in code generation (compiler.c::vfm_gencode)

typedef struct vfm_symb_t {
  char* name;
  vfm_code_t* code;
  int mode;
  int refcnt;
} vfm_symb_t;

typedef struct vfm_dict_t {
  int count;
  int size;
  vfm_symb_t* symbols;
} vfm_dict_t;

typedef struct vfm_segm_t {
  int count;
  int size;
  vfm_code_t* code;
  vfm_code_t* entry;
} vfm_segm_t;

typedef struct vfm_mod_t vfm_mod_t;

typedef struct vfm_use_t {
  int count;
  int size;
  vfm_mod_t** mod;
} vfm_use_t;

struct vfm_mod_t {
  char* name;
  char* ident;
  char* version;
  time_t timestamp;
  vfm_use_t use;
  vfm_dict_t dict;
  vfm_segm_t segment;
};

typedef struct vfm_map_t {
  char* name;
  char* ident;
  char* version;
  time_t timestamp;
  int size;
  int pos;
} vfm_map_t;

typedef struct vfm_arc_t {
  int count;
  int size;
  int pos;
  vfm_map_t* map;
} vfm_arc_t;

#define VFM_NORMAL_STATUS 0
#define VFM_TRACING_STATUS 1
#define VFM_PROFILING_STATUS 2
#define VFM_IO_WAIT_STATUS 4

// TODO: Add double linked queue for threading

typedef struct vfm_env_t {
  int status;
  vfm_mod_t*   mp;
  vfm_data_t*  sp;
  vfm_code_t*  ip;
  vfm_data_t*  dp;
  vfm_code_t** rp;
  vfm_data_t*  sp0;
  vfm_code_t** rp0;
  vfm_data_t*  dp0;
} vfm_env_t;

// NB: Number of byte codes (128 single byte, and three pages double byte)

#define VFM_OPMAX 0x3ff

extern int vfm_errno;
extern void* vfm_optab;
extern char** vfm_opname;
extern int vfm_oprefcnt[];

// TODO: Add vfm_perror for simple print of error message
// TODO: Complete list of error codes

#define VFM_NOERR 0
#define VFM_ERR -1
#define VFM_FILE_ERR -2
#define VFM_MALLOC_ERR -3
#define VFM_MAGIC_ERR -4
#define VFM_MODULE_LIMIT_ERR -5
#define VFM_MODULE_LOOKUP_ERR -6
#define VFM_MODULE_TIMESTAMP_ERR -7
#define VFM_COMPILE_ERR -8
#define VFM_ARC_SEARCH_ERR -9

// Utility functions (file: utility.c)

int fgetint(int* x, FILE* file);
int fputint(int n, FILE* file);
int fgetstr(char* buf, FILE* file);
int fputstr(char* str, FILE* file);

char* vfm_parse_entry(char* name);
char* vfm_parse_module(char* name);

char* vfm_fullname(vfm_mod_t* mod);
char* vfm_name2path(char* name);

vfm_symb_t* vfm_name2symb(char* name, vfm_dict_t *dict);
vfm_symb_t* vfm_addr2symb(vfm_code_t* addr, vfm_dict_t *dict);

int vfm_name2dict(char* name, vfm_dict_t **dict, vfm_mod_t *mod);

int vfm_dump_module(FILE* file, int recursive, vfm_mod_t *mod);
int vfm_lookup_module(char* fullname, vfm_symb_t** symb, vfm_mod_t *mod);

FILE* vfm_fopen_obj_file(char* object);
FILE* vfm_fopen_arc_file(char* archive);

// Compiler functions (file: compiler.c)

int vfm_compile(FILE* file, char* name, char* entry, vfm_mod_t* mod);
int vfm_store(FILE* file, vfm_mod_t *mod);
int vfm_gencode(FILE* file, vfm_mod_t *mod);

// Loader functions (file: loader.c)

int vfm_load(FILE* file, int debug, vfm_mod_t *mod);
int vfm_arc_map_load(FILE* file, vfm_arc_t* arc);
int vfm_arc_load(FILE* file, char* name, int debug, vfm_mod_t *mod, vfm_arc_t* arc);

// Runtime functions (file: runtime.c)

int vfm_init();
int vfm_run(vfm_env_t* env);

// Profiler functions (file: profiler.c)

int vfm_profile(FILE* file, vfm_mod_t *mod);
int vfm_coverage(FILE* file, vfm_mod_t *mod);
int vfm_reset_counters(vfm_mod_t *mod);
