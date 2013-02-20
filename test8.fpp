// Advanced control structure test

package test

module test8

  // Dump test for select
  : test0 ;

  // Chaining functions, no return address pushed

  : foo1 ( -- 1 ) 
    1
  ;
  : foo2 ( -- 2 ) 
    2 chain foo1
  ;
  : foo3 ( -- 3 ) 
    3 chain foo2
  ;
  : test1 ( -- )
    chain foo3
  ;

  // Recursive and tail recursive

  : fum1 ( n -- ) 
    ?dup if dup 1- recurse then ;
  ;
  : fum2 ( n -- )
    ?dup if dup 1- tailrecurse then ;
  ;
  : test2
    4 fum1 empty
    4 fum2 empty
  ;

  // Case statement

  : fie ( x -- y )
    case
    0 of 128 endof
    2 of 256 endof
    4 of 512 endof
    8 12 rangeof 1024 endof
    endcase
  ;
  : test3 ( -- )
    15 for 
      i fie empty
    next 
  ;

  // Multiple definitions with guards (fallback to previous definition)
  // Factorial function as two definitions

  : fac ( x -- 1 )
    drop 1
  ;
  : fac ( x -- x! )
    dup 0> 
  guard 
    dup 1- fac * 
  ;
  : test4 ( -- )
    5 fac   
  ;

  // Integer mapping function as multiple definitions
  // x > 256 : 255 - x
  // x > 128 : 127 - x
  // x < 127 : 1

  : foo ( x -- 1 )
    drop 1
  ;
  : foo ( x -- 127-x )
    dup 128 > 
  guard 
    127 - 
  ;
  : foo ( x -- 255-x )
    dup 256 > 
  guard 
    255 -
  ;
  : test5 ( -- )
    100 foo
    150 foo
    300 foo
  ;

  : main ( -- )
    4 for 
      i select
        test0
        test1
	test2
	test3
	test4
	test5
      endselect
      empty
    next
  ;

endmodule

