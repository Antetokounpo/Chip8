#include<fstream>
#include<iostream>
#include<cstdlib>
#include<cmath>
#include<SDL2/SDL.h>

using namespace std;

class Chip8
{
    public:
        Chip8(){}
        ~Chip8(){}
        bool load(const char* filename);
        void init();
        void step();
        void execute(char16_t opcode);
        void updateKeyboard();
        int* getDisplay();

    private:
        /* Memory duh */
        char memory[4096];
        /* Registers */
        char v[16]; /* Registers classiques */
        char16_t i; /* Sert généralement à stocker des addresses mémoires */
        char16_t pc; /* Program counter */
        char sp; /* Stack pointer */
        /* Memory stack */
        char16_t stack[16];

        /* 64x32 monochrome display */
        int display[32][64] = {};
        /* 16-key hexadecimal keypad */
        int keyboard[16];

        /* Timer registers */
        char dt;
        char st;
        Uint32 lastdt = SDL_GetTicks();
        Uint32 lastst = SDL_GetTicks();

        /* Opcodes */
        void opcode0(char16_t opcode);
        void opcode1(char16_t opcode);
        void opcode2(char16_t opcode);
        void opcode3(char16_t opcode);
        void opcode4(char16_t opcode);
        void opcode5(char16_t opcode);
        void opcode6(char16_t opcode);
        void opcode7(char16_t opcode);
        void opcode8(char16_t opcode);
        void opcode9(char16_t opcode);
        void opcodeA(char16_t opcode);
        void opcodeB(char16_t opcode);
        void opcodeC(char16_t opcode);
        void opcodeD(char16_t opcode);
        void opcodeE(char16_t opcode);
        void opcodeF(char16_t opcode);
};

void Chip8::init()
{
/* 
    Example for char "0"

        "0"       Binary      Hex
    |------------------------------|    
    |  ****  |  11110000  |  0xF0  | 
    |  *  *  |  10010000  |  0x90  |  
    |  *  *  |  10010000  |  0x90  |  
    |  *  *  |  10010000  |  0x90  |  
    |  ****  |  11110000  |  0xF0  |
    |------------------------------|
*/

    /* 16 x 5 bytes font */
    char font[80] = {
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
        0xF0, 0x80, 0xF0, 0x80, 0x80, // F
    };

    /* Load it in reserved memory */
    for(int c  = 0;c<80; c++){
            memory[c] = font[c];
    }

    /* Set program counter to the start of the program */
    pc = 0x200;
}

bool Chip8::load(const char* filename)
{
    streampos size;
    char* buffer;

    init(); /* Initialise l'émulateur avant de loader le programme */

    ifstream file(filename, ios::binary|ios::ate);
    if(file.is_open()){
        size = file.tellg();
        if(size > sizeof(memory)-0x200){printf("File is too big\n"); return false;}
        buffer = new char[size];
        
        file.seekg(0, ios::beg);
        file.read(buffer, size);
        file.close();

        for(int c = 0; c<size; c++){
            memory[0x200 + c] = buffer[c];
        }
    
    delete[] buffer;
    return true;

    }else{
        printf("No such file: %s\n", filename);
        return false;
    }
}

void Chip8::step()
{
    char16_t opcode;
    
    /* Update le state du keyboard, à savoir les touches enfoncées */
    updateKeyboard();
    
    /* 
    opcode = 0x61
    opcode<<8 = 0x6100 
    0x6100 | 0x62 = 0x6162 
    */
    /* Les instructions sont toutes 2 octets */
    opcode = memory[pc]<<8 | memory[pc+1];
    /* Exécute opcode */
    execute(opcode);

    /* Décrémente les registres de temps, à 60 Hz */
    if(st && SDL_GetTicks() - lastst >= 1000/60)
    {
        st--;
        lastst = SDL_GetTicks();
    }
    if(dt && SDL_GetTicks() - lastdt >= 1000/60)
    {
        dt--;
        lastdt = SDL_GetTicks();
    }

    /* Incrémente de 2, car les instructions sont 2 octets */
    pc += 2;

    /* DEBUG */
    //cout << hex << opcode << endl;
    //for(int r = 0; r<16; r++) {printf("V%d: %d ", r, v[r]);}
    //cout << endl;
    //cout << "PC: " << pc << endl;
    //cout << "I: " << hex << i << endl;
    //for(int y = 0; y<32; y++) {for(int x = 0; x<64; x++) {cout << display[y][x] << " ";} cout << endl;}
}

