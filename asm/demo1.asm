start:
IMOV R1, 5
IMOV R2, 3
ADD R3, R1, R2
IMOV R7, 9
SUB R4, R1, R2
CMP R1, R2
BEQ igual
BNE distinto
igual:
NOP
JMP fin
distinto:
MOV R5, R3
LSL R6, R5, 1
JMP fin
.space 0xA
.word 0x1234
.string "OK"
fin:
HALT