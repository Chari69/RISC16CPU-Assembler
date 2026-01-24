; Prueba de traduccion de pseudo-instruccion MOV
; Objetivo: Verificar si MOV R2, R1 se traduce como ADD R2, R1, R0

ADDI R1, R0, 5   ; Cargamos 5 en R1 para tener un dato
MOV  R2, R1      ; Aquí está la prueba crítica
IMOV R5, 9
NOP
LSL R4, R5, 1
HALT             ; Fin del programa