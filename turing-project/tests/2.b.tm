#Q = {q0,q1,q2,q3,q4,q5,a,r}
#S = {a,b}
#G = {a,b,B}
#q0 = q0
#B = B
#F = {a}
#N = 1

; StartState Read Write Move EndState
q0 a B r q1
q0 b B r q2
q0 B B * a

q1 B B l q3
q1 * * r q1

q2 B B l q4
q2 * * r q2

q3 a B l q5
q3 b b * r
q3 B B * r

q4 a a * r
q4 b B l q5
q4 B B * r

q5 B B r q0
q5 * * l q5