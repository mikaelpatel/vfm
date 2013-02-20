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
#include <limits.h>

// Scanner token table

enum token_t {
  COMMENT_TOKEN,
  LINE_TOKEN,
  PACKAGE_TOKEN,
  VERSION_TOKEN,
  IDENT_TOKEN,
  MODULE_TOKEN,
  USE_TOKEN,
  END_MODULE_TOKEN,
  CREATE_TOKEN,
  VARIABLE_TOKEN,
  CONSTANT_TOKEN,
  ALLOT_TOKEN,
  COMMA_TOKEN,
  C_COMMA_TOKEN,
  FUNCTION_TOKEN,
  END_FUNCTION_TOKEN,
  GUARD_TOKEN,
  QUOTE_TOKEN,
  START_COMPILE_TOKEN,
  END_COMPILE_TOKEN,
  RECURSE_TOKEN,
  TAIL_RECURSE_TOKEN,
  IF_TOKEN,
  NOT_IF_TOKEN,
  ELSE_TOKEN,
  THEN_TOKEN,
  BEGIN_TOKEN,
  AGAIN_TOKEN,
  WHILE_TOKEN,
  REPEAT_TOKEN,
  UNTIL_TOKEN,
  FOR_TOKEN,
  NEXT_TOKEN,
  DEC_NEXT_TOKEN,
  DO_TOKEN,
  LOOP_TOKEN,
  INC_LOOP_TOKEN,
  CASE_TOKEN,
  END_CASE_TOKEN,
  OF_TOKEN,
  RANGE_OF_TOKEN,
  END_OF_TOKEN,
  SELECT_TOKEN,
  END_SELECT_TOKEN,
  STRING_TOKEN,
  IDENTIFIER_TOKEN,
  LITERAL_TOKEN,
  KERNEL_TOKEN
};

// NB: Map operation codes (see vfm_opcode.h) onto tokens

#define TOKEN(x) KERNEL_TOKEN + VFM_OP_ ## x

// NB: Modes (tokentab_t.mode) are 
// NB:   not function compile(0), function compile(1), and both(-1)
// NB: The trick is to avoid compile time execution

typedef struct tokentab_t {
  char* string;
  int token;
  int mode;
} tokentab_t;

// TODO: Should use vfm_dict_t and vfm_symb_t instead
// TODO: Possible need for alignment operation (align)
// TODO: Classical create-does> not implemented yet
// TODO: Additional input buffer, output (.") and symbol lookup operators

