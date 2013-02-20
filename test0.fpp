// Package naming, ident and version string

package test

module test0

  ident " @(#) vfm/test, Copyright 2009, Mikael Patel"
  version " $Rev: 5 $"

  : rev ( -- version ident)
    version ident    
  ;

  : main ( -- )
    rev puts "  " puts puts cr
  ;

endmodule

