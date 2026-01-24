#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <cstdint>

using namespace std;

/*
    CALCULO DE DIRECCION INICIAL

    31228863 + 32303416 + 32606881 = 96139160
    U2B(96139160) = 0101 1011 1010 1111 0111 1001 1000

    Truncado a 16 bits
    96139160 mod 65.536 = 63384

    U2B(63384) = 1111 0111 1001 1000 

    Equivalente a HEX
    1111 0111 1001 1000 = 0xF798
*/
#define DIR_INICIAL 0xF798 // Dirección inicial

struct PSR {
    bool Z = false; // Zero
    bool N = false; // Negative
    bool C = false; // Carry
    bool V = false; // Overflow
};

// Clase que representa la CPU del RISC-16
class CPU {
private:
    uint16_t regs[8] = {0}; // Banco de Registros (R0-R7)

public:
    uint16_t PC = DIR_INICIAL;
    PSR psr;
    bool halted = false;

    // Lectura de registros
    uint16_t leer(int r) const {
        if(r == 0) return 0;
        return regs[r];
    }

    // Escritura en Registros
    // La escritura en R0 es ignorada
    void escribir(int r, uint16_t valor) {
        if(r == 0) return; 
        regs[r] = valor;
    }

    // Actualiza banderas Z (Zero) y N (Negative)
    // Usado en operaciones logicas bits
    void actualizarZN(uint16_t v) {
        psr.Z = (v == 0);
        psr.N = (v & 0x8000);
    }

    // Actualiza todas las banderas para SUMA
    void actualizarAdd(uint16_t a, uint16_t b, uint32_t res32) {
        psr.C = (res32 > 0xFFFF); // Carry si el resultado excede 16 bits

        // Overflow: 
        // Si sumamos dos positivos y da negativo, o dos negativos y da positivo
        int16_t sa = (int16_t)a;
        int16_t sb = (int16_t)b;
        int16_t sr = (int16_t)(res32 & 0xFFFF);

        psr.V = ((sa >= 0 && sb >= 0 && sr < 0) ||
                (sa < 0  && sb < 0  && sr >= 0));
    }

    // Actualiza todas las banderas para RESTA (SUB, SUBI, CMP)
    void actualizarSub(uint16_t a, uint16_t b, uint32_t res32) {
        psr.C = (a >= b); // Carry

        // Check Overflow
        int16_t sa = (int16_t)a;
        int16_t sb = (int16_t)b;
        int16_t sr = (int16_t)(res32 & 0xFFFF);

        psr.V = ((sa >= 0 && sb < 0 && sr < 0) ||
                (sa < 0  && sb >= 0 && sr >= 0));
    }
};

// Memoria RAM simulada
class Memoria {
public:
    map<uint16_t, uint16_t> mem; // direccion -> valor

    uint16_t leer(uint16_t dir) {
        return mem[dir];
    }

    void escribir(uint16_t dir, uint16_t v) {
        mem[dir] = v;
    }
    
};

// Clase base para cualquier instruccion
// se utiliza polimorfismo para manejar los 3 formatos (R, I, J)
// adicionalmente se hizo un formato dato
class Instruccion {
protected:
    string nemonico;     // Nombre de la instruccion (ADD, JMP, etc)
    string instruccion;  // Linea en ensamblador original/traducida
    uint16_t direccion;  // PC donde esta ubicada
    uint16_t codigo;     // Codigo maquina generado

public:
    Instruccion(const string& n, uint16_t dir, string &inst)
        : nemonico(n), direccion(dir), instruccion(inst),codigo(0) {}

    // metodos virtuales que cada tipo de instruccion debe implementar para aprovechar el polimorfismo
    virtual void ensamblar() = 0; // Genera el codigo binario
    virtual void ejecutar(CPU& cpu, Memoria& mem) = 0; // Simula el comportamiento
    virtual void cargarDatos(Memoria& mem) {} // Para directivas como .word

    string getInstruccion() const { return instruccion; }
    uint16_t getCodigo() const { return codigo; }
    uint16_t getDireccion() const { return direccion; } 
};


// Tabla de simbolos: etiqueta -> dirección
map<string, uint16_t> tablaSimbolos;

// Programa: lista de instrucciones en orden
vector<Instruccion*> programa;


