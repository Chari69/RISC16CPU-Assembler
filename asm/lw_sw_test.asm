JMP INICIO       ; Saltar la sección de datos

; --- SECCION DE DATOS ---
ORIGEN:  
.word 1234  ; Valor inicial (Decimal 1234 = 0x04D2)

; --- PROGRAMA ---
INICIO:
    JAL BASE_PC     ; Ahora R7 apunta a BASE_PC + 1      
BASE_PC:
    ; Cargar el valor 1234 desde ORIGEN a R1
    LW R1, ORIGEN(R7)  

    ; Verificar si cargó bien (R1 debería ser 1234)
    ; Le sumamos 1 para modificarlo
    ADDI R1, R1, 1     ; R1 = 1235

    ; Traduccion de IMOV
    IMOV R2, 6         ; R2 = 6

    ; Resta que genera NEGATIVO
    SUB  R3, R2, R1    ; R3 = 6 - 1235

    NOP

    HALT               ; Fin exitoso del programa