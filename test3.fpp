// Testing module referencing and name space handling

package test

module test3

  use test.test0
  use test.test1
  use test.test2

  // Implicit referencing

  : foo ( -- ) fun5 ;
  : fie ( -- x ) 5 fac ;
  : fum ( -- y ) 5 fib ;

  // Explicit referencing

  : test1 ( -- )
    test.test1::fun5
    test.test3::foo
    fie 
    fum 
  ;

  // Function reference

  : fac ( n -- n!) test.test2::fac ;
  : five ( fn -- x ) 5 swap execute ;
  : test2 ( -- ) ' fac five ;

  // Delegation to used module main functions to show startup sequencing

  : main ( -- )
    test.test0::rev puts "  " puts puts cr
    test.test1::main empty
    test.test2::main empty
    test.test3::test1 empty
    test.test3::test2 empty
  ;

endmodule