// INSTRUCCION TIPO R
// COP | 00 | Rs1 | Rs2 | Rd
class InstruccionR : public Instruccion {
    uint8_t cop, rs1, rs2, rd;

public:
    InstruccionR(const string& n, uint16_t dir, uint8_t cop, uint8_t rs1, uint8_t rs2, uint8_t rd, string &linea)
        : Instruccion(n, dir, linea), cop(cop), rs1(rs1), rs2(rs2), rd(rd) {}

    void ensamblar() override {
        // Construcción del código máquina mediante desplazamientos
        codigo = 0;
        codigo |= (cop & 0xF) << 12; // Bits 15-12: COP
        codigo |= (0b00) << 10;      // Bits 11-10: Tipo 00
        codigo |= (rs1 & 0x7) << 7;  // Bits 9-7: Rs1
        codigo |= (rs2 & 0x7) << 4;  // Bits 6-4: Rs2
        codigo |= (rd & 0x7);        // Bits 3-0: Rd
    }

    void ejecutar(CPU& cpu, Memoria &mem) override {
        uint16_t a = cpu.leer(rs1);
        uint16_t b = cpu.leer(rs2);
        uint32_t res32;
        uint16_t res16;

        switch (cop) {

        case 0: // ADD
            res32 = (uint32_t)a + b;
            res16 = res32;
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.actualizarAdd(a, b, res32);
            break;

        case 1: // SUB
            res32 = (uint32_t)a - b;
            res16 = res32;
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.actualizarSub(a, b, res32);
            break;

        case 2: // AND
            res16 = a & b;
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.psr.C = false;
            cpu.psr.V = false;
            break;

        case 3: // ORR
            res16 = a | b;
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.psr.C = false;
            cpu.psr.V = false;
            break;

        case 4: // CMP
            res32 = (uint32_t)a - b;
            res16 = res32;
            cpu.actualizarZN(res16);
            cpu.actualizarSub(a, b, res32);
            break;

        case 5: // LSL
            res16 = a;
            for (int i = 0; i < rs2; i++) {
                cpu.psr.C = (res16 >> 15) & 1; // El bit que sale va al Carry
                res16 = res16 << 1;       
            }
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.psr.V = false;
            break;

        case 6: // LSR
            res16 = a;
            for (int i = 0; i < rs2; i++) {
                cpu.psr.C = res16 & 1; // El bit que sale va al Carry
                res16 = res16 >> 1; 
            }
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.psr.V = false;
            break;

        case 7: // ASR
            res16 = a;
            for (int i = 0; i < rs2; i++) {
                cpu.psr.C = res16 & 1;        // Guardo el último bit
                res16 = ((int16_t)res16) >> 1; // Desplazo 1 manteniendo signo (C++ hace ASR en tipos con signo)
            }
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.psr.V = false;
            break;
        }

        cpu.PC++;
    }
};

// INSTRUCCION TIPO I
// COP | 01 | Rs1 | Inmediato
class InstruccionI : public Instruccion {
    uint8_t cop, rs1, rd;
    int8_t imm;         // Inmediato de 7 bits con signo
    string etiqueta;    // Para resolver direcciones en LW/SW

public:
    InstruccionI(const string& n, uint16_t dir, uint8_t cop, uint8_t rs1, int8_t imm, uint8_t rd, string &linea, string etq = "")
        : Instruccion(n, dir, linea), cop(cop), rs1(rs1), rd(rd), imm(imm), etiqueta(etq) {}

