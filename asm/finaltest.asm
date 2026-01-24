; ==========================================
; TEST SUITE PARA PROYECTO OECI RISC-16
; ==========================================

.org 0xF798          ; Dirección inicial calculada (Suma Cédulas)

    JMP INICIO       ; Saltar datos para no ejecutarlos como instrucciones

; --- SECCION DE DATOS ---
VALOR1: 
.word 10     ; Prueba .word
VALOR2: 
.word -5     ; Prueba número negativo
STR:    
.string "AB" ; Prueba .string (ocupa 2 palabras: 'A', 'B')
ESP:    
.space 2     ; Prueba .space
CONST_MAX:
.word 32767 

INICIO:
    ; 1. PRUEBA PSEUDO-INSTRUCCIONES
    NOP              ; Debe traducirse a ADD R0, R0, R0
    IMOV R1, 1       ; Cargar 1 en R1 (ADDI R1, 0, 1)
    MOV R2, R1       ; Copiar R1 a R2 (ADD R2, R1, R0)

    ; 2. PRUEBA ARITMETICA Y MEMORIA (TIPO I)
    JAL CHUNGO
    CHUNGO:
    LW R3, VALOR1(R7); Carga 10 desde memoria a R3. Offset=VALOR1-PC
    ADDI R3, R3, 5   ; R3 = 10 + 5 = 15 (0x000F)
    SW R3, ESP(R7)   ; Guardar 15 en espacio reservado

    ; 3. PRUEBA BANDERAS (PSR)
    ; -- Prueba Z (Zero) --
    CMP R2, R1       ; 1 - 1 = 0. Debe encender Z=1
    BNE FALLO        ; Si Z=0 (no iguales), salta a error
    
    ; -- Prueba N (Negative) --
    IMOV R4, 0
    SUBI R4, R4, 1   ; 0 - 1 = -1 (0xFFFF). Debe encender N=1
    
    ; -- Prueba V (Overflow) --
    ; 32767 + 1 = 32768 (Negativo en complemento a 2 de 16 bits)
    JAL OVERTEST
    OVERTEST:
    LW R5, CONST_MAX(R7)
    ADDI R5, R5, 1   ; 0x8000. Debe encender V=1 y N=1

    ; 4. PRUEBA LOGICA Y SHIFTS (TIPO R)
    IMOV R6, 3       ; R6 = 000...0011
    LSL R6, R1, R6   ; R6 = 3 << 1 = 6 (0110)
    AND R6, R6, R1   ; 6 AND 1 = 0 (0110 & 0001)

    ; 5. PRUEBA RUTINAS (JAL / RET)
    IMOV R1, 10
    JAL RUTINA       ; Salta a RUTINA, guarda PC+1 en R7
    
    ; Al volver:
    CMP R1, R1       ; Forzar Z=1 para terminar bien
    BEQ FIN          ; Salta al final exitoso

FALLO:
    HALT             ; Si llega aquí, algo falló antes

RUTINA:
    ADD R1, R1, R1   ; R1 = 10 + 10 = 20
    RET              ; Vuelve al PC guardado en R7

FIN:
    HALT             ; Fin del programa