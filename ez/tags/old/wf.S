procedure wf(file)
   local t, line, i, j, f

   f = open(file, "r")
   while (line = map(read(f), ucase, lcase))
      for (i = 1; i = upto(lcase, line, i); i = j) {
         j = many(lcase, line, i)
         t[line[i:j]] = t[line[i:j]] + 1
         }
   for (i in t)
      write(t[i], "\t", i, "\n")
   close(f)
end
wf("wf.S")
