// OBike simulator entry point.
//
// Two frontends behind one core:
//   --headless [--frames N]   steps the loop and writes PPM frames, no deps
//   (default)                 SDL2 window, keyboard mapped to the 5-way switch
//
// Both call the same setup()/loop() the firmware does, so the thing being
// exercised is the real application, not a reimplementation of it.

#include "Arduino.h"
#include "Adafruit_ST7789.h"
#include "SimHAL.hpp"
#include "HAL/HAL.hpp"
#include "App.hpp"

#include <cstdio>
#include <cstring>
#include <string>

#ifdef SIM_USE_SDL
#include <SDL.h>
#endif

// Defined in src/main.cpp -- the firmware's own entry points. The simulator
// deliberately drives these rather than reimplementing startup, so boot order
// bugs show up here too.
void setup();
void loop();

static const int SCR_W = 240;
static const int SCR_H = 320;

// ---------------------------------------------------------------------------
// Frame output
// ---------------------------------------------------------------------------

static void writePPM(const char* path, const uint16_t* fb) {
    FILE* f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "[sim] cannot write %s\n", path); return; }
    fprintf(f, "P6\n%d %d\n255\n", SCR_W, SCR_H);
    for (int i = 0; i < SCR_W * SCR_H; ++i) {
        const uint16_t c = fb[i];
        // RGB565 -> RGB888, replicating high bits into the low ones so full
        // scale maps to 255 rather than 248.
        const uint8_t r = (uint8_t)(((c >> 11) & 0x1F) * 255 / 31);
        const uint8_t g = (uint8_t)(((c >> 5) & 0x3F) * 255 / 63);
        const uint8_t b = (uint8_t)(( c        & 0x1F) * 255 / 31);
        fputc(r, f); fputc(g, f); fputc(b, f);
    }
    fclose(f);
}

// ---------------------------------------------------------------------------
// Headless
// ---------------------------------------------------------------------------

static int runHeadless(int frames, int stepMs, const char* outDir) {
    setup();
    for (int i = 0; i < frames; ++i) {
        loop();
        simAdvanceMillis(stepMs);
        if (outDir) {
            char path[256];
            snprintf(path, sizeof(path), "%s/frame_%04d.ppm", outDir, i);
            writePPM(path, Adafruit_ST7789::panelBuffer());
        }
    }
    printf("[sim] headless run complete: %d frames, t=%ums\n", frames, millis());
    return 0;
}

// ---------------------------------------------------------------------------
// SDL
// ---------------------------------------------------------------------------

#ifdef SIM_USE_SDL
static int runSdl(int scale) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "[sim] SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* win = SDL_CreateWindow("OBike simulator",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       SCR_W * scale, SCR_H * scale, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565,
                                         SDL_TEXTUREACCESS_STREAMING, SCR_W, SCR_H);
    setup();

    printf("[sim] arrows = D-pad, Enter/Space = select, S = toggle SD card,\n"
           "      G = toggle GPS fix, [ / ] = speed down/up, Esc = quit\n");

    bool running = true;
    uint32_t lastTicks = SDL_GetTicks();

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                const bool down = (e.type == SDL_KEYDOWN);
                switch (e.key.keysym.sym) {
                    case SDLK_UP:     Sim::setButton(Sim::ButtonName::Up, down); break;
                    case SDLK_DOWN:   Sim::setButton(Sim::ButtonName::Down, down); break;
                    case SDLK_LEFT:   Sim::setButton(Sim::ButtonName::Left, down); break;
                    case SDLK_RIGHT:  Sim::setButton(Sim::ButtonName::Right, down); break;
                    case SDLK_RETURN:
                    case SDLK_SPACE:  Sim::setButton(Sim::ButtonName::Select, down); break;
                    case SDLK_ESCAPE: if (down) running = false; break;
                    case SDLK_s: if (down) Sim::state().sdPresent = !Sim::state().sdPresent; break;
                    case SDLK_g: if (down) Sim::state().gpsValid  = !Sim::state().gpsValid;  break;
                    case SDLK_LEFTBRACKET:
                        if (down) { Sim::state().gpsSpeedKmh = max(0.0f, Sim::state().gpsSpeedKmh - 2.0f);
                                    Sim::state().wheelRPM = Sim::state().gpsSpeedKmh * 7.9f; }
                        break;
                    case SDLK_RIGHTBRACKET:
                        if (down) { Sim::state().gpsSpeedKmh += 2.0f;
                                    Sim::state().wheelRPM = Sim::state().gpsSpeedKmh * 7.9f;
                                    Sim::state().wheelRPMLive = true; }
                        break;
                    default: break;
                }
            }
        }

        // Advance simulated time by real elapsed time, so hold-to-repeat and
        // the 100 ms render throttle behave as they do on the device.
        const uint32_t now = SDL_GetTicks();
        const uint32_t delta = now - lastTicks;
        lastTicks = now;
        simAdvanceMillis(delta ? delta : 1);

        loop();

        SDL_UpdateTexture(tex, nullptr, Adafruit_ST7789::panelBuffer(), SCR_W * sizeof(uint16_t));
        SDL_RenderClear(ren);
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
        SDL_Delay(1);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
#endif

int main(int argc, char** argv) {
    bool headless = false;
    int frames = 200, stepMs = 20, scale = 2;
    const char* outDir = nullptr;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--headless") headless = true;
        else if (a == "--frames" && i + 1 < argc) frames = atoi(argv[++i]);
        else if (a == "--step"   && i + 1 < argc) stepMs = atoi(argv[++i]);
        else if (a == "--out"    && i + 1 < argc) outDir = argv[++i];
        else if (a == "--scale"  && i + 1 < argc) scale = atoi(argv[++i]);
        else if (a == "--sd"     && i + 1 < argc) simSetSdRoot(argv[++i]);
        else if (a == "--help") {
            printf("OBike simulator\n"
                   "  --headless          run without a window\n"
                   "  --frames N          headless frame count (default 200)\n"
                   "  --step MS           simulated ms per frame (default 20)\n"
                   "  --out DIR           write PPM frames to DIR\n"
                   "  --scale N           SDL window scale (default 2)\n"
                   "  --sd DIR            SD card root (default ./sdcard)\n");
            return 0;
        }
    }

#ifdef SIM_USE_SDL
    if (!headless) return runSdl(scale);
#else
    if (!headless) {
        printf("[sim] built without SDL; running headless. Use --out DIR for frames.\n");
    }
#endif
    return runHeadless(frames, stepMs, outDir);
}