tokentab_t tokentab[] = {
  { "(", COMMENT_TOKEN, 0 },
  { "//", LINE_TOKEN, 0 },
  { "\\", LINE_TOKEN, 0 },
  { "package", PACKAGE_TOKEN, 0 },
  { "module", MODULE_TOKEN, 0 },
  { "version", VERSION_TOKEN, -1 },
  { "ident", IDENT_TOKEN, -1 },
  { "use", USE_TOKEN, 0 },
  { "endmodule", END_MODULE_TOKEN, 0 },
  { "create", CREATE_TOKEN, 0 },
  { "variable", VARIABLE_TOKEN, 0 },
  { "constant", CONSTANT_TOKEN, 0 },
  { "allot", ALLOT_TOKEN, -1 },
  { ":", FUNCTION_TOKEN, 0 },
  { ";", END_FUNCTION_TOKEN, 1 },
  { "recurse", RECURSE_TOKEN, 1 },
  { "tailrecurse", TAIL_RECURSE_TOKEN, 1 },
  { "'", QUOTE_TOKEN, 1 },
  { "]", START_COMPILE_TOKEN, 0 },
  { "[", END_COMPILE_TOKEN, 1 },
  { "guard", GUARD_TOKEN, 1 },
  { "if", IF_TOKEN, 1 },
  { "-if", NOT_IF_TOKEN, 1 },
  { "else", ELSE_TOKEN, 1 },
  { "then", THEN_TOKEN, 1 },
  { "begin", BEGIN_TOKEN, 1 },
  { "again", AGAIN_TOKEN, 1 },
  { "while", WHILE_TOKEN, 1 },
  { "repeat", REPEAT_TOKEN, 1 },
  { "until", UNTIL_TOKEN, 1 },
  { "for", FOR_TOKEN, 1 },
  { "next", NEXT_TOKEN, 1 },
  { "-next", DEC_NEXT_TOKEN, 1 },
  { "do", DO_TOKEN, 1 },
  { "loop", LOOP_TOKEN, 1 },
  { "+loop", INC_LOOP_TOKEN, 1 },
  { "select", SELECT_TOKEN, 1 },
  { "endselect", END_SELECT_TOKEN, 1 },
  { "case", CASE_TOKEN, 1 },
  { "of", OF_TOKEN, 1 },
  { "rangeof", RANGE_OF_TOKEN, 1 },
  { "endof", END_OF_TOKEN, 1 },
  { "endcase", END_CASE_TOKEN, 1 },
  { "\"", STRING_TOKEN, -1 },
  { "nop", TOKEN(NEXT), 1 },
  { "chain", TOKEN(BRAX), 1 },
  { "trace", TOKEN(TRACE), 1 },
  { "profile", TOKEN(PROFILE), 1 },
  { "execute", TOKEN(EXEC), 1 },
  { "exit", TOKEN(UNNEST), 1 },
  { "?exit", TOKEN(UNNEZE), 1 },
  { "i", TOKEN(RCOPY), 1 },
  { "task", TOKEN(TASK), 1 },
  { "here", TOKEN(HERE), 1 },
  { "c@", TOKEN(CLOAD), 1 },
  { "c!", TOKEN(CSTORE), 1 },
  { "@", TOKEN(LOAD), 1 },
  { "!", TOKEN(STORE), 1 },
  { "+c@", TOKEN(ICLOAD), 1 },
  { "+c!", TOKEN(ICSTORE), 1 },
  { "+!", TOKEN(ISTORE), 1 },
  { "+@", TOKEN(ILOAD), 1 },
  { ">r", TOKEN(RPUSH), 1 },
  { "dup>r", TOKEN(RDUP), 1 },
  { "r>", TOKEN(RPOP), 1 },
  { "r@", TOKEN(RCOPY), 1 },
  { "depth", TOKEN(DEPTH), 1 },
  { "drop", TOKEN(DROP), 1 },
  { "nip", TOKEN(NIP), 1 },
  { "empty", TOKEN(EMPTY), 1 },
  { "dup", TOKEN(DUP), 1 },
  { "?dup", TOKEN(DUPNZ), 1 },
  { "over", TOKEN(OVER), 1 },
  { "tuck", TOKEN(TUCK), 1 },
  { "pick", TOKEN(PICK), 1 },
  { "swap", TOKEN(SWAP), 1 },
  { "rot", TOKEN(ROT), 1 },
  { "-rot", TOKEN(TOR), 1 },
  { "roll", TOKEN(ROLL), 1 },
  { "cell", TOKEN(CELL), 1 },
  { "-2", TOKEN(CONSTN2), 1 },
  { "-1", TOKEN(CONSTN1), 1 },
  { "0", TOKEN(CONST0), 1 },
  { "1", TOKEN(CONST1), 1 },
  { "2", TOKEN(CONST2), 1 },
  { "3", TOKEN(CONST3), 1 },
  { "true", TOKEN(TRUE), 1 },
  { "false", TOKEN(FALSE), 1 },
  { "not", TOKEN(NOT), 1 },
  { "and", TOKEN(AND), 1 },
  { "or", TOKEN(OR), 1 },
  { "xor", TOKEN(XOR), 1 },
  { "negate", TOKEN(NEG), 1 },
  { "1+", TOKEN(INC), 1 },
  { "1-", TOKEN(DEC), 1 },
  { "2+", TOKEN(INC2), 1 },
  { "2-", TOKEN(DEC2), 1 },
  { "2*", TOKEN(MUL2), 1 },
  { "2/", TOKEN(DIV2), 1 },
  { "+", TOKEN(ADD), 1 },
  { "-", TOKEN(SUB), 1 },
  { "*", TOKEN(MUL), 1 },
  { "*/", TOKEN(MULDIV), 1 },
  { "/", TOKEN(DIV), 1 },
  { "%", TOKEN(REM), 1 },
  { "/%", TOKEN(DIVREM), 1 },
  { "<<", TOKEN(LSH), 1 },
  { ">>", TOKEN(RSH), 1 },
  { "0<>", TOKEN(ZNE), 1 },
  { "0<", TOKEN(ZLT), 1 },
  { "0<=", TOKEN(ZLE), 1 },
  { "0=", TOKEN(ZEQ), 1 },
  { "0>=", TOKEN(ZGE), 1 },
  { "0>", TOKEN(ZGT), 1 },
  { "!=", TOKEN(NE), 1 },
  { "<", TOKEN(LT), 1 },
  { "<=", TOKEN(LE), 1 },
  { "==", TOKEN(EQ), 1 },
  { ">=", TOKEN(GE), 1 },
  { ">", TOKEN(GT), 1 },
  { "within", TOKEN(WITHIN), 1 },
  { "abs", TOKEN(ABS), 1 },
  { "min", TOKEN(MIN), 1 },
  { "max", TOKEN(MAX), 1 },
  { ".s", TOKEN(DUMP), 1 },
  { ".", TOKEN(PUTI), 1 },
  { "putc", TOKEN(PUTC), 1 },
  { "puti", TOKEN(PUTI), 1 },
  { "putx", TOKEN(PUTX), 1 },
  { "puts", TOKEN(PUTS), 1 },
  { "cr", TOKEN(CR), 1 },
  { "getc", TOKEN(GETC), 1 },
  { "gets", TOKEN(GETS), 1 },
  { "ext0", TOKEN(EXT0), 1 },
  { "halt", TOKEN(HALT), 1 },
  { 0, IDENTIFIER_TOKEN, 0 },
};

