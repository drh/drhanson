procedure hanoi(n, src, dst, tmp)
   if (n > 0) {
      hanoi(n - 1, src, tmp, dst)
      write("move a disk from ", src, " to ", dst, "\n")
      hanoi(n - 1, tmp, dst, src)
      }
end
hanoi(3, "a", "b", "c")
