; symbol V is not declared
#Q = {q0,q1,q2,q3,q4,u,z,z_,o,o_,t,t_,a,r}
#S = {0,1,2,#}
#G = {0,1,2,#,B}
#q0 = q0
#B = B
#F = {a}
#N = 1

; StartState Read Write Move EndState
q0 # * r q1
q0 B B * r
q0 * * r q0

q1 # # r r
q1 B V l q2
q1 * * r q1

q2 # # l q3
q2 * * l q2

q3 # # * r
q3 B B r q4
q3 * * l q3

q4 0 B r z
q4 1 B r o
q4 2 B r t
q4 # # r a
q4 B B r a

z # # r z_
z * * r z

z_ 0 B l q2
z_ B B r z_
z_ V V * r
z_ * * r z_

o # # r o_
o * * r o

o_ 1 B l q2
o_ B B r o_
o_ V V * r
o_ * * r o_

t # # r t_
t * * r t

t_ 2 B l q2
t_ B B r t_
t_ V V * r
t_ * * r t_