static int line_nr = 0;

static int scan(FILE* file, char* string, int mode, tokentab_t* tab)
{
  tokentab_t* tp;
  char* sp;
  int c;

  while (1) {
    sp = string;
    *sp = 0;
    tp = tab;

    // Skip whitespace
    while ((c = getc(file)) != EOF && c <= ' ')
      if (c == '\n') line_nr++;
    if (c == EOF) return (EOF);

    // Scan string
    do { 
      *sp++ = c; 
    } while ((c = getc(file)) != EOF && c > ' ');
    *sp++ = 0;
    if (c == '\n') line_nr++;
    if (*string == 0) return (EOF);

    // Lookup string
    while (tp->string && strcmp(tp->string, string)) tp++;

    // Check for special forms; comments, comment lines and string literals
    if (tp->token == COMMENT_TOKEN) {
      while ((c = getc(file)) != EOF && c != ')');
      if (c == EOF) return (EOF);
      continue;
    }
    if (tp->token == LINE_TOKEN) {
      while ((c = getc(file)) != EOF && c != '\n');
      if (c == EOF) return (EOF);
      line_nr++;
      continue;
    }
    if (tp->token == STRING_TOKEN) {
      sp = string;
      while ((c = getc(file)) != EOF && c != '\"') {
	*sp++ = c;
	if (c == '\n')
	  line_nr++;
      }
      *sp = 0;
      if (c == EOF) return (EOF);
      return (STRING_TOKEN);
    }
    if (tp->string == 0)
      return (tp->token);
    return (tp->mode == -1 || tp->mode == mode ? tp->token : 0);
  };
}

#define USE_MAX 64
#define STATE_MAX 64
#define RESOLVE_MAX 64
#define SYMBOLS_MAX 256
#define CODE_MAX 32 * 1024

#define latest (&symbols[nr_symb - 1])

#define push(d) *++sp = d

#define inc_state() *sp = *sp + 1;

#define push_state(t) *++sp = t

#define pop_state(t) \
  if (*sp-- != t) { \
    error("illegal control structure");	\
    return (vfm_errno = VFM_COMPILE_ERR); \
  }

#define check_state() \
  if (sp != state) { \
    error("illegal control structure");	\
    return (vfm_errno = VFM_COMPILE_ERR); \
  }

#define is_state(t) (*sp == t)

#define mark(p) *++rp = p

#define mark_backward() *++rp = dp

#define mark_forward() \
  *++rp = dp; \
  *dp++ = 0 

#define mark_swap() \
  tp = *rp; \
  *rp = *(rp - 1); \
  *(rp - 1) = tp

#define resolve_forward() \
  gen = (dp - *rp - 1); \
  *(*rp--) = gen

