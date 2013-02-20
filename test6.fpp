// Access of variables and memory access across modules

package test

module test6

  use test.test5

  variable y

  // Implicit naming

  : foo ( -- )
    x @ y !
  ;

  : main ( -- )
    0 test.test5::x !
    0 y !
    foo test.test5::foo
    0x10203040 y !
    y dup c@ puti
    1+ dup c@ puti
    1+ dup c@ puti
    1+ c@ puti
    y @ putx
    cr
  ;

endmodule