    void ensamblar() override {
        // Resolucion de direcciones (Offset = Etiqueta - PC actual)
        if (!etiqueta.empty() && tablaSimbolos.count(etiqueta)) {
            uint16_t target = tablaSimbolos[etiqueta];
            int diferencia = (int)(target - direccion);
            imm = (int8_t)diferencia;
        }

        codigo = 0;
        codigo |= (cop & 0xF) << 12;
        codigo |= (0b01) << 10;
        codigo |= (rs1 & 0x7) << 7;
        codigo |= (imm & 0x7F); // mascara de 7 bits
    }

    
    void ejecutar(CPU& cpu, Memoria& mem) override {
        uint16_t base = cpu.leer(rs1);
        int16_t imm_ext = imm;
        uint32_t res32;
        uint16_t res16;
        uint16_t addr;

        switch (cop) {

        case 0: // ADDI
            res32 = (uint16_t)base + (uint16_t)imm_ext;
            res16 = res32;
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.actualizarAdd(base, imm_ext, res32);
            cpu.PC++;
            break;

        case 1: // SUBI
            res32 = base - imm_ext;
            res16 = res32;
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.actualizarSub(base, imm_ext, res32);
            cpu.PC++;
            break;

        case 2: // ANDI
            res16 = base & imm_ext;
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.psr.C = false;
            cpu.psr.V = false;
            cpu.PC++;
            break;

        case 3: // ORI
            res16 = base | imm_ext;
            cpu.escribir(rd, res16);
            cpu.actualizarZN(res16);
            cpu.psr.C = false;
            cpu.psr.V = false;
            cpu.PC++;
            break;

        case 4: // LW
            addr = base + imm_ext;
            cpu.escribir(rd, mem.leer(addr));
            cpu.PC++;
            break;

        case 5: // SW
            addr = base + imm_ext;
            mem.escribir(addr, cpu.leer(rd));
            cpu.PC++;
            break;
        }
    }
};

// INSTRUCCION TIPO J
// COP | 10 | Cond | Offset

class InstruccionJ : public Instruccion {
    uint8_t cop, cond;
    string etiquetaDestino;
    int8_t offset; // salto relativo al PC

public:
    InstruccionJ(const string& n, uint16_t dir, uint8_t cop, uint8_t cond, string &linea)
        : Instruccion(n, dir, linea), cop(cop), cond(cond), offset(0) {}

    void setOffset(int8_t off) { offset = off; }
    void setEtiquetaDestino(const string& etq) { etiquetaDestino = etq; }

    void ensamblar() override {
        // calculo del offset para saltos
        if (!etiquetaDestino.empty()) {
            if (tablaSimbolos.count(etiquetaDestino)) {
                int16_t target = tablaSimbolos[etiquetaDestino];
                // Offset = (Direccion de Etiqueta) - PC
                offset = (int8_t)(target - direccion); 
            } else {
                cout << "Error: Etiqueta no encontrada: " << etiquetaDestino << endl;
                exit(1);
            }
        }
        codigo = 0;
        codigo |= (cop & 0xF) << 12;
        codigo |= (0b10) << 10;
        codigo |= (cond & 0x7) << 7;
        codigo |= (offset & 0x7F);
    }

    void ejecutar(CPU& cpu, Memoria &mem) {
        bool saltar = false;

        switch (cop) {
            case 0: // JMP
                saltar = true;
                break;

            case 1: // BEQ
                saltar = cpu.psr.Z;
                break;

            case 2: // BNE
                saltar = !cpu.psr.Z;
                break;

            case 3: // BGT
                saltar = (!cpu.psr.Z && cpu.psr.N == cpu.psr.V);
                break;

            case 4: // JAL
                cpu.escribir(7, cpu.PC + 1);
                saltar = true;
                break;

            case 5: // RET
                cpu.PC = cpu.leer(7);
                return;

            case 6: // RETI
                cpu.PC = cpu.leer(7);
                return;

            case 7: // HALT
                cpu.halted = true;
                return;
        }

        if (saltar) {
            cpu.PC += offset;
        } else {
            cpu.PC++;
        }
    }
};

// INSTRUCCION DATO
// PARA EL MANEJO DE DATOS Y DIRECTIVAS (.data, .word)
class InstruccionDato : public Instruccion {
    uint16_t valor;

public:
    InstruccionDato(uint16_t dir, uint16_t v, string &inst) 
        : Instruccion(".data", dir, inst), valor(v) {}

    void ensamblar() override {
        codigo = valor; // El codigo máquina es el dato mismo
    }

    void ejecutar(CPU& cpu, Memoria& mem) override {
        // Al "ejecutar" un dato, la CPU simplemente lo ignora y sigue
        //mem.escribir(direccion, valor); // IDK: capaz deberia comentar la linea? ya se lee la memoria al inicio de la simulacion con cargarDatos().
        cpu.PC++; 
    }

    void cargarDatos(Memoria& mem) override {
        mem.escribir(direccion, valor);
    }
};