#define resolve_backward() \
  gen = (*rp - dp - 1); \
  rp -= 1; \
  *dp++ = gen

#define gen_char(c) *dp++ = c

#define gen_allot(n) for (i = 0; i < n; i++) gen_char(0);

#define gen_data(n) \
  *dp++ = (vfm_code_t) (n >> 24); \
  *dp++ = (vfm_code_t) (n >> 16); \
  *dp++ = (vfm_code_t) (n >> 8); \
  *dp++ = (vfm_code_t) n

#define gen_symbol(s) \
  if (nr_symb > 255) { \
    error("module symbol limit exceeded"); \
  } \
  *dp++ = nr_symb; \
  symbols[nr_symb].name = strdup(s); \
  symbols[nr_symb].code = dp; \
  symbols[nr_symb].mode = 0; \
  symbols[nr_symb].refcnt = 0; \
  mod->dict.count += 1;	      \
  nr_symb += 1; 
  
#define gen_op(op) \
  gen = op - KERNEL_TOKEN; \
  if (gen > 127) { \
    error("extended operations is not yet implemented"); \
  } \
  *dp++ = gen ; \
  vfm_oprefcnt[gen] += 1;

#define gen_code(op) \
  if (is_state(SELECT_TOKEN)) { \
    error("primitive operation in select block is not allowed"); \
  } \
  gen = *dp++ = VFM_OP_ ## op; \
  vfm_oprefcnt[gen] += 1;

#define gen_clit(n) \
  gen_code(CLIT); \
  *dp++ = (vfm_code_t) n

#define gen_lit(n) \
  gen_code(LIT); \
  *dp++ = (vfm_code_t) (n >> 24); \
  *dp++ = (vfm_code_t) (n >> 16); \
  *dp++ = (vfm_code_t) (n >> 8); \
  *dp++ = (vfm_code_t) n

#define gen_slit(s) \
  if (mode) { \
    gen_code(SLIT); \
    gen = strlen(s) + 1; \
    if (gen > 127) { \
      error("literal string exceeds length limit (127)"); \
    } \
    *dp++ = (vfm_code_t) gen; \
  } \
  strcpy(dp, s); \
  dp = dp + strlen(s) + 1;

#define gen_call(s) \
  gen = ((s->code - dp) - 2);	  \
  *dp++ = (vfm_code_t) (gen >> 8); \
  *dp++ = (vfm_code_t) (gen & 0xff); \
  s->refcnt += 1; \
  vfm_oprefcnt[VFM_OP_NEST] += 1

#define gen_module_call(m,s) \
  if (is_state(SELECT_TOKEN)) { \
    error("module call in select block is not allowed"); \
  } \
  gen_code(MEST); \
  *dp++ = m; \
  gen = s->code - use_ref[m]->segment.code; \
  *dp++ = (vfm_code_t) (gen >> 8); \
  *dp++ = (vfm_code_t) (gen & 0xff); \
  gen_code(UNMEST); \
  s->refcnt += 1 


// TODO: Consider path optimization; bra-*-unnest, bne-*-unnest
// TODO: Add constant and allocation parameters on stack

#define error(msg) \
  fprintf(stderr, "%s:%d: error: %s\n", filename, line_nr, msg)

#define warning(msg) \
  fprintf(stderr, "%s:%d: warning: %s\n", filename, line_nr, msg)

#define check(expected,msg) \
  if (token != expected) { \
    error(msg); \
    return (vfm_errno = VFM_COMPILE_ERR); \
  }

#define match(expected,msg) \
  token = scan(file, string, mode, tokentab); \
  if (token != expected) { \
    error(msg); \
    return (vfm_errno = VFM_COMPILE_ERR); \
  }