void Chip8::updateKeyboard()
{
    /*
        Chip8 keypad
         ­­­_______________
        | 1 | 2 | 3 | C |
        | 4 | 5 | 6 | D |
        | 7 | 8 | 9 | E |
        | A | 0 | B | F |
         ¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
          1   2   3   4
          Q   W   E   R
          A   S   D   F
          Z   X   C   V
    
    */

    for(int c = 0; c<16; c++) keyboard[c] = 0;
    const Uint8* state = SDL_GetKeyboardState(NULL);

    if(state[SDL_SCANCODE_1])
        keyboard[0x1] = 1;
    if(state[SDL_SCANCODE_2])
        keyboard[0x2] = 1;
    if(state[SDL_SCANCODE_3])
        keyboard[0x3] = 1;
    if(state[SDL_SCANCODE_4])
        keyboard[0xC] = 1;
    if(state[SDL_SCANCODE_Q])
        keyboard[0x4] = 1;
    if(state[SDL_SCANCODE_W])
        keyboard[0x5] = 1;
    if(state[SDL_SCANCODE_E])
        keyboard[0x6] = 1;
    if(state[SDL_SCANCODE_R])
        keyboard[0xD] = 1;
    if(state[SDL_SCANCODE_A])
        keyboard[0x7] = 1;
    if(state[SDL_SCANCODE_S])
        keyboard[0x8] = 1;
    if(state[SDL_SCANCODE_D])
        keyboard[0x9] = 1;
    if(state[SDL_SCANCODE_F])
        keyboard[0xE] = 1;
    if(state[SDL_SCANCODE_Z])
        keyboard[0xA] = 1;
    if(state[SDL_SCANCODE_X])
        keyboard[0x0] = 1;
    if(state[SDL_SCANCODE_C])
        keyboard[0xB] = 1;
    if(state[SDL_SCANCODE_V])
        keyboard[0xF] = 1;
}

int* Chip8::getDisplay()
{
    return (int*)display;
}

void Chip8::execute(char16_t opcode)
{
    switch(opcode & 0xF000)
    {
        case 0x0000:
            opcode0(opcode); break;
        case 0x1000:
            opcode1(opcode); break;
        case 0x2000:
            opcode2(opcode); break;
        case 0x3000:
            opcode3(opcode); break;
        case 0x4000:
            opcode4(opcode); break;
        case 0x5000:
            opcode5(opcode); break;
        case 0x6000:
            opcode6(opcode); break;
        case 0x7000:
            opcode7(opcode); break;
        case 0x8000:
            opcode8(opcode); break;
        case 0x9000:
            opcode9(opcode); break;
        case 0xA000:
            opcodeA(opcode); break;
        case 0xB000:
            opcodeB(opcode); break;
        case 0xC000:
            opcodeC(opcode); break;
        case 0xD000:
            opcodeD(opcode); break;
        case 0xE000:
            opcodeE(opcode); break;
        case 0xF000:
            opcodeF(opcode); break;
    }
}

void Chip8::opcode0(char16_t opcode)
{
    switch(opcode)
    {
        /* CLS */
        case 0x00E0:
            for(int y = 0; y<32; y++) {for(int x = 0; x<64; x++) {display[y][x] = 0;}}
            break;
        /* RET */
        case 0x00EE:
            pc = stack[sp];
            pc -= 2; /* Le PC ne doit pas se faire incrémenter avec cette instruction */
            sp--;
            break;
    }
}

void Chip8::opcode1(char16_t opcode)
{
    /* 1nnn - JP nnn */
    pc = opcode & 0x0FFF;
    pc -= 2; /* Le PC ne doit pas se faire incrémenter avec cette instruction */
}

void Chip8::opcode2(char16_t opcode)
{
    /* 2nnn - CALL nnn */
    sp++;
    stack[sp] = pc+2;
    pc = opcode & 0x0FFF;
    pc -= 2; /* Le PC ne doit pas se faire incrémenter avec cette instruction */
}

void Chip8::opcode3(char16_t opcode)
{
    /* 3xkk - SE Vx, kk */
    char x = (opcode & 0x0F00) >> 8;
    char kk = opcode & 0x00FF;

    if(v[x] == kk)
        pc += 2;
}

void Chip8::opcode4(char16_t opcode)
{
    /* 4xkk - SNE Vx, kk */
    char x = (opcode & 0x0F00) >> 8;
    char kk = opcode & 0x00FF;

    if(v[x] != kk)
        pc += 2;
}

