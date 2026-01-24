; CASO B: Resta CON Acarreo negativo (3 - 5)
; Matemáticas: 3 - 5 = -2 (0xFFFE). Se pide prestado.
; Carry Out debería ser 0.
; Tu código (3 < 5) dará C = 1.
ADDI R3, R0, 5      ; R3 = 5
ADDI R4, R0, 3      ; R4 = 3
SUB  R6, R4, R3     ; R6 = 3 - 5 = -2
HALT