int vfm_compile(FILE* file, char* filename, char* entry, vfm_mod_t* mod)
{
  FILE* usef;
  char string[1024];
  char tmp[1024];
  int mode = 0; 
  int token = 0;
  int state[STATE_MAX];
  int* sp = state;
  int value = 0;
  int param = 0;

// TODO: Should reconsider static data and use the heap

  static vfm_mod_t use_mod[USE_MAX];
  static vfm_mod_t* use_ref[USE_MAX];
  static vfm_symb_t symbols[SYMBOLS_MAX];

  int used[USE_MAX];
  int nr_use = 0;
  vfm_symb_t* symb;
  int nr_symb = 0;
  vfm_code_t code[CODE_MAX];
  vfm_code_t* dp = code; 
  vfm_code_t* resolve[RESOLVE_MAX];
  vfm_code_t** rp = resolve;
  vfm_code_t* tp;
  int gen;
  int i; 

  // Initiate error number
  vfm_errno = VFM_NOERR;

  // Initiate code generator structure
  for (i = 0; i < USE_MAX; i++)
    used[i] = 0;
  for (i = 0; i < VFM_OP_HALT + 1; i++)
    vfm_oprefcnt[i] = 0;
  mod->ident = "";
  mod->version = "";
  mod->use.mod = use_ref;
  mod->use.count = 0;
  mod->use.size = USE_MAX;
  mod->dict.symbols = symbols; 
  mod->dict.size = sizeof(symbols) / sizeof(vfm_symb_t); 
  mod->dict.count = 0;
  mod->segment.code = code;
  mod->segment.entry = 0;
  mod->segment.count = 0;
  mod->segment.size = CODE_MAX;
  tmp[0] = 0;

  // Initiate run-time tables for operation coding
  vfm_init();

  // Scan for package name
  line_nr = 1;
  token = scan(file, string, mode, tokentab);
  if (token == PACKAGE_TOKEN) {
    match(IDENTIFIER_TOKEN, "package name expected");
    sprintf(tmp, "%s.", string);
    token = scan(file, string, mode, tokentab);
  }
  // Check for module name. Create full name with package prefix
  check(MODULE_TOKEN, "module expected");
  match(IDENTIFIER_TOKEN, "module name expected");
  strcat(tmp, string);
  mod->name = strdup(tmp);
  sprintf(tmp, "%s.fpp", string);
  if (strcmp(tmp, filename)) {
    error("file and module name do not match");
    return (vfm_errno = VFM_COMPILE_ERR);
  }

  // Set compile time timestamp
  mod->timestamp = time(NULL);

  // Check for ident string
  token = scan(file, string, mode, tokentab);
  if (token == IDENT_TOKEN) {
    match(STRING_TOKEN, "ident string expected");
    mod->ident = strdup(string);
    token = scan(file, string, mode, tokentab);
  }

  // Check for version string
  if (token == VERSION_TOKEN) {
    match(STRING_TOKEN, "version string expected");
    mod->version = strdup(string);
    token = scan(file, string, mode, tokentab);
  }

  // Scan for use statements
  nr_use = 0;
  while (token == USE_TOKEN) {
    match(IDENTIFIER_TOKEN, "module name expected");
    if (nr_use == USE_MAX) {
      error("too many use statements");
      return (vfm_errno = VFM_COMPILE_ERR);
    }
    // Load modules and add to search table
    use_ref[nr_use] = &use_mod[nr_use];
    use_mod[nr_use].name = strdup(string);
    sprintf(tmp, "%s.vfm", vfm_name2path(string));
    usef = fopen(tmp, "r");
    if (!usef) {
      sprintf(string, "unknown module file '%s'", tmp);
      error(string);
      return (vfm_errno = VFM_COMPILE_ERR);
    }
    if (vfm_load(usef, 1, use_ref[nr_use]) != 0)
      return (vfm_errno);
    fclose(usef);
    nr_use += 1;
    mod->use.count = nr_use;

    // Scan next token for possible additional use statements
    token = scan(file, string, mode, tokentab);
  }
  if (token == EOF) {
    error("missing module body");
    return (vfm_errno = VFM_COMPILE_ERR);
  }

  // Compile module body; functions
  mode = 0;
  param = 0;
  do {
    switch (token) {
    case IDENT_TOKEN:
      if (mode) {
	gen_code(IDENT);
      } else
	error("illegal ident");
      break;
    case VERSION_TOKEN:
      if (mode) {
	gen_code(VERSION);
      } else
	error("illegal version");
      break;
    case ALLOT_TOKEN:
      if (mode) {
	gen_code(ALLOT);
      } else {
	pop_state(LITERAL_TOKEN);
	gen_allot(param);
      }
      break;
    case FUNCTION_TOKEN:
      match(IDENTIFIER_TOKEN, "function name expected");
      mode = 1;
      gen_symbol(string);
      break;
    case END_FUNCTION_TOKEN:
      check_state();
      mode = 0;
      gen_code(UNNEST);
      break;
    case RECURSE_TOKEN:
      gen_call(latest);
      break;
    case TAIL_RECURSE_TOKEN:
      mark(latest->code);
      gen_code(BRA);
      resolve_backward();
      break;
    case QUOTE_TOKEN:
      match(IDENTIFIER_TOKEN, "identifier expected");
      gen_code(PLIT);
      symb = vfm_name2symb(string, &mod->dict);
      if (!symb) {
	sprintf(tmp, "%s: undefined", string);
	error(tmp);
	return (vfm_errno = VFM_COMPILE_ERR);
      }
      gen_call(symb);
      break;
    case START_COMPILE_TOKEN:
      mode = 1;
      break;
    case END_COMPILE_TOKEN:
      mode = 0;
      break;
    case CREATE_TOKEN:
      match(IDENTIFIER_TOKEN, "identifier expected");
      gen_symbol(string);
      gen_code(UNSLIT);
      break;
    case VARIABLE_TOKEN:
      match(IDENTIFIER_TOKEN, "identifier expected");
      gen_symbol(string);
      gen_code(UNSLIT);
      gen_allot(sizeof(vfm_data_t));
      break;
    case CONSTANT_TOKEN:
      match(IDENTIFIER_TOKEN, "identifier expected");
      pop_state(LITERAL_TOKEN);
      gen_symbol(string);
      gen_code(UNLIT);
      gen_data(param);
      break;
    case GUARD_TOKEN:
      symb = latest;
      mod->dict.count -= 1;
      symb = vfm_name2symb(symb->name, &mod->dict);
      mod->dict.count += 1;
      if (!symb) {
	sprintf(tmp, "%s: undefined, illegal guard statement", latest->name);
	error(tmp);
	return (vfm_errno = VFM_COMPILE_ERR);
      }
      gen_code(BRZX);
      gen_call(symb);
      break;
    case IF_TOKEN:
      push_state(THEN_TOKEN);
      gen_code(BRZE);
      mark_forward();
      break;
    case NOT_IF_TOKEN:
      push_state(THEN_TOKEN);
      gen_code(BRZN);
      mark_forward();
      break;
    case ELSE_TOKEN:
      pop_state(THEN_TOKEN);
      push_state(THEN_TOKEN);
      gen_code(BRA);
      mark_forward();
      mark_swap();
      resolve_forward();
      break;
    case THEN_TOKEN:
      pop_state(THEN_TOKEN);
      resolve_forward();
      break;
    case BEGIN_TOKEN:
      push_state(BEGIN_TOKEN);
      mark_backward();
      break;
    case AGAIN_TOKEN:
      pop_state(BEGIN_TOKEN);
      gen_code(BRA);
      resolve_backward();
      break;
    case WHILE_TOKEN:
      pop_state(BEGIN_TOKEN);
      push_state(WHILE_TOKEN);
      gen_code(BRZE);
      mark_forward();
      mark_swap();
      break;
    case REPEAT_TOKEN:
      pop_state(WHILE_TOKEN);
      gen_code(BRA);
      resolve_backward();
      resolve_forward();
      break;
    case UNTIL_TOKEN:
      pop_state(BEGIN_TOKEN);
      gen_code(BRZN);
      resolve_backward();
      break;
    case FOR_TOKEN:
      push_state(FOR_TOKEN);
      gen_code(RPUSH);
      mark_backward();
      break;
    case NEXT_TOKEN:
      pop_state(FOR_TOKEN);
      gen_code(RBZN);
      resolve_backward();
      break;
    case DEC_NEXT_TOKEN:
      pop_state(FOR_TOKEN);
      gen_code(RDBG);
      resolve_backward();
      break;
    case DO_TOKEN:
      push_state(DO_TOKEN);
      gen_code(RPUSH);
      gen_code(RPUSH);
      mark_backward();
      break;
    case LOOP_TOKEN:
      pop_state(DO_TOKEN);
      gen_code(RBNE);
      resolve_backward();
      break;
    case INC_LOOP_TOKEN:
      pop_state(DO_TOKEN);
      gen_code(RDNE);
      resolve_backward();
      break;
    case SELECT_TOKEN:
      gen_code(NNEST);
      mark_forward();
      push_state(SELECT_TOKEN);
      break;
    case END_SELECT_TOKEN:
      pop_state(SELECT_TOKEN);
      resolve_forward();
      break;
    case CASE_TOKEN:
      push(0);
      push_state(CASE_TOKEN);
      break;
    case OF_TOKEN:
      pop_state(CASE_TOKEN);
      push_state(OF_TOKEN);
      gen_code(OVER);
      gen_code(EQ);
      gen_code(BRZE);
      mark_forward();
      gen_code(DROP);
      break;
    case RANGE_OF_TOKEN:
      pop_state(CASE_TOKEN);
      push_state(OF_TOKEN);
      gen_code(WITHIN);
      gen_code(BRZE);
      mark_forward();
      gen_code(DROP);
      break;
    case END_OF_TOKEN:
      pop_state(OF_TOKEN);
      inc_state();
      push_state(CASE_TOKEN);
      gen_code(BRA);
      mark_forward();
      mark_swap();
      resolve_forward();
      break;
    case END_CASE_TOKEN:
      pop_state(CASE_TOKEN);
      gen_code(DROP);
      for (i = *sp--; i > 0; i--) {
	resolve_forward();
      }
      break;
    case STRING_TOKEN:
      gen_slit(string);
      break;
    case IDENTIFIER_TOKEN:
      // Check for integer literal
      if (sscanf(string, "%d%s", &value, tmp) == 1 ||
	  sscanf(string, "0x%x%s", &value, tmp) == 1) {
	if (mode) {
	  if (value > SCHAR_MIN && value < SCHAR_MAX) {
	    gen_clit(value);
	  } else {
	    gen_lit(value);
	  }
	} else {
	  push_state(LITERAL_TOKEN);
	  param = value;
	}
	break;
      }
      // Check for symbol
      i = vfm_lookup_module(string, &symb, mod);
      if (i < 0) {
	sprintf(tmp, "%s: undefined", string);
	error(tmp);
	return (vfm_errno = VFM_COMPILE_ERR);
      }
      if (i == 0) {
	gen_call(symb);
      } else {
	i = i - 1;
	used[i] += 1;
	gen_module_call(i,symb);
      }
      break;
    default:
      gen_op(token);
    }
    token = scan(file, string, mode, tokentab);
  } while (token != END_MODULE_TOKEN && token != EOF);

  // Check for end of module
  if (token == EOF) {
    warning("missing end of module");
  } 

  // Check end of file after module end
  match(EOF, "not end of file");

  // Check that defined used modules are used
  for (i = 0; i < nr_use; i++) {
    if (!used[i]) {
      sprintf(string, "module %s defined but not used", use_ref[i]->name);
      warning(string);
    }
  }

  // Set segment size and entry
  mod->segment.count = mod->segment.size = dp - code; 
  symb = vfm_name2symb(entry, &mod->dict);
  if (symb != 0) {
    mod->segment.entry = symb->code;
    symb->refcnt += 1;
  } else {
    mod->segment.entry = 0;
  }

  return (VFM_NOERR);
}

