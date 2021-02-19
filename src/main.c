/**
 * Bran-CHIP 8
 * file: main.c  Copyright (C) 2021  Brandon Stevens
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <SDL2/SDL.h>

/* SDL */
SDL_Window *window;
SDL_Texture *texture;
SDL_Renderer *renderer;
#define SCALE 10 /* scale from screen to window */
#define SCREEN_WIDTH 64 /* screen buffer width */
#define SCREEN_HEIGHT 32 /* screen buffer height */
#define WINDOW_WIDTH SCREEN_WIDTH*SCALE /* window width */
#define WINDOW_HEIGHT SCREEN_HEIGHT*SCALE /* window height */

#define PRINT(s) printf(s)

/* calculate pixel value from single bit */
#define BIT_TO_PIXEL(bit) ((0xFFFFFF00 * (uint8_t)bit) | 0x000000FF)
/* calculate bit value from pixel */
#define PIXEL_TO_BIT(bit) (((uint8_t)bit ^ 0x000000FF) / 0xFFFFFF00)
/* map pointer to section in memory (will erase all data in that section) */
#define MAP_TO(ptr0, ptr1, start, size) ptr0 = (ptr1+start); memset(ptr0, 0, size)
/* get lower 8-bits of address */
#define LOW_8(address) (uint8_t)(address & 0xFF)
/* get upper 8-bits of address */
#define HIGH_8(address) (uint8_t)((address>>8) & 0xFF)
/* combine two 8-bit values (little endian) */
#define MAKE_16(a, b) (uint16_t)(((b & 0xFF) << 8) | (a & 0xFF))

/* instructions */
#define WRAP_SP(sp) ((sp==0)?15:sp-1)
#define WRAP_SPPU(sp) ((sp==15)?0:sp-1)
#define MAP_PC(pc) (pc-0x200)
#define POP() (stack[--SP]);if (SP>=16) SP=15
#define PUSH(v) (stack[SP++]=v);if(SP>=16) SP=0
#define RAND() rand()%256
#define INDEX(x, y) (x+SCREEN_WIDTH*y)
#define DISPLAY(x, y, ox, oy) display[INDEX((V[x]+ox<=0?SCREEN_WIDTH-1:(V[x]+ox>=SCREEN_WIDTH?0:V[x]+ox)),(V[y]+oy<=0?SCREEN_HEIGHT-1:(V[y]+oy>=SCREEN_HEIGHT?0:V[y]+oy)))]


#define KEY(i) (keypad[i])

#define HLT() hlt=1

#define CLS() PRINT("CLS\n");for(int i=0;i<SCREEN_WIDTH*SCREEN_HEIGHT;i++)display[i]=0;PC+=2
#define RET() PRINT("RET\n");PC=POP();PC+=2
#define JP(addr) PRINT("JP\n"); PC=MAP_PC(addr)
#define CALL(addr) PRINT("CALL\n");PUSH(PC); JP(addr)
#define SE(x, b) PRINT("SE\n");if(V[x]==b)PC+=2;PC+=2
#define SNE(x, b) PRINT("SNE\n");if(V[x]!=b)PC+=2;PC+=2
#define SER(x, y) PRINT("SER\n");if(V[x]==V[y])PC+=2;PC+=2
#define SNER(x, y) PRINT("SNER\n");if(V[x]!=V[y])PC+=2;PC+=2
#define LD(x, b) PRINT("LD\n");V[x]=b;PC+=2
#define ADD(x, b) PRINT("ADD\n");V[x]=V[x]+b;PC+=2
#define LDR(x, y) PRINT("LDR\n");V[x]=V[y];PC+=2
#define OR(x, y) PRINT("OR\n");V[x]=V[x]|V[y];PC+=2
#define AND(x, y) PRINT("AND\n");V[x]=V[x]&V[y];PC+=2
#define XOR(x, y) PRINT("XOR\n");V[x]=V[x]^V[y];PC+=2
#define ADDR(x, y, p) PRINT("ADDR\n");p=V[x]+V[y];V[0xF]=(p>=256?1:0);V[x]=(uint8_t)p;PC+=2
#define SUBR(x, y) PRINT("SUBR\n");V[0xF]=(V[x]>V[y]?1:0);V[x]=V[y]-V[x];PC+=2
#define SHR(x, y) PRINT("SHR\n");V[0xF]=V[x]&1;V[x]/=2;PC+=2
#define SUBN(x, y) PRINT("SUBN\n");V[0xF]=(V[y]>V[x]?1:0);V[x]=V[x]-V[y];PC+=2
#define SHL(x, y) PRINT("SHL\n");V[0xF]=V[x]&0x80;V[x]*=2;PC+=2
#define LDI(addr) PRINT("LDI\n");I=addr;PC+=2
#define JPO(addr) PRINT("JPO\n");JP(addr+V[0]);PC+=2
#define RND(x, b) PRINT("RND\n");V[x]=RAND()&b;PC+=2

