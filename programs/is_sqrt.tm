; Input: Unary x (in 1's)
; Output: T if x is a perfect square, F otherwise
;                                   _
; For input x = n^2, runs in x + 2\/x + 1 = (n+1)^2 steps
#Q = {q0,subl,subr,incrl,incrr,align,is_ps,not_ps,halt}
#S = {1}
#G = {_,1,T,F}
#q0 = q0
#B = _
#F = {halt}
#N = 2

; StartState Read Write Move EndState
q0 _* ** ** is_ps
q0 ** *1 ** subr

subl 11 _* rl subl
subl 1_ *1 *l incrl
subl _1 ** ** not_ps
subl __ ** ** is_ps

subr 11 _* rr subr
subr 1_ *1 *r incrr
subr _1 ** ** not_ps
subr __ ** ** is_ps

incrl  ** *1 ** subr
incrr  ** *1 ** subl

is_ps ** T* ** halt
not_ps ** F* ** halt