// ------------------------------------------------------------
//                  FUNCIONES AUXILIARES
// ------------------------------------------------------------

// Limpia comentarios (;) y espacios en blanco
string limpiarLinea(string linea) {
    size_t pos = linea.find(';');
    if (pos != string::npos) linea = linea.substr(0, pos);
    
    size_t ini = linea.find_first_not_of(" \t\r\n");
    if (ini == string::npos) return ""; 

    size_t fin = linea.find_last_not_of(" \t\r\n");
    return linea.substr(ini, fin - ini + 1);
}

// Tokeniza separando por espacios y comas
// Divide una línea en tokens
vector<string> tokenizar(const string& linea) {
    vector<string> tokens;
    string token;
    stringstream ss(linea);

    while (ss >> token) {
        if (token.back() == ',')
            token.pop_back();
        tokens.push_back(token);
    }
    return tokens;
}

// Verifica si un string es numero 
// sirve tanto para hex o decimal
bool esNumero(const string& s) {
    if (s.empty()) return false;
    char* end = nullptr;
    strtol(s.c_str(), &end, 0);
    return end && *end == '\0';
}

// Formato Hexadecimal de 4 digitos para el output
string hex4(uint16_t valor) {
    string res = "0000";
    const char* digitos = "0123456789ABCDEF";
    
    res[0] = digitos[(valor >> 12) & 0xF];
    res[1] = digitos[(valor >> 8) & 0xF];
    res[2] = digitos[(valor >> 4) & 0xF];
    res[3] = digitos[valor & 0xF];
    
    return res;
}

// Limpia el simbolo '$' de inmediatos
string limpiarInmediato(string s) {
    if (s.empty()) return s;
    if (s[0] == '$') return s.substr(1);
    return s;
}

// Convierte Registros a solo el numero ("R3" -> 3)
int reg(const string &r) {
    string clean = limpiarInmediato(r);
    if(esNumero(clean)) return stoi(clean);
    return clean[1] - '0';
}

// Convierte a binario string para el output
string bin16(uint16_t valor) {
    string res(16, '0');
    for (int i = 15; i >= 0; --i) {
        res[15 - i] = ((valor >> i) & 1) ? '1' : '0';
    }
    return res;
}

// ------------------------------------------------------------
//                    MANEJO DE DIRECTIVAS
// ------------------------------------------------------------

void procesarDirectiva(const vector<string>& t, uint16_t& pc, string &linea) {
    if (t[0] == ".org") {
        pc = stoi(t[1], nullptr, 0); // establece una nueva direccion como PC 
    }
    else if (t[0] == ".word") {
        uint16_t val = stoi(t[1], nullptr, 0);
        programa.push_back(new InstruccionDato(pc, val, linea));
        pc += 1;
    }
    else if (t[0] == ".space") { // reserva espacio vacio
        int cantidad = stoi(t[1], nullptr, 0);
        for (int i = 0; i < cantidad; i++) {
            string spaceLine = "[espacio reservado]";
            programa.push_back(new InstruccionDato(pc, 0, spaceLine));
            pc += 1;
        }
    }
    else if (t[0] == ".string") { // almacena cadena de caracteres
        string contenido = t[1].substr(1, t[1].size() - 2);
        for (char c : contenido) {
            string charLine = "[char] '" + string(1, c) + "'";
            programa.push_back(new InstruccionDato(pc, (uint16_t)c, charLine));
            pc += 1;
        }
    }
}

// ------------------------------------------------------------
//           MANEJO y PROCESAMIENTO DE INSTRUCCIONES
// ------------------------------------------------------------

