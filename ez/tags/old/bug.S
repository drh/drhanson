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
#   dump(buf);
   for (i in tbuf)
      buf[pos = pos + 1] = tbuf[i]
end;
