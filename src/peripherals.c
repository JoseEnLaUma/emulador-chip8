#include "peripherals.h"
#include <SDL2/SDL.h>

SDL_Window *screen;
SDL_Renderer *renderer;

//Mapear las teclas del teclado
SDL_Scancode keymappings[16] = {
    SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4,
    SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
    SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
    SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V
};

int should_quit = 0;

void init_display(void){
    SDL_Init(SDL_INIT_VIDEO);
    screen = SDL_CreateWindow("CHIP-8", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 64 *8, 32*8, 0);
    renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);
}

void draw(unsigned char *display){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for(int y = 0; y < 32; y++){
        for(int x = 0; x < 64; x++){
            if(display[x + (y*64)]){
                SDL_Rect rect;
                rect.x = x * 8;
                rect.y = y * 8;
                rect.w = 8;
                rect.h = 8;

                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    SDL_RenderPresent(renderer);
}

void sdl_ehandler(unsigned char *keypad){
    SDL_Event event;
    if(SDL_PollEvent(&event)){
        const Uint8 *state = SDL_GetKeyboardState(NULL);

        switch(event.type){
            case SDL_QUIT:
                should_quit = 1;
                break;
            default:
                if(state[SDL_SCANCODE_ESCAPE]){
                    should_quit = 1;
                }

                for(int keycode = 0; keycode < 16; keycode++){
                    keypad[keycode] = state[keymappings[keycode]];
                }
                break;
        } 
    }
}

void stop_display(void){
    SDL_DestroyWindow(screen);
    SDL_Quit();
}