#define DRW(x, y, n) PRINT("DRW\n");V[0xF]=0;\
for(int yc=0;yc<n;yc++){\
    uint16_t pix=ram[I+yc];\
    for(int xc=0;xc<8;xc++){\
        if((pix&(0x80>>xc))!=0){\
            if (DISPLAY(x,y,xc,yc) == 1){\
                V[0xF]=1;\
            }\
            DISPLAY(x,y,xc,yc)^=1;\
        }\
}}drawflag=1;PC+=2\


#define SKP(x) PRINT("SKP\n");if(KEY(V[x])==1) PC+=4; else PC+=2
#define SKNP(x) PRINT("SKNP\n");if(KEY(V[x])!=1) PC+=4; else PC+=2
#define LDT(x) PRINT("LDT\n");V[x]=dt;PC+=2
#define LDK(x) PRINT("LDK\n");for (int i = 0; i < 16; i++) { if(V[x]=keypad[i]) V[x]; }PC+=2
#define LDTT(x) PRINT("LDTT\n");dt=V[x];PC+=2
#define LDST(x) PRINT("LDST\n");st=V[x];PC+=2
#define ADDI(x) PRINT("ADDI\n");I=V[x]+I;PC+=2
#define LDF(x) PRINT("LDF\n");I=(5*(uint16_t)V[x]);PC+=2
#define LDB(x) PRINT("LDB\n");ram[I]=(V[x]/100);ram[I+1]=(V[x]/10);ram[I+2]=(V[x]*10);PC+=2
#define LDRX(x) PRINT("LDRX\n");for(int i=0;i<=x;i++) ram[I+i]=V[i];PC+=2
#define LDRRX(x) PRINT("LDRRX\n");for(int i=0;i<=x;i++) V[i]=ram[I+i];PC+=2

/* memory map */
#define RAM_START  0x0000
#define ROM_START  0x0200
#define RAM_END    0x0FFF
#define RAM_SIZE   0x1000
#define ROM_SIZE   0x0E00
#define FONT_START 0x0000
#define FONT_END   0x004F
#define FONT_SIZE  0x0050
#define RES_START  0x0000
#define RES_END    0x01FF
#define RES_SIZE   0x0200

/* ram */
uint8_t ram[RAM_SIZE];
/* rom */
uint8_t *rom; /* mapped to 0x200-0xFFF of RAM */
/* fontset */
uint8_t fontset[0x50] = {
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
    0xF0, 0x90, 0xF0, 0x90, 0x90, // a
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // b
    0xF0, 0x80, 0x80, 0x80, 0xF0, // c
    0xE0, 0x90, 0x90, 0x90, 0xE0, // d
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // e
    0xF0, 0x80, 0xF0, 0x80, 0x80, // f
};
/* reserved */
uint8_t *res;
/* stack */
uint16_t stack[0xF];

/* video memory */
uint8_t display[SCREEN_WIDTH*SCREEN_HEIGHT];
uint32_t sdl_display[SCREEN_WIDTH*SCREEN_HEIGHT];

/* general purpose 8-bit regs */
uint8_t V[0xF];
/* memory register */
uint16_t I;
/* sound timer */
uint8_t st;
/* delay timer */
uint8_t dt;
/* program counter */
uint16_t PC;
/* stack pointer */
uint8_t SP;

/* is the system halted? */
uint8_t hlt;
/* last key pressed */
uint8_t key;

uint8_t drawflag;

