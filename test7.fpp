// Math function test

package test

module test7

  : test1 ( -- ) 
    10 abs 
   -20 abs 
    0 abs 
  ;
  : test2 ( -- ) 
    10 -100 min 
         10 min 
    abs 100 min 
  ;
  : test3 ( -- ) 
    -100 10 max 
        -10 max 
    abs 100 max 
  ;
  : test4 ( -- ) 
    -101 -100 100 within 
    -100 -100 100 within 
       0 -100 100 within 
     100 -100 100 within 
     101 -100 100 within 
  ;
  : test5 ( -- )
    1 5 << 4 >> 
    negate
      5 << 4 >>
  ;
  : main ( -- )
    test1 empty
    test2 empty
    test3 empty
    test4 empty
    test5 empty
  ;

endmodule

