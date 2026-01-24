; ==========================================================
; TEST DE CORRECCIONES: CARRY Y SHIFTS ITERATIVOS (FIXED)
; ==========================================================

; ----------------------------------------------------------
; 1. PRUEBA DE RESTA (SUB) y CARRY (C)
; ----------------------------------------------------------
; Caso A: 5 - 3 (Resultado positivo)
; Esperado: Resultado 2, C = 1 (Aritmética sin signo: No hubo préstamo)
ADDI R1, R0, 5      ; R1 = 5
ADDI R2, R0, 3      ; R2 = 3
SUB  R3, R1, R2     ; R3 = 5 - 3 = 2. CHECK: PSR C=1

; Caso B: 3 - 5 (Resultado negativo / Underflow)
; Esperado: Resultado -2 (0xFFFE), C = 0 (Aritmética sin signo: Hubo préstamo)
SUB  R4, R2, R1     ; R4 = 3 - 5 = -2. CHECK: PSR C=0


; ----------------------------------------------------------
; 2. PRUEBA DE LSL (Logical Shift Left)
; ----------------------------------------------------------
; Prueba A: Verificar que usa Rs2 como cantidad
ADDI R1, R0, 1      ; R1 = 1
ADDI R2, R0, 3      ; R2 = 3 (Cantidad a desplazar)
LSL  R5, R1, R2     ; R5 = 1 << 3. 
                    ; Esperado R5 = 8 (0x0008). 

; Prueba B: Carry en LSL (CORREGIDA)
; Queremos probar con 0x4000 (16384). Como no cabe en un inmediato de 7 bits,
; lo construimos dinámicamente: 1 << 14 = 16384.

ADDI R1, R0, 1      ; R1 = 1
ADDI R7, R0, 14     ; R7 = 14 (Auxiliar para crear el número)
LSL  R1, R1, R7     ; R1 = 1 << 14 = 0x4000 (Bit 14 encendido)

; Ahora hacemos el test real de Carry:
ADDI R2, R0, 2      ; Shift amount = 2
LSL  R6, R1, R2     ; R6 = 0x4000 << 2.
                    ; Vuelta 1: 0x8000. Vuelta 2: 0x0000.
                    ; Esperado R6 = 0x0000. CHECK: PSR C=1 (Bit 15 se cayó)


; ----------------------------------------------------------
; 3. PRUEBA DE LSR (Logical Shift Right)
; ----------------------------------------------------------
; Valor: 3 (Binario ...0011)
; Desplazamos 2 a la derecha.
ADDI R1, R0, 3      ; R1 = 3
ADDI R2, R0, 2      ; Cantidad = 2
LSR  R7, R1, R2     ; R7 = 3 >> 2.
                    ; Vuelta 1: 1 (Carry toma 1). Vuelta 2: 0 (Carry toma 1).
                    ; Esperado R7 = 0, CHECK: PSR C=1


; ----------------------------------------------------------
; 4. PRUEBA DE ASR (Arithmetic Shift Right)
; ----------------------------------------------------------
; Valor: -16 (0xFFF0). Signo negativo.
; Desplazamos 2 a la derecha. Debe mantener el signo.
ADDI R1, R0, -16    ; R1 = -16 (0xFFF0) -> Cabe en 7 bits signed (-64 a 63)
ADDI R2, R0, 2      ; Cantidad = 2
ASR  R7, R1, R2     ; R7 = -16 >> 2.
                    ; Esperado: -4 (0xFFFC).
                    ; Si da positivo (0x3FFC), el ASR está mal.

HALT