/* chip8 keypad layout */
const unsigned char keymap[16] = {
    '1','2','3','4', 
    'q','w','e','r', 
    'a','s','d','f', 
    'z','x','c','v'
};
unsigned int keypad[16];

/* initialize chip8 */
void chip_initialize( void );
/* print chip8 memory map */
void print_memory_map( void );
/* print preview of rom file */
void print_rom( void );
/* take a snapshot of ram */
void ram_snapshot( void );

/* initialize display */
void initialize_sdl( void );
/* update display */
void sdl_update( void );
/* quit sdl */
void sdl_close( void );


/* execute next instruction in ROM */
void update_cpu( void ) {
    uint16_t inst = (rom[PC]<<8)|rom[PC+1];
    uint8_t x = (inst & 0x0F00) >> 8;
    uint8_t y = (inst & 0x00F0) >> 4;
    uint16_t nnn = inst & 0x0FFF;
    uint8_t kk = inst & 0x00FF;
    uint8_t n = inst & 0x000F;

    //printf("kk: %x\n", kk);
    uint16_t p;

    switch ((inst&0xF000)>>12) {
        case 0:
            if (n == 0) {
                CLS();
            } else if(n == 0xE) {
                RET();
            }
            break;
        case 1:
            JP(nnn);
            break;
        case 2:
            CALL(nnn);
            break;
        case 3:
            SE(x,kk);
            break;
        case 4:
            SNE(x,kk);
            break;
        case 5:
            SER(x,y);
            break;
        case 6:
            LD(x,kk);
            break;
        case 7:
            ADD(x,kk);
            break;
        case 8:
            switch (n) {
                case 0:
                    LDR(x,y);
                    break;
                case 1:
                    OR(x,y);
                    break;
                case 2:
                    AND(x,y);
                    break;
                case 3:
                    XOR(x,y);
                    break;
                case 4:
                    ADDR(x,y,p);
                    break;
                case 5:
                    SUBR(x,y);
                    break;
                case 6:
                    SHR(x,y);
                    break;
                case 7:
                    SUBN(x,y);
                    break;
                case 0xE:
                    SHL(x,y);
                    break;
            }
            break;
        case 9:
            SNER(x,y);
            break;
        case 0xA:
            LDI(nnn);
            break;
        case 0xB:
            JPO(nnn);
            break;
        case 0xC:
            RND(x,kk);
            break;
        case 0xD:
            DRW(x,y,n);
            break;
        case 0xE:
            if (kk==0x9E) {
                SKP(x);
            } else if (kk==0xA1) {
                SKNP(x);
            }
            break;
        case 0xF:
            if (kk==0x7) { LDT(x); }
            else if (kk==0xA) { LDK(x); }
            else if (kk==0x15) { LDTT(x); }
            else if (kk==0x18) { LDST(x); }
            else if (kk==0x1E) { ADDI(x); }
            else if (kk==0x29) { LDF(x); }
            else if (kk==0x33) { LDB(x); }
            else if (kk==0x55) { LDRX(x); }
            else if (kk==0x65) { LDRRX(x); }
            break;
    }
    for (int i = 0; i < 16; i++) printf("R%i: 0x%x\n", i, V[i]);
    printf("PC: %i, SP:%i 0x%x, I:%x\n", PC, SP, inst, I);
    printf("\n");
}

void display_snapshot( void );

int main(int argc, char *argv[]) {
    /* initialize the chip8 */
    chip_initialize();

    FILE *fp;

    /* load rom file */
    load_rom_file(fp, argv[argc-1]);
    /* init sdl */
    initialize_sdl();

    /* print rom contents */
    print_rom();
    /* print the memory map */
    print_memory_map();

    /* main loop */
    int quit = 0;
    int cycles = 1;
    while (!quit) {
        if (cycles%16==0 && dt>0) dt--;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit=1;
            if (e.type == SDL_KEYDOWN) {
                for (int i = 0; i < 16; i++) {
                    if (keymap[i] == e.key.keysym.sym) {
                        keypad[i] = 1;
                        hlt = 0;
                    }
                }
            } else if (e.type == SDL_KEYUP) {
                for (int i = 0; i < 16; i++) {
                    if (keymap[i] == e.key.keysym.sym) {
                        keypad[i] = 0;
                    }
                }
            }
        }
        
        if (!hlt)
            update_cpu();
        if (drawflag) {
            for (int x = 0; x < SCREEN_WIDTH*SCREEN_HEIGHT; x++) {
                sdl_display[x] = BIT_TO_PIXEL(display[x]);
            }
            drawflag=0;
            sdl_update();
        }
        cycles++;
        usleep(1000);
    }

    display_snapshot();

    ram_snapshot();

    sdl_close();
}


