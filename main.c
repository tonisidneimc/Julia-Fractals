#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

#define WIDTH 800
#define HEIGHT 600
#define MAX_ITER 256
#define BAILOUT 4
#define ZOOM_FACTOR 1.1
#define THREADS 32

typedef struct {
    double x;
    double y;
} complex_t;

extern inline double sqr(double x) {
    return x * x;
}

extern inline double mag(complex_t z) {
    return sqrt(sqr(z.x) + sqr(z.y));
}

extern inline complex_t add(complex_t z1, complex_t z2) {
    return (complex_t) { z1.x + z2.x, z1.y + z2.y };
}

extern inline complex_t mul(complex_t z1, complex_t z2) {
    return (complex_t) { z1.x * z2.x - z1.y * z2.y, z1.x * z2.y + z1.y * z2.x };
}

typedef struct {
    int r, g, b;
} color_t;

color_t colors[MAX_ITER];

void init_colors() {
    color_t* c = colors;
    for (int i = 0; i < MAX_ITER; i++, c++) {
        (*c).r = (int)(sin(0.3*i + 0) * 127 + 128);
        (*c).g = (int)(sin(0.3*i + 2) * 127 + 128);
        (*c).b = (int)(sin(0.3*i + 4) * 127 + 128);
    }
}

typedef struct {
    int start;
    int end;
    complex_t c;
    SDL_Renderer *renderer;
} thread_data;

SDL_mutex *renderer_mutex;

int draw_julia_part(void *arg) {
    thread_data* data = (thread_data*)arg;
    int i, j, k;
    double zx, zy;
    complex_t z;

    for (i = data->start; i < data->end; i++) {
        for (j = 0; j < HEIGHT; j++) {
            zx = 1.5 * (i - WIDTH / 2) / (0.5 * WIDTH);
            zy = (j - HEIGHT / 2) / (0.5 * HEIGHT);
            z = (complex_t) { zx, zy };
            k = 0;
            do {
                z = add(mul(z, z), data->c);
                k++;
            } while (mag(z) < BAILOUT && k < MAX_ITER);
            
            SDL_LockMutex(renderer_mutex);
            if (k == MAX_ITER) {
                SDL_SetRenderDrawColor(data->renderer, 0, 0, 0, 255);
            } else {
                color_t color = colors[k];
                SDL_SetRenderDrawColor(data->renderer, color.r, color.g, color.b, 255);
            }
            SDL_RenderDrawPoint(data->renderer, i, j);
            SDL_UnlockMutex(renderer_mutex);
        }
    }
    return 0;
}

void draw_julia(SDL_Renderer *renderer, complex_t c) {
    int i;
    thread_data data[THREADS];
    SDL_Thread *threads[THREADS];
    int step = WIDTH / THREADS;

    for (i = 0; i < THREADS; i++) {
        data[i].start = i * step;
        data[i].end = (i == THREADS - 1) ? WIDTH : (i + 1) * step;
        data[i].c = c;
        data[i].renderer = renderer;
        threads[i] = SDL_CreateThread(draw_julia_part, NULL, (void *)&data[i]);
        if (threads[i] == NULL) {
            fprintf(stderr, "Unable to create thread: %s\n", SDL_GetError());
            exit(1);
        }
    }

    for (i = 0; i < THREADS; i++) {
        SDL_WaitThread(threads[i], NULL);
    }
}

int main(int argc, char **argv) {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    int quit = 0;
    double zoom = 1.0;
    complex_t c = { -0.4, 0.6 }; // starting value of c

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Unable to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    init_colors();
    renderer_mutex = SDL_CreateMutex();

    window = SDL_CreateWindow("Julia Fractal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = 1;
                    break;
                case SDL_MOUSEMOTION:
                    // update value of c based on mouse position
                    c.x = (double)event.motion.x / WIDTH * 2 - 1;
                    c.y = (double)event.motion.y / HEIGHT * 2 - 1;
                    break;
                case SDL_MOUSEWHEEL:
                    if (event.wheel.y > 0) {
                        zoom *= ZOOM_FACTOR;
                    } else if (event.wheel.y < 0) {
                        zoom /= ZOOM_FACTOR;
                    }
                    break;
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Scale the fractal according to the zoom factor
        complex_t scaled_c = {
            c.x / zoom,
            c.y / zoom
        };
        draw_julia(renderer, scaled_c);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}

