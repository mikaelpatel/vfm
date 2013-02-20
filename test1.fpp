// General testing and benchmarking of threading (next, nest and unnest)

package test

module test1

// Five function depth

: fun1 ;
: fun2 fun1 ;
: fun3 fun2 ;
: fun4 fun3 ;
: fun5 fun4 ;

// Conditional paths

: fun6 if -2 else 2 then ;
: fun7 -if -2 else 2 then ;

// Execution path tests

: test0 ;
: test1 nop nop nop nop nop ;
: test2 fun5 ;
: test3 false fun6 true fun6 false fun7 true fun7 ;

// Arithmetic and stack operation tests

: test4 1 2 + 3 * 4 / 2 == ;
: test5 1 -2 3 swap rot dup depth drop over ?dup 0 ?dup ;
: test6 1 -2 + 3 * 1+ dup 4 / 1- negate % 0= 0> 0<= 0< ;

// Iteration tests

: test7 5 begin 1- dup until drop ;
: test8 5 begin 1- dup while repeat drop ;
: test9 5 for i next ;
: test10 10 for i 3 -next ;
: test11 0 10 do i loop ;
: test12 0 10 do i 3 +loop ;
: test13 5 begin ?dup if 1- else exit then again ;

// Extended token test

: test14 ext0 nop ext0 nop ext0 nop ext0 nop ext0 nop ;

// All test cases

: main ( -- )
  test0 empty
  test1 empty
  test2 empty
  test3 empty
  test4 empty
  test5 empty
  test6 empty
  test7 empty
  test8 empty
  test9 empty
  test10 empty
  test11 empty
  test12 empty
  test13 empty
  test14 empty
;

endmodule

