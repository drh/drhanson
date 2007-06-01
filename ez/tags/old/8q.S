procedure queens(c)
   local r

   for (r = 1; r <= 8; r = r + 1)
      if (rows[r] == up[r-c+8] == down[r+c-1] == 1) {
         rows[r] = up[r-c+8] = down[r+c-1] = 0
         x[c] = r
         if (c == 8)
            write(x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], "\n")
         else            queens(c + 1)
         rows[r] = up[r-c+8] = down[r+c-1] = 1
         }
end
for (i = 1; i <= 15; i = i + 1)
   up[i] = down[i] = 1
for (i = 1; i <= 8; i = i + 1)
   rows[i] = 1
queens(1)
