#Q = {q0,q1,q2}
#S = {a,b}
#G = {a,b,B}
#q0 = q0
#B = B
#F = {q2}
#N = 1

; StartState Read Write Move EndState
q0 a B r q0
q0 b B r q1
q1 b B R q1 ; here
q1 B B r q2
