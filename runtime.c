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

#if defined(VFM_USE_NEXT_POINTER)
# define NEXT() goto *np
#elif defined(VFM_USE_INLINE_NEXT)
# define NEXT() if ((ir = *ip++) >= 0) goto *optab[ir]; goto NEXT0
#else 
# define NEXT() goto NEXT
#endif

// NB: Assembly list operation hint

#define OP(n) n: asm("# OP(" # n ")"); 

int vfm_errno = 0;
void* vfm_optab = 0;
char** vfm_opname = 0;
int vfm_oprefcnt[VFM_OPMAX + 1] = { 0 };

// Utility functions

#if defined(VFM_USE_NEXT_POINTER)
static vfm_symb_t* inc_refcnt(vfm_code_t *cp, vfm_dict_t* dict)
{
  vfm_symb_t* symb = vfm_addr2symb(cp, dict);
  if (symb)
    symb->refcnt += 1;
  return (symb);
}

static void ftrace(FILE* file, int depth, vfm_code_t* cp, vfm_mod_t* mp)
{
  vfm_symb_t* symb = vfm_addr2symb(cp, &mp->dict);
  fprintf(file, "R[%d] %s::", depth, mp->name);
  if (symb){
    symb->refcnt += 1;
    fprintf(file, "%s@", symb->name);
  }
  fprintf(file, "%p", cp);
}
#endif

// TODO: Get this done automatically before main if possible

int vfm_init()
{
  return (vfm_run(0));
}

// TODO: Add document block per operation and use a script to extract
// TODO: Extension operation to call C function through table

