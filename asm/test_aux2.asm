; --- 2. PRUEBA DE CARRY EN RESTA (SUB) ---
; Objetivo: Verificar si C es "Carry Out" o "Borrow".
; Definición PDF: C = 1 si hay acarreo de salida.
; Tu código: C = (a < b) -> Esto es Borrow (1 si falta, 0 si sobra).

; CASO A: Resta SIN Acarreo negativo (5 - 3)
; Matemáticas: 5 - 3 = 2. No se pide prestado.
; Carry Out debería ser 1.
; Tu código (5 < 3) dará C = 0.
ADDI R3, R0, 5      ; R3 = 5
ADDI R4, R0, 3      ; R4 = 3
SUB  R5, R3, R4     ; R5 = 5 - 3 = 2
HALT