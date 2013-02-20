// Variable and memory access tests

package test

module test5

  create msg " string constant"
  variable x
  create y 120 allot
  100 constant z

  : foo ( -- x )
    x @ 1+ x ! x @ 
    0x10 x 3 + c! x @ 
    x @ putx
  ;

  : fie ( -- x )
    y  1 over c!
    1+ 2 over c!
    1+ 3 over c!
    1+ 4 swap c!
    y @ putx
    y 3 for
      dup c@ puti
      1+
    next drop     
  ;

  : fum ( -- x )
    z x ! x @ puti
  ;

  : main ( -- )
      msg puts cr
      foo empty cr
      fie empty cr
      fum empty cr
  ;

endmodule