void load_rom_file(FILE *fp, const char *fname) {
    fp = fopen(fname, "rb");
    memset(rom, 0, ROM_SIZE);
    if (fp == NULL) {
        printf("ERROR: ROM FILE '%s' DOES NOT EXIST.\n", fname);
        exit(-1);
    }
    printf("Successfuly opened '%s'.\n", fname);
    printf("Reading from binary file...\n");
    const size_t fsize = fread(rom, sizeof(uint8_t), ROM_SIZE, fp);
    printf("Successfuly read %d bytes (%dkb) from '%s'\n", fsize, fsize/1024, fname);
    printf("Now closing file\n\n");
    fclose(fp);
}

void chip_initialize( void ) {
    srand(0); /* seed rand */
    memset(ram, 0, RAM_SIZE*sizeof(uint8_t));
    memset(stack, 0, 0xF*sizeof(uint16_t));
    memset(V, 0, 0xF*sizeof(uint8_t));
    st = 0;
    dt = 0;
    PC = 0;
    SP = 0;
    /* map rom to ram starting at 0x200 */
    MAP_TO(rom, ram, ROM_START, ROM_SIZE);
    /* map reserved space to ram */
    MAP_TO(res, ram, RES_START, RES_SIZE);

    /* copy fontset to ram */
    memcpy(res, fontset, FONT_SIZE);
}

void print_memory_map( void ) {
    printf("RAM:    0x%04X - 0x%04X   (0x%05X)  (%ikb)\n", RAM_START, RAM_END, RAM_SIZE, RAM_SIZE/1024);
    printf("FONT:   0x%04X - 0x%04X   (0x%05X)  (0kb)\n", FONT_START, FONT_END, FONT_SIZE);
    printf("ROM:    0x%04X - 0x%04X   (0x%05X)  (%ikb)\n", ROM_START, RAM_END, RAM_SIZE-ROM_START, (RAM_SIZE-ROM_START)/1024);
    printf("STACK:  0x%04X - 0x%04X   (0x%05X)  (0kb)\n", 0, 31, 32);
    printf("REG:    0x%04X - 0x%04X   (0x%05X)  (0kb)\n\n", 0, 15, 16);
}

void print_rom( void ) {
    printf("ROM Preview:\n");
    for (size_t i = 0; i < 64; i++) {
        if (i % 16 == 0) printf("0x%08x  ", i);
        printf("0x%x ", rom[i]);
        if ((i+1) % 16 == 0) printf("\n");
    }
    printf("[...]\n");
    printf("0x%08x\n\n", ROM_SIZE);
}

void ram_snapshot( void ) {
    FILE *fp = fopen("ram.txt", "w");
    for (size_t i = 0; i < RAM_SIZE; i++) {
        if (i % 16 == 0) fprintf(fp, "0x%08x  ", i);
        fprintf(fp, "0x%02x ", ram[i]);
        if ((i+1) % 16 == 0) fprintf(fp, "\n");
    }
    fclose(fp);
}

void display_snapshot( void ) {
    FILE *fp = fopen("display.txt", "w");
    for (size_t i = 0; i < SCREEN_WIDTH*SCREEN_HEIGHT; i++) {
        fprintf(fp, "0x%02x ", display[i]);
        if ((i+1) % SCREEN_WIDTH == 0) fprintf(fp, "\n");
    }
    fclose(fp);
}

void initialize_sdl( void ) {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, 0);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 64, 32);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
}

void sdl_update( void ) {
    SDL_UpdateTexture(texture, NULL, sdl_display, 64 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void sdl_close( void ) {
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyTexture(texture);
    SDL_Quit();
}