int vfm_store(FILE* file, vfm_mod_t *mod)
{
  vfm_symb_t* symb;
  int entry;
  int i;

  // Calculate entry
  if (mod->segment.entry)
    entry = mod->segment.entry - mod->segment.code;
  else
    entry = 0;

  // Write magic header
  fputs(VFM_OBJ_MAGIC, file);

  // Write module name, package, version, compile time and use module list
  fputstr(mod->name, file);
  fputstr(mod->ident, file);
  fputstr(mod->version, file);
  fputint((int) mod->timestamp, file);
  fputint(mod->use.count, file);
  for(i = 0; i < mod->use.count; i++) {
    fputstr(mod->use.mod[i]->name, file);
    fputint((int) mod->use.mod[i]->timestamp, file);
  }

  // Write code area; relocated entry, size and code area
  fputint(entry, file);
  fputint(mod->segment.size, file);
  fwrite(mod->segment.code, 1, mod->segment.size, file);

  // Write symbol table; count *[name, code, mode]
  fputint(mod->dict.count, file);
  symb = mod->dict.symbols;
  for(i = 0; i < mod->dict.count; i++) {
    fputstr(symb[i].name, file);
    fputint(symb[i].code - mod->segment.code, file);
    fputint(symb[i].mode, file);
  }
  return (VFM_NOERR);
}