int vfm_run(vfm_env_t* env) 
{

#include "optab.i"

  if (!env) {
    vfm_optab = optab;
    vfm_opname = opname;
    return (0);
  }

#if defined(VFM_USE_NEXT_POINTER) 
  register void* np = &&NEXT;
#endif
  vfm_data_t* sp = env->sp;
  register vfm_data_t tos = ((env->sp != env->sp0) ? *sp-- : 0);
  vfm_code_t** rp = env->rp;
  vfm_code_t* ip = env->ip;
  vfm_data_t* dp = env->dp;
  vfm_mod_t* mp = env->mp;
  vfm_data_t ir;
  int tmp;

  // Check some basic invariants
  if (ip < mp->segment.code || ip > (mp->segment.code + mp->segment.size)) {
    return (VFM_ERR);
  }

#if defined(VFM_USE_NEXT_POINTER) 
  // Restore correct inner interpreter
  np = (env->status & VFM_TRACING_STATUS) ? &&TRACING : np;
  if (np == &&NEXT && (env->status & VFM_PROFILING_STATUS))
    np = &&PROFILING;
  // Get the profiling data right
  if (np != &&NEXT) {
    vfm_oprefcnt[VFM_OP_NEST] += 1;
    inc_refcnt(ip, &mp->dict);
  }
#endif

  // Let go!
  NEXT();

// NB: EXT0...EXT3 should be opcode (0..3) as opcode is page number
// NB: EXT0 n == n when n < 128. EXT0(VFM_OP_ADD) == VFM_OP_ADD
// NB: EXT0..EXT3 are extension pages for future usage
// NB: Allows 1024 opcodes (primitive instructions)

OP(EXT0)
  ir = (*(ip++) & 0xff);
  goto *optab[ir];

OP(EXT1)
  ir = 0x100 | (*(ip++) & 0xff);
  goto *optab[ir];

OP(EXT2)
  ir = 0x200 | (*(ip++) & 0xff);
  goto *optab[ir];

OP(EXT3)
  ir = 0x300 | (*(ip++) & 0xff);
  goto *optab[ir];

// NB: Positive opcode (0..127) are direct.
// NB: Negative opcode are implicit msb of 16 bit ip relative offset
// NB: Relative to the instruction pointer after reading the offset

OP(NEXT)
  if ((ir = *ip++) >= 0)
    goto *optab[ir];
#if !defined(VFM_USE_NEXT_POINTER)
 NEXT0:
#endif
  ir = ((ir << 8) | (*(ip++) & 0xff));
  *++rp = ip;
  ip = ip + ir;
  goto NEXT;

OP(TRACING)
#if defined(VFM_USE_NEXT_POINTER)
  while ((ir = *ip++) < 0) {
    ir = ((ir << 8) | (*(ip++) & 0xff));
    *++rp = ip;
    ip = ip + ir;
    fprintf(stdout, "%8s ", "NEST");
    ftrace(stdout, rp - env->rp0, ip, mp);
    fprintf(stdout, "\n");
    vfm_oprefcnt[VFM_OP_NEST] += 1;
  }
  fprintf(stdout, "%8s ", opname[(unsigned) ir]);

  // Check for some special trace cases; module call, select call
  if (ir == VFM_OP_MEST) {
    int i = *ip;
    tmp = ((*(ip + 1) << 8) | (*(ip + 2) & 0xff));
    vfm_code_t* tp = mp->use.mod[i]->segment.code + tmp;
    ftrace(stdout, rp - env->rp0, tp, mp->use.mod[i]);
  } else if ((ir == VFM_OP_BRAX) || (ir == VFM_OP_BRZX)) {
    vfm_code_t* tp = ip;
    tmp = *tp++;
    tmp = ((tmp << 8) | ((*tp++) & 0xff));
    tp = tp + tmp;
    ftrace(stdout, rp - env->rp0, tp, mp);
  } else if ((ir == VFM_OP_NNEST) && (tos >= 0 && (tos + tos) < *ip)) {
    vfm_code_t* tp = ip + tos + tos + 1;
    tmp = *tp++;
    tmp = ((tmp << 8) | ((*tp++) & 0xff));
    tp = tp + tmp;
    ftrace(stdout, rp - env->rp0, tp, mp);
  } else if (ir >= VFM_OP_UNNEST && ir <= VFM_OP_UNLIT) {
    fprintf(stdout, "R[%d]", rp - env->rp0);
  } else {
    int i = (sp - env->sp0);
    fprintf(stdout, "S[%d] ", i);
    if (i > 0) {
      vfm_data_t* tp = env->sp0 + 1;
      while (--i) {
        fprintf(stdout, "%d ", (int) *++tp);
      }
      fprintf(stdout, "%d", (int) tos);
    } 
  }
  fprintf(stdout, "\n");
  vfm_oprefcnt[ir] += 1;
  goto *optab[ir];
#else
  goto NEXT;
#endif

OP(PROFILING)
#if defined(VFM_USE_NEXT_POINTER)
  while ((ir = *ip++) < 0) {
    ir = ((ir << 8) | (*(ip++) & 0xff));
    *++rp = ip;
    ip = ip + ir;
    inc_refcnt(ip, &mp->dict);
    vfm_oprefcnt[VFM_OP_NEST] += 1;
  }
  // Check for some special profiling cases; module call, select call
  if (ir == VFM_OP_MEST) {
    int i = *ip;
    tmp = ((*(ip + 1) << 8) | (*(ip + 2) & 0xff));
    vfm_code_t* tp = mp->use.mod[i]->segment.code + tmp;
    inc_refcnt(tp, &mp->use.mod[i]->dict);
  } else if ((ir == VFM_OP_BRAX) || (ir == VFM_OP_BRZX)) {
    vfm_code_t* tp = ip;
    tmp = *tp++;
    tmp = ((tmp << 8) | ((*tp++) & 0xff));
    tp = tp + tmp;
    inc_refcnt(tp, &mp->dict);
  } else if ((ir == VFM_OP_NNEST) && (tos >= 0 && (tos + tos) < *ip)) {
    vfm_code_t* tp = ip + tos + tos + 1;
    tmp = *tp++;
    tmp = ((tmp << 8) | ((*tp++) & 0xff));
    tp = tp + tmp;
    inc_refcnt(tp, &mp->dict);
  }
  vfm_oprefcnt[ir] += 1;
  goto *optab[ir];
#else
  goto NEXT;
#endif

// NB: NEST is an implicit operation in the token threaded inner interpreter

OP(NEST)
  ir = *ip++;
  ir = ((ir << 8) | (*(ip++) & 0xff));
  *++rp = ip;
  ip = ip + ir;
  NEXT();

OP(NNEST)
  ir = *ip++;
  tmp = 2 * tos;
  tos = *sp--;
  if (tmp >= 0 && tmp < ir) {
    *++rp = ip + ir;
    ip = ip + tmp;
    ir = *ip++;
    ir = ((ir << 8) | (*(ip++) & 0xff));
  }
  ip = ip + ir;
  NEXT();

OP(UNNEST)
  ip = *rp--;
  NEXT();

OP(UNNEZE)
  if (tos == 0) {
    tos = *sp--;
    ip = *rp--;
  }
  NEXT();
 
// NB: MEST is a module call that requires module index(int8) and offset(int16)
// TODO: Add full symbolic module call (runtime lookup of symbol)
// TODO: Performance enhance with reference caching (rewriting)

OP(MEST)
  ir = *ip++;
  *(++rp) = (vfm_code_t*) mp;
  mp = mp->use.mod[ir];
  ir = *ip++;
  ir = ((ir << 8) | (*(ip++) & 0xff));
  *(++rp) = ip;
  ip = mp->segment.code + ir;
  NEXT();

// NB: MESTI is a module call that requires module index(int8) and symbol index(int8)
// NB: This requires that symbols are loaded. Should be checked or flagged in object header.

OP(MESTI)
  ir = *ip++;
  *(++rp) = (vfm_code_t*) mp;
  mp = mp->use.mod[ir];
  ir = (unsigned) *ip++;
  *(++rp) = ip;
  ip = mp->dict.symbols[ir].code;
  NEXT();

OP(UNMEST)
  mp = (vfm_mod_t*) *rp--;
  NEXT();

OP(UNMEZT)
  mp = (vfm_mod_t*) *rp--;
  ip = *rp--;
  NEXT();

OP(UNSLIT)
  *++sp = tos;
  tos = (int) ip;
  ip = *rp--;
  NEXT();

OP(UNLIT)
  *++sp = tos;
  tos = (vfm_data_t) *ip++;
  tos = ((tos << 8) | (*(ip++) & 0xff));
  tos = ((tos << 8) | (*(ip++) & 0xff));
  tos = ((tos << 8) | (*(ip++) & 0xff));
  ip = *rp--;
  NEXT();

OP(BRA)
  ir = *ip++;
  ip = ip + ir; 
  NEXT();

OP(BRAX)
  ir = *ip++;
  ir = ((ir << 8) | (*(ip++) & 0xff));
  ip = ip + ir; 
  NEXT();

// NB: Conditional operations are optimized for non stalling pipeline

OP(BRZX)
  ir = *ip++;
  ir = ((ir << 8) | (*(ip++) & 0xff));
  ip = ip + ((-(tos == 0)) & ir);
  tos = *sp--;
  NEXT();

OP(BRZE)
  ir = *ip++;
  ip = ip + ((-(tos == 0)) & ir);
  tos = *sp--;
  NEXT();    

OP(BRZN)
  ir = *ip++;
  ip = ip + ((-(tos != 0)) & ir);
  tos = *sp--;
  NEXT();    

OP(DBZN)
  ir = *ip++;
  if (--tos >= 0)
    ip = ip + ir;
  else
    tos = *sp--;
  NEXT();    

OP(RBZN)
  ir = *ip++;
  *rp = *rp - 1;
  if (((int) *rp) >= 0)
    ip = ip + ir;
  else
    rp = rp - 1;
  NEXT();    

OP(RDBG)
  ir = *ip++;
  *rp = *rp - tos;
  if (((int) *rp) >= 0)
    ip = ip + ir;
  else
    rp = rp - 1;
  tos = *sp--;
  NEXT();    

OP(RBRI)
  if (tos < *sp) {
    *++rp = (vfm_code_t*) tos;
    *++rp = (vfm_code_t*) *sp--;
    tos = *sp--;
  } else {
    sp -= 1;
    tos = *sp++;
    ir = *ip++;
    ip = ip + ir;
  }
  NEXT();

OP(RBNE)
  ir = *ip++;
  *rp = *rp + 1;
  if (*rp <= *(rp - 1))
    ip = ip + ir;
  else
    rp = rp - 2;
  NEXT();    

OP(RDNE)
  ir = *ip++;
  *rp = *rp + tos;
  if (*rp <= *(rp - 1))
    ip = ip + ir;
  else
    rp = rp - 2;
  tos = *sp--;
  NEXT();    

OP(TASK)
  *++sp = tos;
  tos = (vfm_data_t) env;
  NEXT();

OP(LOCAL)
  ir = *ip++;
  ir = ((ir << 8) | (*(ip++) & 0xff));
  *++sp = tos;
  tos = (vfm_data_t) (env->dp0 + (unsigned) ir);
  NEXT();

OP(HERE)
  *++sp = tos;
  tos = (vfm_data_t) dp;
  NEXT();
  
OP(ALLOT)
  dp = dp + tos;
  tos = *sp--;
  NEXT(); 

OP(TRACE)
#if defined(VFM_USE_NEXT_POINTER)
  if (tos) {
    np = &&TRACING;
    env->status |= VFM_TRACING_STATUS;
  } else {
    np = (env->status & VFM_PROFILING_STATUS) ? &&PROFILING : &&NEXT;
    env->status &= ~VFM_TRACING_STATUS;
  }
#endif
  tos = *sp--;
  NEXT();

OP(PROFILE)
#if defined(VFM_USE_NEXT_POINTER)
  if (tos) {
    if (!(env->status & VFM_TRACING_STATUS)) np = &&PROFILING;
    env->status |= VFM_PROFILING_STATUS;
    if (tos == 1)
      vfm_reset_counters(mp);
  } else {
    np = (env->status & VFM_TRACING_STATUS) ? &&TRACING : &&NEXT;
    env->status &= ~VFM_PROFILING_STATUS;
  }
#endif
  tos = *sp--;
  NEXT();

// NB: Problem with passing addresses between modules
// TODO: Fix function pointer with module and offset (as MEST)
// TODO: EXEC has to push current module and apply MEST
// TODO: Compiler 'execute' should compile EXEC and UNMEST
// TODO: Compiler symbol quote should compile module and symbol offset

OP(EXEC)
  *++rp = ip;
  ip = (vfm_code_t*) tos;
  tos = *sp--;
  NEXT();   

OP(CLOAD)
  tos = *((char*) tos);
  NEXT();

OP(CSTORE)
  *((char*) tos) = *sp--;
  tos = *sp--;
  NEXT();

OP(LOAD)
  tos = *((vfm_data_t*) tos);
  NEXT();

OP(STORE)
  *((vfm_data_t*) tos) = *sp--;
  tos = *sp--;
  NEXT();

OP(ICLOAD)
  tos = *((char*) tos + *sp--);
  NEXT();

OP(ICSTORE)
  *((char*) tos) += *sp--;
  tos = *sp--;
  NEXT();

OP(ILOAD)
  tos = *((vfm_data_t*) tos + *sp--);
  NEXT();

OP(ISTORE)
  *((vfm_data_t*) tos) += *sp--;
  tos = *sp--;
  NEXT();

// TODO: Dictionary search and symbol access operations; find
// TODO: Memory block operations; memcpy, memcmp
// TODO: String operations; strcpy, strcmp, strcat
// TODO: Double word operations; 2dup, 2swap
// TODO: Floating point operations; fadd, fsub, fmul, fdiv

OP(RPUSH)
  *++rp = (vfm_code_t*) tos;
  tos = *sp--;
  NEXT();

OP(RDUP)
  *++rp = (vfm_code_t*) tos;
  NEXT();

OP(RPOP)
  *++sp = tos;
  tos = (vfm_data_t) *rp--;
  NEXT();

OP(RCOPY)
  *++sp = tos;
  tos = (vfm_data_t) *rp;
  NEXT();

OP(LIT)
  *++sp = tos; 
  tos = (vfm_data_t) *ip++;
  tos = ((tos << 8) | (*(ip++) & 0xff));
  tos = ((tos << 8) | (*(ip++) & 0xff));
  tos = ((tos << 8) | (*(ip++) & 0xff));
  NEXT();

OP(CLIT)
  *++sp = tos; 
  tos = (vfm_data_t) *ip++;
  NEXT();

OP(PLIT)
  *++sp = tos; 
  ir = *ip++;
  ir = ((ir << 8) | (*(ip++) & 0xff));
  tos = (vfm_data_t) (ip + ir);
  NEXT();

OP(SLIT)
  ir = *ip++;
  *++sp = tos;
  tos = (int) ip;
  ip = ip + (unsigned) ir; 
  NEXT();

OP(DEPTH)
  tmp = (sp - env->sp0);
  *++sp = tos;
  tos = tmp;
  NEXT();

OP(DROP)
  tos = *sp--;
  NEXT();

OP(NIP)
  sp -= 1;
  NEXT();

OP(EMPTY)
  sp = env->sp0;
  NEXT();

OP(DUP)
  *++sp = tos;
  NEXT();

OP(DUPNZ)
  if (tos != 0) 
    *++sp = tos;
  NEXT();

OP(OVER)
  tmp = *sp;
  *++sp = tos;
  tos = tmp;
  NEXT();

OP(TUCK)
  tmp = *sp;
  *sp = tos;
  *++sp = tmp;
  NEXT();

OP(PICK)
  tos = *(sp - tos);
  NEXT();

OP(SWAP)
  tmp = tos;
  tos = *sp;
  *sp = tmp;
  NEXT();

OP(ROT)
  tmp = tos;
  tos = *(sp - 1);
  *(sp - 1) = *sp;
  *sp = tmp;
  NEXT();

OP(TOR)
  tmp = tos;
  tos = *sp;
  *sp = *(sp - 1);
  *(sp - 1) = tmp;
  NEXT();

OP(ROLL)
  if (tos > 0) {
    sp[0] = sp[-tos];
    for (; tos > 0; tos--)
      sp[-tos] = sp[-tos + 1];
  }
  tos = *sp--;
  NEXT();

OP(CELL)
  *++sp = tos;
  tos = sizeof(vfm_data_t);
  NEXT();

OP(CONSTN2)
  *++sp = tos;
  tos = -2;
  NEXT();

OP(CONSTN1)
  *++sp = tos;
  tos = -1;
  NEXT();

OP(CONST0)
  *++sp = tos;
  tos = 0;
  NEXT();

OP(CONST1)
  *++sp = tos;
  tos = 1;
  NEXT();

OP(CONST2)
  *++sp = tos;
  tos = 2;
  NEXT();

OP(CONST3)
  *++sp = tos;
  tos = 3;
  NEXT();

OP(TRUE)
  *++sp = tos;
  tos = -1;
  NEXT();

OP(FALSE)
  *++sp = tos;
  tos = 0;
  NEXT();

OP(NOT)
  tos = ~tos;
  NEXT();

OP(AND)
  tos = *sp-- & tos;
  NEXT();

OP(OR)
  tos = *sp-- | tos;
  NEXT();

OP(XOR)
  tos = *sp-- ^ tos;
  NEXT();

OP(NEG)
  tos = -tos;
  NEXT();
    
OP(INC)
  tos += 1;
  NEXT();

OP(DEC)
  tos -= 1;
  NEXT();

OP(INC2)
  tos += 2;
  NEXT();

OP(DEC2)
  tos -= 2;
  NEXT();

OP(MUL2)
  tos <<= 1;
  NEXT();

OP(DIV2)
  tos >>= 1;
  NEXT();

OP(ADD)
  tos = *sp-- + tos;
  NEXT();

OP(SUB)
  tos = *sp-- - tos;
  NEXT();

OP(MUL)
  tos = *sp-- * tos;
  NEXT();

OP(MULDIV)
  tmp = *sp--;
  tos = (((vfm_data2_t) tos) * (*sp--)) / tmp;
  NEXT();

OP(DIV)
  tos = *sp-- / tos;
  NEXT();

OP(REM)
  tos = *sp-- % tos;
  NEXT();

OP(DIVREM)
  tmp = *sp / tos;
  tos = *sp % tos;
  *sp = tmp;
  NEXT();

OP(LSH)
  tos = *sp-- << tos;
  NEXT();

OP(RSH)
  tos = *sp-- >> tos;
  NEXT();

OP(ZNE)
  tos = -(tos != 0);
  NEXT();

OP(ZLT)
  tos = -(tos < 0);
  NEXT();

OP(ZLE)
  tos = -(tos <= 0);
  NEXT();

OP(ZEQ)
  tos = -(tos == 0);
  NEXT();

OP(ZGE)
  tos = -(tos >= 0);
  NEXT();

OP(ZGT)
  tos = -(tos > 0);
  NEXT();

OP(NE)
  tos = -(*sp-- != tos);
  NEXT();

OP(LT)
  tos = -(*sp-- < tos);
  NEXT();

OP(LE)
  tos = -(*sp-- <= tos);
  NEXT();

OP(EQ)
  tos = -(*sp-- == tos);
  NEXT();

OP(GE)
  tos = -(*sp-- >= tos);
  NEXT();

OP(GT)
  tos = -(*sp-- > tos);
  NEXT();

OP(WITHIN)
  tmp = *sp--;
  tos = -((*sp <= tos) & (*sp >= tmp));
  NEXT();

OP(ABS)
  tos = ((-(tos < 0)) & (-tos)) | ((-(tos >= 0)) & tos);
  NEXT();

OP(MIN)
  tos = ((-(tos < *sp) & tos) | (-(tos >= *sp) & *sp));
  sp = sp - 1;
  NEXT();

OP(MAX)
  tos = ((-(tos > *sp) & tos) | (-(tos <= *sp) & *sp));
  sp = sp - 1;
  NEXT();

OP(DUMP)
  tmp = (sp - env->sp0);
  fprintf(stdout, "[%d] ", (int) tmp);
  if (tmp > 0) {
    vfm_data_t* tp = env->sp0 + 1;
    while (--tmp) {
      fprintf(stdout, "%d ", (int) *++tp);
    }
    fprintf(stdout, "%d", (int) tos);
  }
  fprintf(stdout, "\n");
  NEXT();

// TODO: Memory block read/write
// TODO: Read line from input stream; expect

OP(PUTC)
  putc(tos, stdout);
  tos = *sp--;
  NEXT();

OP(PUTI)
  fprintf(stdout, "%d ", (int) tos);
  tos = *sp--;
  NEXT();

OP(PUTX)
  fprintf(stdout, "0x%x ", (int) tos);
  tos = *sp--;
  NEXT();

OP(PUTS)
  fputs((char*) tos, stdout);
  tos = *sp--;
  NEXT();

OP(CR)
  putc('\n', stdout);
  NEXT();

OP(GETC)
  *++sp = tos;
  tos = getc(stdin);
  NEXT();

OP(GETS)
  tos = (int) fgets((char*) *--sp, tos, stdin);
  NEXT();

OP(VERSION)
  *++sp = tos;
  tos = (vfm_data_t) mp->version;
  NEXT();

OP(IDENT)
  *++sp = tos;
  tos = (vfm_data_t) mp->ident;
  NEXT();

OP(HALT)
  if (sp != env->sp0) *++sp = tos;
  env->sp = sp;
  env->ip = ip;
  env->rp = rp;
  env->dp = dp;
  env->mp = mp;
  return (0);
}