void Chip8::opcode5(char16_t opcode)
{
    /* 5xy0 - SE Vx, Vy */
    char x = (opcode & 0x0F00) >> 8;
    char y = (opcode & 0x00F0) >> 4;

    if(v[x] == v[y])
        pc += 2;
}

void Chip8::opcode6(char16_t opcode)
{
    /* 6xkk - LD Vx, kk */
    char x = (opcode & 0x0F00) >> 8;
    char kk = opcode & 0x00FF;

    v[x] = kk;
}

void Chip8::opcode7(char16_t opcode)
{
    /* 7xkk - ADD Vx, kk */
    char x = (opcode & 0x0F00) >> 8;
    char kk = opcode & 0x00FF;

    v[x] += kk;
}

void Chip8::opcode8(char16_t opcode)
{
    /* 8xyk */
    char x = (opcode & 0x0F00) >> 8;
    char y = (opcode & 0x00F0) >> 4;

    switch(opcode & 0x000F) /* k */
    {
        /* 8xy0 - LD Vx, Vy */
        case 0:
            v[x] = v[y]; break;
        /* 8xy1 - OR Vx, Vy */
        case 1:
            v[x] |= v[y]; break;
        /* 8xy2 - AND Vx, Vy */
        case 2:
            v[x] &= v[y]; break;
        /* 8xy3 - XOR Vx, Vy */
        case 3:
            v[x] ^= v[y]; break;
        /* ADD Vx, Vy */
        case 4:
            if(v[x] + v[y] > 255) {v[0xF] = 1; v[x] = v[x] + v[y] & 0xFF;} else {v[0xF] = 0; v[x] += v[y];} break;
        /* SUB Vx, Vy */
        case 5:
            if(v[x] > v[y]) {v[0xF] = 1; v[x] = v[y] - v[x];} else {v[0xF] = 0; v[x] -= v[y];} break;
        /* SHR Vx {, Vy} */
        case 6:
            v[0xF] = v[x] & 0x1; v[x] >>= 1; break;
        /* SUBN Vx, Vy */
        case 7:
            if(v[y] > v[x]) {v[0xF] = 1; v[x] = v[x] - v[y];} else {v[0xF] = 0; v[x] = v[y] - v[x];} break;
        /* SHL Vx {, Vy} */
        case 0xE:
            v[0xF] = v[x] & 0x1; v[x] <<= 1; break;
    }
}

void Chip8::opcode9(char16_t opcode)
{
    /* 9xy0 - SNE Vx, Vy */
    char x = (opcode & 0x0F00) >> 8;
    char y = (opcode & 0x00F0) >> 4;

    if(v[x] != v[y])
        pc += 2;
}

void Chip8::opcodeA(char16_t opcode)
{
    /* Annn - LD I, nnn */
    i = opcode & 0x0FFF;
}

void Chip8::opcodeB(char16_t opcode)
{
    /* Bnnn - JP v0, nnn */
    pc = (opcode & 0x0FFF) + v[0];
}

void Chip8::opcodeC(char16_t opcode)
{
    /* Cxkk - RND Vx, kk */
    char x = (opcode & 0x0F00) >> 8;
    char kk = opcode & 0x00FF;
    char rb = rand() % 256;

    v[x] = rb & kk;
}

void Chip8::opcodeD(char16_t opcode)
{
    /* Dxyn - DRW Vx, Vy, n */
    /* Dessine le sprite aux coord Vx, Vy
       Chaque sprite est 8 bits de large et
       n de haut. Les sprite sont XOR avec les
       pixels existant. Si un pixel se fait effacé, VF
       est set à 1, sinon 0. 
    */

    char x = (opcode & 0x0F00) >> 8;
    char y = (opcode & 0x00F0) >> 4;
    char n = opcode & 0x000F;

    char sprite;
    int oldPixel;
    int newPixel;
    v[0xF] = 0;

    for(int yoffset = 0; yoffset<n; yoffset++)
    {
        char sprite = memory[i+yoffset]; /* Ça marche par ligne de 8 bits */
        for(int xoffset = 0; xoffset<8; xoffset++)
        {
            oldPixel = display[(v[y] + yoffset) % 32][(v[x] + xoffset) % 64];
            newPixel = oldPixel ^ bool(sprite & (0x1 << (7-xoffset)));
            if(oldPixel && !newPixel) v[0xF] = 1; /* If bit flips from set to unset */
            display[(v[y] + yoffset) % 32][(v[x] + xoffset) % 64] = newPixel;
        }
    }   
}

