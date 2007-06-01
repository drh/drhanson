procedure insert(pos)
   local line, tbuf, i;

   for (i in buf)
      if (i > pos)
         tbuf[i] = buf[i];
   while (line = read(script))
      if (line ~= ".")
         buf[pos = pos + 1] = line
      else
         break;
   for (i in tbuf)
      buf[pos = pos + 1] = tbuf[i]
end;

procedure delete(pos1, pos2)
   local n, i;

   n = pos2 - pos1 + 1;
   for (i = pos1; i <= size(buf) - n; i = i + 1)
      buf[i] = buf[i + n];
   for (; i <= size(buf); i = i + 1)
         buf[i] = ""
end;

procedure print(pos1, pos2)
   for (; pos1 <= pos2; pos1 = pos1 + 1)
      write(buf[pos1], "\n");
end;

procedure fwrite(f)
   local out, i;

   outp = open(f, "w");
   for (i in buf)
      if (buf[i] ~= "")
         write(outp, buf[i], "\n");
   close(outp);
end;

inp = open("ed.in");
for (i = 1; line = read(inp); i = i + 1)
   buf[i] = line;
close(inp);
script = open("ed.script");
digits = "0123456789";
while (line = read(script)) {
   low = integer(line[1:many(digits, line)]);
   if (i = many(",", line))
      high = integer(line[i:many(digits, line, i)])
   else
      high = low;
   if (find("i", line))
      insert(low)
   else if (find("d", line))
      delete(low, high)
   else if (find("p", line))
      print(low, high)
   else if (i=match("w", line))
      fwrite(line[upto(ascii[34:0],line,i):0])
   };
