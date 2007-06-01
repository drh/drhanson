f = open("checkbook");
while (line = read(f))
   cbook[integer(line[1:upto(":", line, 1)])] = line;
balance = 545.45;
write("num\tdate\tamt\tdep\tbalance\twho\n");
for (num in cbook) {
   i = upto(":", cbook[num], 1);
   date = cbook[num][i+1:j = upto(":", cbook[num], i + 1)];
   who = cbook[num][j+1:i = upto(":", cbook[num], j + 1)];
   amt = cbook[num][i+1:j = upto(":", cbook[num], i + 1)];
   dep = cbook[num][j+1:i = upto(":", cbook[num], j + 1)];
   balance = balance + dep - amt;
   write(num, "\t", date, "\t", amt, "\t", dep, "\t", balance, "\t", who, "\n")
   }