void Chip8::opcodeE(char16_t opcode)
{
    /* ExKK */
    char x = (opcode & 0x0F00) >> 8;

    switch(opcode & 0x00FF)
    {   
        /* SKP Vx */
        case 0x9E:
            if(keyboard[x]) pc += 2; break;
        /* SKNP Vx */
        case 0xA1:
            if(!keyboard[x]) pc += 2; break;
    }
}

void Chip8::opcodeF(char16_t opcode)
{
    /* FxKK */
    char x = (opcode & 0x0F00) >> 8;

    switch(opcode & 0x00FF)
    {
        /* Fx07 - LD Vx, DT */
        case 0x07:
            v[x] = dt; break;
        /* Fx0A - LD Vx, K */
        case 0x0A:
        {
            /* Attend qu'une touche soit enfoncée, grâce à la magie de SDL */
            char key = 16;

            while(key == 16)
            {
                SDL_Event e;
                while(SDL_PollEvent(&e)){
                    if(e.type == SDL_QUIT)
                        exit(0);
                    if(e.type == SDL_KEYDOWN){
                        switch(e.key.keysym.sym)
                        {
                            case SDLK_1:
                                key = 0x1; break;
                            case SDLK_2:
                                key = 0x2; break;
                            case SDLK_3:
                                key = 0x3; break;
                            case SDLK_4:
                                key = 0xC; break;
                            case SDLK_q:
                                key = 0x4; break;
                            case SDLK_w:
                                key = 0x5; break;
                            case SDLK_e:
                                key = 0x6; break;
                            case SDLK_r:
                                key = 0xD; break;
                            case SDLK_a:
                                key = 0x7; break;
                            case SDLK_s:
                                key = 0x8; break;
                            case SDLK_d:
                                key = 0x9; break;
                            case SDLK_f:
                                key = 0xE; break;
                            case SDLK_z:
                                key = 0xA; break;
                            case SDLK_x:
                                key = 0x0; break;
                            case SDLK_c:
                                key = 0xB; break;
                            case SDLK_v:
                                key = 0xF; break;
                        }
                    }
                }
            }

            v[x] = key; break;
        }
        /* Fx15 - LD DT, Vx */
        case 0x15:
            dt = v[x]; break;
        /* Fx18 - LD ST, Vx */
        case 0x18:
            st = v[x]; break;
        /* Fx1E - ADD I, Vx */
        case 0x1E:
            i += v[x]; break;
        /* Fx29 - LD F, Vx */
        case 0x29:
            i = 5*v[x]; break;
        /* Fx33 - LD B, Vx */
        case 0x33:
            for(int c = 0; c<3; c++) {memory[i+c] = int(v[x] / pow(10, c)) % 10;} break;
        /* Fx55 - LD [I], Vx */
        case 0x55:
            for(int c = 0; c<x; c++) {memory[i+c] = v[c];} break;
        /* Fx65 - LD Vx, [I] */
        case 0x65:
            for(int c = 0; c<=x; c++) {v[c] = memory[i+c];} break;
    }
}

int main(int argc, char** argv)
{
    if(argc < 2) {cout << "Usage: chip8 [filename]" << endl; return -1;}

    Chip8 cpu;
    /* Load le programme qui est le premier argument fourni */
    if(!cpu.load(argv[1])) return -1;

    /* Init SDL */
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Init(SDL_INIT_VIDEO);
    window = SDL_CreateWindow("Chip8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1024, 512, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

    bool quit = false;
    SDL_Event e; /* Pour les events de SDL */
    void* pixels; /* Données des pixels de la texture SDL */
    int pitch; /* On en a besoin, sinon SEGFAULT */
    Uint32* mpixels; /* Pointeur vers les pixels de SDL, Uint32 pour pouvoir les modifier */
    Uint32 color; /* Couleur du pixel à set */

    /* Main loop */
    while(!quit)
    {
        while(SDL_PollEvent(&e) != 0)
        {
            if(e.type == SDL_QUIT)
                quit = true;
        }

        /* Cycle de l'émulateur */
        cpu.step();

        /* Update l'écran */
        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        mpixels = (Uint32*)pixels;
        for(int i = 0; i<2048; i++){
            if(cpu.getDisplay()[i]) color = 16777215; /* Blanc */
            else color = 0; /* Noir */
            mpixels[i] = color;
        }

        SDL_UnlockTexture(texture);

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF); /* Noir */
        SDL_RenderClear(renderer);

        SDL_RenderCopy(renderer, texture, NULL, NULL);

        SDL_RenderPresent(renderer);

    }

    return 0;
}
