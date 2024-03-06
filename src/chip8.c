#include "chip8.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

const short display_size = 64*32; 
 
#define debug_print(fmt, ...)                        \
    do {                                             \
        if (DEBUG) fprintf(stderr, fmt, __VA_ARGS__); \
    }while (0);

int DEBUG = 1;
extern int errno;

/*
    ====================================
            ARQUITECTURA CHIP-8
    ====================================
*/

//Fontset
unsigned char fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

//Memoria
unsigned char memory[4096] = {0};

/*
    Registros:
        * 16 registros de propósito general, que se les 
        * suele llamar por Vx, siendo x un numero hex.
        * 
        * un char son 8 bits, asi que guardamos 16 de ellos
*/
unsigned char V[16] = {0};

/*
    Registro especial llamado I, que se usa para
    almacenar las direcciones de memoria.

    Un short son 16 bits;
*/
unsigned short I = 0;

/*
    Short para almacenar PC, las instrucciones 
    empiezan en la dir. de memoria 0x200
*/
unsigned short pc = 0x200;

/*
    Stack Pointer: usado para apuntar al nivel más
    alto de la pila

    Es un registro de 8 bits
*/
unsigned char sp = 0;


/*
    Stack(Pila): Array de 16 valores de 16 bits,
    se usa para guardar los valores que el intérprete
    debe devolver al terminar una función/subrutina
*/
unsigned short stack[16] = {0};

//Keypad
unsigned char keypad[16] = {0};

//Display
unsigned char display[64*32] = {0};

//Delay Timer
unsigned char dt = 0;

//Sound Timer
unsigned char st = 0;

//Actualizar Display Flag
unsigned char draw_flag = 0;

//Play a sound flag
unsigned char sound_flag = 0;

/*
    ===============================
            LÓGICA CHIP-8
    ===============================
*/


//Inicializar la CPU cargando el fontset en memoria
void init_cpu(void){
    srand((unsigned int)time(NULL));
    //Cargar fonts en memoria
    memcpy(memory, fontset, sizeof(fontset));
}

//Cargar la rom en memoria
int load_rom(char *filename){
    FILE *fp = fopen(filename, "rb");

    if(fp == NULL) return errno;

    struct stat st;
    stat(filename, &st);
    size_t fsize = st.st_size;

    size_t bytes_read = fread(memory + 0x200, 1, sizeof(memory) - 0x200, fp);

    fclose(fp);

    if(bytes_read != fsize){
        return -1;
    }

    return 0;
}


