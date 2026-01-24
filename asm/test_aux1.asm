; ==========================================
; TEST DE DESPLAZAMIENTOS Y CARRY
; ==========================================

; --- 1. PRUEBA DE DESPLAZAMIENTO (LSL) ---
; Objetivo: Verificar si desplaza X bits o siempre 1 bit.
; Tu código actual tiene 'res16 = a << 1', ignorando el argumento.

ADDI R1, R0, 1      ; R1 = 1
; Intentamos desplazar 3 bits a la izquierda. 
; Debería ser: 1 << 3 = 8 (0x0008)
; Si hay bug:  1 << 1 = 2 (0x0002)
; Nota: Usamos R3 en el tercer operando para que tu parser lea el valor '3' en rs2
LSL R2, R1, R3
HALT