void procesarInstruccion(const vector<string>& t, uint16_t pc, string &linea) {
    string op = t[0];

    // ---------- TIPO R ----------
    if (op == "ADD" || op == "SUB" || op == "AND" || op == "ORR" ||
        op == "CMP" || op == "LSL" || op == "LSR" || op == "ASR") {

        int cop =
            (op == "ADD") ? 0 :
            (op == "SUB") ? 1 :
            (op == "AND") ? 2 :
            (op == "ORR") ? 3 :
            (op == "CMP") ? 4 :
            (op == "LSL") ? 5 :
            (op == "LSR") ? 6 : 7;

        int rd  = (op == "CMP") ? 0 : reg(t[1]); // CMP no guarda resultado
        int rs1 = (op == "CMP") ? reg(t[1]) : reg(t[2]);
        int rs2 = (op == "CMP") ? reg(t[2]) : reg(t[3]);

        programa.push_back(
            new InstruccionR(op, pc, cop, rs1, rs2, rd, linea)
        );
        return;
    }

    // ---------- TIPO I ARITMETICO ----------
    if (op == "ADDI" || op == "SUBI" || op == "ANDI" || op == "ORI") {
        int cop =
            (op == "ADDI") ? 0 :
            (op == "SUBI") ? 1 :
            (op == "ANDI") ? 2 : 3;

        string immStr = limpiarInmediato(t[3]);

        programa.push_back(
            new InstruccionI(op, pc, cop, reg(t[2]), stoi(immStr), reg(t[1]), linea)
        );
        return;
    }

    // ---------- LOAD / STORE ----------
    if (op == "LW" || op == "SW") {
        int cop = (op == "LW") ? 4 : 5;

        unsigned p1 = t[2].find('(');
        unsigned p2 = t[2].find(')');

        string offsetStr = t[2].substr(0, p1); // Sacamos el texto antes del parentesis
        int offset = 0;
        string etiqueta = "";

        if (esNumero(offsetStr)) {
            offset = stoi(offsetStr);
        } else {
            etiqueta = offsetStr;
        }

        int rs1 = reg(t[2].substr(p1 + 1, p2 - p1 - 1));
        int rd = reg(t[1]);

        programa.push_back(
            new InstruccionI(op, pc, cop, rs1, offset, rd, linea, etiqueta)
        );
        return;
    }

    // ---------- TIPO J ----------
    if (op == "JMP" || op == "BEQ" || op == "BNE" || op == "BGT" ||
        op == "JAL" || op == "RET" || op == "RETI" || op == "HALT") {

        int cop =
            (op == "JMP")  ? 0 :
            (op == "BEQ")  ? 1 :
            (op == "BNE")  ? 2 :
            (op == "BGT")  ? 3 :
            (op == "JAL")  ? 4 :
            (op == "RET")  ? 5 :
            (op == "RETI") ? 6 : 7;

        InstruccionJ* inst = new InstruccionJ(op, pc, cop, 0, linea);

        // Instrucciones que requieren destino
        if (op != "RET" && op != "RETI" && op != "HALT") {
            if (t.size() < 2) {
                cout << "Falta operando en instruccion: " << op << endl;
                exit(1);
            }
            if (esNumero(t[1])) {
                inst->setOffset((int8_t)stoi(t[1], nullptr, 0));
            } else {
                inst->setEtiquetaDestino(t[1]);
            }
        }

        programa.push_back(inst);
        return;
    }

    // ---------- PSEUDO-INSTRUCCIONES ----------
    if (op == "NOP") {
        string lineaTrad = "ADD R0, R0, R0";
        programa.push_back(
            new InstruccionR("ADD", pc, 0, 0, 0, 0, lineaTrad)
        );
        return;
    }

    if (op == "MOV") {
        string lineaTrad = "ADD " + t[1] + ", " + t[2] + ", R0";
        programa.push_back(
            new InstruccionR("ADD", pc, 0, reg(t[2]), 0, reg(t[1]), lineaTrad)
        );
        return;
    }

    if (op == "IMOV") {
        string lineaTrad = "ADDI " + t[1] + ", " + t[2] + ", R0";
        programa.push_back(
            new InstruccionI("ADDI", pc, 0, 0, stoi(t[2]), reg(t[1]), lineaTrad)
        );
        return;
    }

    // Error
    cout << "Instruccion desconocida: " << op << endl;
    exit(1);
}

// ------------------------------------------------------------
//              PRIMERA PASADA DEL ENSAMBLADOR
// ------------------------------------------------------------

