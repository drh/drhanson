procedure sh(cd)
   local args, i;

   for (write("% "); args = getargs(read()); write("% "))
      if (args[0] == "c")
         cd[args[1]] = read() || "\n"
      else if (args[0] == "d")
         cd[args[1]] = ""
      else if (args[0] == "p")
         write(cd[args[1]])
      else if (args[0] == "ls")
         for (i in cd) write(i, "\n")
      else if (args[0] == "cd") {
         if (type(cd[args[1]]) ~= "table") {
            cd[args[1]] = "";
            cd[args[1]]["."] = cd[args[1]];
            cd[args[1]][".."] = cd;
            };
         cd = cd[args[1]];
         };
end;

procedure getargs(lin)
   local args, i, p1, p2;

   if (lin == "") return("");
   p2 = 1;
   lin = " " || lin || " ";
   for (i = 0; p2 = upto(" ", lin, p1 = many(" ", lin, p2)); i = i + 1)
      args[i] = lin[p1:p2];
   return (args);
end;

sh(root["."] = root);
