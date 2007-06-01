! void *_flush()
!
! flushes the register windows to the stack, returns sp

.seg "text"
.global __flush
__flush:ta	3	! flush windows to the stack
	retl; mov %sp,%o0