void primeraPasada(const string& archivo) {
    ifstream in(archivo);
    if (!in) {
        cout << "No se pudo abrir el archivo\n";
        exit(1);
    }

    string linea;
    uint16_t pc = DIR_INICIAL;

    while (getline(in, linea)) {
        linea = limpiarLinea(linea);
        if (linea.empty()) continue;

        // Etiqueta
        if (linea.back() == ':') {
            tablaSimbolos[linea.substr(0, linea.size() - 1)] = pc; // dfdadf
            continue;
        }

        vector<string> tokens = tokenizar(linea);
        if (tokens.empty()) continue;

        // Directiva
        if (tokens[0][0] == '.') {
            procesarDirectiva(tokens, pc, linea);
            continue;
        }

        // Instruccion
        procesarInstruccion(tokens, pc, linea);
        pc++;
    }
}

// ------------------------------------------------------------
//              SEGUNDA PASADA DEL ENSAMBLADOR
//             Generacion del codigo de maquina
// ------------------------------------------------------------

void segundaPasada() {
    // itera sobre todas las instrucciones del programa y llama a su ensamblar ensamblar() en cada una
    for (Instruccion *&inst : programa) {
        inst->ensamblar();
    }
}

// ------------------------------------------------------------
//                       SIMULACION
// ------------------------------------------------------------

void simular(vector<Instruccion*>& programa, CPU &cpu, Memoria &mem) {
    // iteramos sobre todas las instrucciones para cargar los datos de las directivas en la memoria virtual
    for (Instruccion* inst : programa) {
        inst->cargarDatos(mem);
    }
    
    while (!cpu.halted) {
        Instruccion* inst = nullptr;

        // busqueda de la instruccion en la lista segun el PC actual
        for (Instruccion* i : programa) {
            if (i->getDireccion() == cpu.PC) {
                inst = i;
                break;
            }
        }

        if (!inst) {
            cout << "PC invalido: " << hex << cpu.PC << endl;
            break;
        }

        inst->ejecutar(cpu, mem); // cada instruccion llama a su funcion ejecutar() correspondiente segun el tipo
    }
}

// ------------------------------------------------------------
//                       SALIDA
// ------------------------------------------------------------

void guardarSalida(const vector<Instruccion*>& programa, const CPU &cpu, const Memoria &mem, const string& nombreArchivo) {
    ofstream archivo(nombreArchivo);

    if(!archivo.is_open()) {
        archivo.open(nombreArchivo);
    }

    if (tablaSimbolos.empty()) archivo << "TABLA DE SIMBOLOS VACIA" << endl;
    else archivo << "TABLA DE SIMBOLOS" << endl;

    for (auto& e : tablaSimbolos)  
        archivo << e.first << " -> " << hex << e.second << endl;

    archivo << "\nDireccion | Codigo Hex | # Instruccion" << endl;
    for (int i = 0; i < programa.size(); i++) {
        archivo << "0x" << hex4(programa[i]->getDireccion())
             << " | 0x" << hex4(programa[i]->getCodigo())
             << " | " << programa[i]->getInstruccion() << endl;
    }

    archivo << "\nINSTRUCCIONES LEIDAS: " << dec << programa.size() << endl;

    archivo << "\nREGISTROS:" << endl;
    for (int i = 0; i < 8; i++)
        archivo << "R" << i << " = 0x" << hex4(cpu.leer(i)) << endl;

    archivo << "\nPSR:" << endl;
    archivo << "Z=" << cpu.psr.Z
         << " N=" << cpu.psr.N
         << " C=" << cpu.psr.C
         << " V=" << cpu.psr.V << endl;

    // salida binaria
    archivo << "\nCodigo Objeto (Binario)" << endl;
    for (int i = 0; i < programa.size(); i++) {
        archivo << bin16(programa[i]->getCodigo()) << endl;
    }

    archivo.close();
}

// ------------------------------------------------------------
//                            MAIN
// ------------------------------------------------------------

int main(int argc, char** argv) {
    string archivo;

    if (argc >= 2) {
        archivo = argv[1];
    } else {
        cout << "Ingrese el nombre del archivo (.asm): ";
        cin >> archivo;
    }
    
    // Fase de Ensamblamiento
    primeraPasada(archivo);
    segundaPasada();
    
    // Fase de Simulación
    CPU cpu;
    Memoria mem;
    simular(programa, cpu, mem);

    // quitar extension de archivo y cambiarla a .txt
    archivo.erase(archivo.length() - 4);
    archivo += "_out.txt";

    // Imprimir resultado a archivo
    guardarSalida(programa, cpu, mem, archivo);

    return 0;
}