/*
    Ciclo de emulación (3 pasos en bucle:
        -Fetch la instrucción de la memoria en el PC actual
        -Decode la instrucción para ejeutarla
        -Execute cada instrucción
*/
void ciclo_emulacion(void){
    draw_flag = 0;
    sound_flag = 0;

    unsigned short op_code = memory[pc] << 8 | memory[pc +1];

    //Registro Vx, basicamente coge las X presentes en instrucciones
    unsigned short x = (op_code & 0x0F00) >> 8;

    //Registro Vy, lo mismo que Vx pero con las Y
    unsigned short y = (op_code & 0x00F0) >> 4;

    switch(op_code & 0xF000){
        case 0x0000:
            switch(op_code & 0x00FF){
                case 0x00E0:
                    debug_print("[OK] 0x%X: 00E0\n", op_code);
                    for(int i = 0; i < display_size; i++){
                        display[i] = 0;
                    }
                    pc += 2;
                    break;
                case 0x00EE:
                    debug_print("[OK] 0x%X: 00EE\n", op_code);
                    pc = stack[sp];
                    sp--;
                    pc+= 2;
                    break;
                default:
                    debug_print("[FAILED] Unknown opcode: 0x%X", op_code);
            }
        case 0x1000:
            debug_print("[OK] 0x%X: 1NNN\n", op_code);
            pc = op_code & 0x0FFF;
            break;
        case 0x2000:
            debug_print("[OK] 0x0%X: 2NNN\n", op_code);
            sp++;
            stack[sp] = pc;
            pc = op_code & 0x0FFF;
            break;
        case 0x3000:
            debug_print("[OK] 0x0%X: 3XNN\n", op_code);
            if(V[x] == (op_code & 0x00FF)){
                pc += 2;
            }
            pc += 2;
            break;
        case 0x4000:
            debug_print("[OK] 0x0%X: 4XNN\n", op_code);
            if(V[x] != (op_code & 0x00FF)){
                pc += 2;
            }
            pc += 2;
            break;
        case 0x5000:
            debug_print("[OK] 0x0%X: 5XY0\n", op_code);
            if(V[x] == V[y]){
                pc += 2;
            }
            pc += 2;
            break;
        case 0x6000:
            debug_print("[OK] 0x0%X: 6XNN\n", op_code);
            V[x] = (op_code & 0x00FF);
            pc += 2;
            break;
        case 0x7000:
            debug_print("[OK] 0x0%X: 7XNN\n", op_code);
            V[x] += (op_code & 0x00FF);
            pc += 2;
            break;
        case 0x8000:
            switch (op_code & 0x000F){
                case 0x0000:
                    debug_print("[OK] 0x%X: 8XY0\n", op_code);
                    V[x] = V[y];
                    pc += 2;
                    break;
                case 0x0001:
                    debug_print("[OK] 0x%X: 8XY1\n", op_code);
                    V[x] = (V[x] | V[y]);
                    pc += 2;
                    break;
                case 0x0002:
                    debug_print("[OK] 0x%X: 0x8XY2\n", op_code);
                    V[x] = (V[x] & V[y]);
                    pc += 2;
                    break;
                case 0x0003:
                    debug_print("[OK] 0x%X: 0x8XY3\n", op_code);
                    V[x] = (V[x] ^ V[y]);
                    pc += 2;
                    break;
                case 0x0004:
                    debug_print("[OK] 0x%X: 0x8XY4\n", op_code);
                    V[x] = V[x] + V[y];
                    pc += 2;
                    break;
                case 0x0005:
                    debug_print("[OK] 0x%X: 0x8XY5\n", op_code);
                    V[0xF] = (V[x] > V[y]) ? 1 : 0;
                    V[x] -= V[y];
                    pc += 2;
                    break;
                case 0x0006:
                    debug_print("[OK] 0x%X: 0x8XY6\n", op_code);
                    V[0xF] = V[x] & 0x1;
                    V[x] = (V[x] >> 1);
                    pc += 2;
                    break;
                case 0x0007:
                    debug_print("[OK] x%X: 0x8XY7\n", op_code);
                    V[0xF] = (V[x] < V[y]) ? 1 : 0;
                    V[x] = V[y] - V[x];
                    pc += 2;
                    break;
                case 0x000E:
                    debug_print("[OK] 0x%X: 0x8XYE\n", op_code);
                    V[0xF] = (V[x] >> 7) & 0x1;
                    V[x] = (V[x] << 1);
                    pc += 2;
                    break;
                default:
                    debug_print("[FAILED] Unknown op_code: 0x%X\n", op_code);
                    break;
            }
            break;
        case 0x9000:
            debug_print("[OK] 0x0%X: 9XY0\n", op_code);
            if(V[x] != V[y]){
                pc += 2;
            }
            pc += 2;
            break;
        case 0xA000:
            debug_print("[OK] 0x%X: 0xANNN\n", op_code);
            I = op_code & 0x0FFF;
            pc += 2;
            break;
        case 0xB000:
            debug_print("[OK] 0x%X: 0xBNNN", op_code);
            pc = (op_code & 0x0FFF) + V[0];
            break;
        case 0xC000:
            debug_print("[OK] 0x%X: 0xCXNN\n", op_code);
            V[x] = (rand() % 256) & (op_code & 0x00FF);
            pc += 2;
            break;
        //La siguiente instrucción es el Display, se viene tocho papus
        case 0xD000:
            debug_print("[OK] 0x%X: 0xDXYN\n", op_code);
            unsigned short height = op_code & 0x000F;
            unsigned short px;

            //Ajustar flag de colisión a 0
            V[0xF] = 0;

            //Para cada fila
            for(int yline = 0; yline < height; yline++){
                px = memory[I + yline];
                //Para cada pixel de una fila
                for(int xline = 0; xline < 8; xline++){
                    if((px & (0x80 >> xline)) != 0){
                        if(display[(V[x] + xline + ((V[y] + yline)*64)) == 1]){
                            V[0xF] = 1;
                        }
                        display[V[x] + xline + ((V[y] + yline) * 64)] ^= 1;
                    }
                }
            }
            pc += 2;
            break;
        case 0xE000:
            switch(op_code & 0x00FF){
                case 0x009E:
                    debug_print("[OK] 0x%X: 0xEX9E\n", op_code);
                    if(keypad[V[x]]){
                        pc += 2;
                    }
                    pc += 2;
                    break;
                case 0x00A1:
                    debug_print("[OK] 0x%X: 0xEXA1\n", op_code);
                    if(!keypad[V[x]]){
                        pc += 2;
                    }
                    pc += 2;
                    break;
                default:
                    debug_print("[FAILED] Unknown op_code: 0x%X\n", op_code);
            }
            break;
        case 0xF000:
            switch(op_code & 0x00FF){
                case 0x0007:
                    debug_print("[OK] 0x%X: 0xFX07\n", op_code);
                    V[x] = dt;
                    pc += 2;
                    break;
                case 0x0015:
                    debug_print("[OK] 0x%X: 0xFX15\n", op_code);
                    dt = V[x];
                    pc += 2;
                    break;
                case 0x0018:
                    debug_print("[OK] 0x0%X: 0xF18\n", op_code);
                    st = V[x];
                    pc += 2;
                    break;
                case 0x001E:
                    debug_print("[OK] 0x%X: 0xFX1E\n", op_code);
                    I += V[x];
                    pc += 2;
                    break;
                case 0x000A:
                    debug_print("[OK] 0x%X: 0xFX0A\n", op_code);
                    for(int i = 0; i < 16; i++){
                        if(keypad[i]){
                            V[x] = i;
                            pc += 2;
                            break;
                        }
                    }
                    break;
                case 0x0029:
                    debug_print("[OK] 0x%X: 0xFX29\n", op_code);
                    I = V[x] * 5;
                    pc += 2;
                    break;
                case 0x0033:
                    debug_print("[OK] 0x%X: 0xFX33\n", op_code);
                    memory[I] = (V[x] % 1000) / 100;
                    memory[I+1] = (V[x] % 100) / 10;
                    memory[I+2] = (V[x] % 10);
                    pc += 2;
                    break;
                case 0x0055:
                    debug_print("[OK] 0x%X: 0xFX55\n", op_code);
                    for(int i = 0; i <= x; i++){
                        memory[I + i] = V[i];
                    }
                    pc += 2;
                    break;
                case 0x0065:
                    debug_print("[OK] 0x%X: 0xFX65\n", op_code);
                    for(int i = 0; i <= x; i++){
                        V[i] = memory[I +i];
                    }
                    pc += 2;
                    break;
                default:
                    debug_print("[FAILED] Unknown op_code: 0x%X\n", op_code);
                    break;
            }
            break;
        default:
            debug_print("[FAILED] Unknown op_code: 0x%X", op_code);
            break;
    }

    //Timers
    if(dt > 0) dt -= 1;
    if(st > 0){
        sound_flag = 1;
        puts("BEEP");
        st -= 1;
    }
}