#define CODE_PER_LINE 8

static void name2ident(char* sp)
{
  for (;*sp; sp++)
    if (*sp == '.') 
      *sp = '_';
}

int vfm_gencode(FILE* file, vfm_mod_t *mod)
{
  char name[FILENAME_MAX];
  vfm_symb_t* symbols = mod->dict.symbols;
  vfm_code_t* code = mod->segment.code;
  int count = mod->dict.count;
  vfm_code_t* dp = mod->segment.code;
  vfm_code_t* entry = mod->segment.entry;
  int size = mod->segment.size;
  int i;

  strcpy(name, mod->name);
  name2ident(name);
  fprintf(file, "#ifndef _%s_i_\n", name);
  fprintf(file, "#define _%s_i_\n", name);

  // Generate include statements for used modules
  for (i = 0; i < mod->use.count; i++) {
    char filename[FILENAME_MAX];
    strcpy(filename, mod->use.mod[i]->name); 
    vfm_name2path(filename);
    fprintf(file, "#include \"%s.i\"\n", filename); 
  }

  // Generate code area
  fprintf(file, "vfm_code_t %s_code[] = {", name);
  for (i = 0; i < size; i++) {
    if (i % CODE_PER_LINE == 0) 
      fprintf(file, "\n  ");
    fprintf(file, "0x%.2x, ", (int) (*dp++) & 0xff);
  }
  if (i % CODE_PER_LINE == 0) fprintf(file, "\n");
  fprintf(file, "};\n");

  // Generate symbols 
  fprintf(file, "vfm_symb_t %s_symbols[] = {\n", name);
  for(i = 0; i < count; i++) {
    fprintf(file, "  { \"%s\", %s_code + %d, %d, %d },\n", 
	    symbols[i].name, 
	    name, symbols[i].code - code,
	    symbols[i].mode,
	    symbols[i].refcnt);
  }
  fprintf(file, "};\n");

  // Generate used module list
  if (mod->use.count > 0) {
    fprintf(file, "vfm_mod_t* %s_use[] = {\n", name);
    for (i = 0; i < mod->use.count; i++) {
      char usename[FILENAME_MAX];
      strcpy(usename, mod->use.mod[i]->name); 
      name2ident(usename);
      fprintf(file, "  &%s_mod, \n", usename); 
    }
    fprintf(file, "};\n"); 
  }

  // Generate module header
  fprintf(file, "vfm_mod_t %s_mod = {\n", name);
  fprintf(file, "  \"%s\",\n", mod->name);
  fprintf(file, "  \"%s\",\n", mod->ident);
  fprintf(file, "  \"%s\",\n", mod->version);
  fprintf(file, "  %d,\n", (int) mod->timestamp);
  fprintf(file, "  {\n");
  fprintf(file, "    %d,\n", mod->use.count); 
  fprintf(file, "    %d,\n", mod->use.count); 
  if (mod->use.count > 0)
    fprintf(file, "    %s_use, \n", name); 
  else
    fprintf(file, "    0, \n"); 
  fprintf(file, "  },\n");
  fprintf(file, "  {\n");
  fprintf(file, "    %d,\n", mod->dict.count); 
  fprintf(file, "    %d,\n", mod->dict.count); 
  fprintf(file, "    %s_symbols\n", name); 
  fprintf(file, "  },\n");
  fprintf(file, "  {\n");
  fprintf(file, "    %d,\n", size); 
  fprintf(file, "    %d,\n", size); 
  fprintf(file, "    %s_code,\n", name); 
  if (entry)
    fprintf(file, "    %s_code + %d,\n", name, entry - code);
  else
    fprintf(file, "    0,\n");
  fprintf(file, "  }\n");
  fprintf(file, "};\n"); 
  fprintf(file, "#endif\n");

  return (VFM_NOERR);
}
