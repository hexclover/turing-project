; Input: a(unary) 0 b(unary) on tape 0
; Output: gcd(a, b)(unary) on tape 0
; NOTE:
; - The rules take precedence in the order they are defined (like in pattern matching in Haskell, etc.).
#Q = {q0,find0,isAZero,isBZero,nz,markA,markARight,markB,doCmp,ge,lt,geReset,geDoSub,ltReset,output,fin}
#S = {0,1}
#G = {0,1,A,B,_}
#q0 = q0
#B = _
#F = {fin}
; Tapes:
;   0: I/O
;   1: Marks the furthest bit of A
;   2: Marks the furthest bit of B
#N = 3

; StartState Read Write Move EndState

; q0: Stub initial state.
q0 *** *** *** find0

; find0: Move head0 right to locate the separator 0
;        Assume heads 1&2 at position 0
find0 0** 000 *** isAZero
find0 *** *** r** find0

; isAZero: When head0 is at 0, check if a is 0
isAZero 0** *** l** isAZero
isAZero 1** *** r** isBZero
isAZero _** *** r** output

; isBZero: Now check if b is 0 (knowing that a != 0)
isBZero 0** *** r** isBZero
isBZero _** *** l** output
isBZero 1** *** l** nz

; nz: ab != 0. Let's compare them
nz *** *** *** markA

; markA: Move head 0 and head 1 left, find a place where head0 reads _
;        then dump an A symbol
markA _** *A* r** markARight
markA *** *** ll* markA

; markARight: Move head 0 right to 0, then start markB
markARight 0** *** *** markB
markARight *** *** r** markARight

; markB: Move head 0 and head 2 right, find a place where head0 reads _
;        and put down a B
markB _** **B *** doCmp
markB *** *** r*r markB

; doCmp: Move head 1 and 2 towards position 0, until one reads the symbol 0 (center)
; Note:
;   At the moment the comparison finishes, head 0 is located at one past b
;   So moving head 0 <- when jumping to gt/eq simplifies the process a bit
doCmp *00 *** l** ge ; equal
doCmp *0* *** *** lt ; a < b
doCmp **0 *** l** ge ; a > b
doCmp *** *** *rl doCmp ; continue

; ge: a >= b, PREPARE TO subtract b from a
ge _*B *** *** geDoSub
ge **B *** l** ge ; move head 0 left to find a _
ge *** *** **r ge ; move head 2 right to find the B again

; geDoSub: actually do subtraction
;     Just move 0 -> and 2 <- and erase characters on tape 0, until head 2 reads 0
geDoSub **0 *** *** geReset
geDoSub *** _** r*l geDoSub

; geReset: At this moment, head 2 must be at the central zero
;          Heads 0 and 1 may need to be moved furthur to the right for re-centering
geReset 000 *** *** isAZero
geReset 00* *** **r geReset
geReset 0** *** *r* geReset
geReset *** *** r** geReset

; lt: a < b, subtract a from b
;     Just move 0 and 1 left and erase characters on tape 0, until head 1 reads A
lt *A* *** *** ltReset
lt *** _** ll* lt

; ltReset: maybe move {head0 <-, head1 ->, head 2 <-} to make all of them rest on the ZERO
ltReset 000 *** *** isAZero
ltReset 00* *** **l ltReset
ltReset 0** *** *r* ltReset
ltReset *** *** l** ltReset

; output: Erase the separator 0
output 0** _** *** fin

; Halt: Halt
