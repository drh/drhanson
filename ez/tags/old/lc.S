procedure lc(f)
   local n;
   for (n = 0; read(f); n = n + 1)
      ;
   write (n,"\n")
end;
lc(open("lc.S"));
