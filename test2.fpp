// Classical nesting intensive functions

package test

module test2

// Recursive factorial function

: fac ( x -- y ) 
  ?dup if 
    dup 1- fac * 
    exit
  then
  1 
;

// Iterative factorial function

: iter-fac ( x -- y ) 
  1 swap for 
    i ?dup 
    if * then 
  next 
;

// Recursive fibonacci function

: fib ( x -- y )
  dup 2 > if 
    dup 1- fib 
    swap 2- fib + 
    exit 
  then
  drop 1 
;

// The tests

: test1 ( -- x5..x0 )
  5 for i fac next
;

: test2 ( -- x5..x0 )
  5 for i fib next
;

: test3 ( -- x5..x0 )
  5 for i iter-fac next
;

: test4 ( -- x0 )
  40 fib 
;

: main ( -- )
  test1 empty
  test2 empty
  test3 empty
;

endmodule

