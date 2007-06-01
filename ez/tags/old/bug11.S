procedure sh(cd)
   local args, i, lin;

   for (write("% "); lin = read(); write("% ")) {
      if (args = getargs(lin)) 0 else next;
      if (args[0] == "c")
         cd[args[1]] = read() || "\n"
      else if (args[0] == "d")
         cd[args[1]] = ""
      else if (args[0] == "p")
         write(cd[args[1]])
      else if (args[0] == "ls") {
         for (i in cd)
            if (type(cd[i]) ~= "string" | cd[i] ~= "") write(i, "\n")
         }
      else if (args[0] == "cd") {
         dump(args);  dump(cd);
         if (args[1]) 0 else args[1] = "";
         if (type(cd[args[1]]) ~= "table") {
write("1");
            cd[args[1]] = "";
write("2");
            cd[args[1]][""] = root;
write("3");
            cd[args[1]]["."] = cd[args[1]];
write("4");
            cd[args[1]][".."] = cd;
write("5");
            };
         cd = cd[args[1]];
         };
      };
end;

procedure getargs(lin)
   local args, i, p1, p2;

   p2 = 1;
   lin = " " || lin || " ";
   for (i = 0; p2 = upto(" ", lin, p1 = many(" ", lin, p2)); i = i + 1)
      args[i] = lin[p1:p2];
   return (args);
end;

sh(root[""] = root["."] = root[".."] = root);
