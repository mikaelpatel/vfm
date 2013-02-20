// Benchmarks and test of output operations

package test

module test4

  use test.test1
  use test.test2

  : benchmark1 ( -- ) 20 fib . cr ;
  : benchmark2 ( -- ) 10 test.test1::test8 " test8" puts cr ;
  : benchmark3 ( -- )" hello world" puts cr ;

  : main ( -- )
    benchmark1 
    benchmark2
    benchmark3
  ;

endmodule


