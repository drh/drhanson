procedure wc(file)
   local nl, nc, nw, s, i, f;

   f = open(file);
   nc = nw = 0;
   for (nl = 0; s = read(f); nl = nl + 1) {
      s = s || " ";
      for (i = upto(ascii[34:0], s, 1); i = many(" \t", s, i);)
         nw = nw + 1;
      nc = nc + size(s)
      };
   write (nl, " ", nw, " ", nc, "\n")
end;
"wc.S:\t";
wc("wc.S");
"wf.S:\t";
wc("wf.S");
