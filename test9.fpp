// Task structure

package test

module test9

  ident " @(#) vfm/test.test9, Copyright 2009, Mikael Patel"
  version " $Rev: 5 $"

  : main ( -- )
    " task: " puts task dup putx cr
    " status: " puts dup @ putx cr
    " mp: " puts cell + dup @ putx cr    
    dup @
    "   name: " puts dup @ puts cr
    "   ident: " puts cell + dup @ puts cr
    "   version: " puts cell + dup @ puts cr
    "   timestamp: " puts cell + dup @ putx cr
    drop
    " sp: " puts cell + dup @ putx cr 
    " ip: " puts cell + dup @ putx cr 
    " dp: " puts cell + dup @ putx cr 
    " rp: " puts cell + dup @ putx cr 
    " sp0: " puts cell + dup @ putx cr 
    " rp0: " puts cell + dup @ putx cr 
    " dp0: " puts cell + dup @ putx cr 
    .s
  ;

endmodule

