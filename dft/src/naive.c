#include <raylib.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "core/dck.h"

// cc src/naive.c ../raylib/lib/libraylib.a -o naive.exe -I../raylib/include -lm -ldl -lpthread && ./naive.exe

#define BG_COLOR ((Color) { \
    .r = 66, \
    .g = 66, \
    .b = 66, \
    .a = 255, \
})

#define BAR_BG_COLOR ((Color) { \
    .r = 6, \
    .g = 6, \
    .b = 6, \
    .a = 255, \
})

#define CURSOR_COLOR ((Color) { \
    .r = 234, \
    .g = 66, \
    .b = 66, \
    .a = 255, \
})

#define WAVE_COLOR ((Color) { \
    .r = 66, \
    .g = 66, \
    .b = 123, \
    .a = 255, \
})

int
main(void)
{
    SetTraceLogLevel(LOG_WARNING); // KYS

    const char *audio_file = "../oneyplays/witch_hunt_test/audio.ogg";

    InitAudioDevice();

    Music music = LoadMusicStream(audio_file);
    if (IsMusicReady(music)) {
        printf("'%s' loaded succesfully as music.\n", audio_file);
    }
    else {
        fprintf(stderr, "'%s' is not ready as music!\n", audio_file);
        exit(1);
    }

    Wave wave = LoadWave(audio_file);
    if (IsWaveReady(wave)) {
        printf("'%s' loaded succesfully as wave.\n", audio_file);
    }
    else {
        fprintf(stderr, "'%s' is not ready as wave!\n", audio_file);
        exit(1);
    }

    assert(wave.sampleSize == 16);
    assert(wave.channels   == 2);

    unsigned short *wave_data = wave.data;

    PlayMusicStream(music);

    InitWindow(1920, 1080, "Naive DFT");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    float music_length = GetMusicTimeLength(music);

    dck_stretchy_t (float, unsigned) cached_wave_ratios = {0};

    while (!WindowShouldClose()) {
        UpdateMusicStream(music);

        int bar_height   = 200;
        int cursor_width = 4;

        int window_width  = GetScreenWidth();
        int window_height = GetScreenHeight();

        if (IsKeyPressed(KEY_P)) {
            if (IsMusicStreamPlaying(music)) {
                StopMusicStream(music);
                printf("stop\n");
            }
            else {
                PlayMusicStream(music);
                printf("play\n");
            }
        }

        if (IsKeyPressed(KEY_SPACE)) {
            if (IsMusicStreamPlaying(music)) {
                PauseMusicStream(music);
                printf("pause\n");
            }
            else {
                ResumeMusicStream(music);
                printf("resume\n");
            }
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            int mouse_x = GetMouseX();
            int mouse_y = GetMouseY();

            if (mouse_y > window_height - bar_height) {
                float ratio = mouse_x / (float)window_width;
                float seek_pos = ratio * music_length;
                SeekMusicStream(music, seek_pos);
            }
        }

        BeginDrawing();

        ClearBackground(BG_COLOR);

        DrawRectangle(0, window_height - bar_height, window_width, bar_height, BAR_BG_COLOR);

        if (cached_wave_ratios.count != window_width) {
            cached_wave_ratios.count = 0;
            dck_stretchy_reserve(cached_wave_ratios, window_width);

            unsigned pixel_frames = wave.frameCount / window_width;

            for (unsigned x_pos = 0; x_pos < window_width; ++x_pos) {
                unsigned max_amp = 0;

                unsigned frame_end = pixel_frames * (x_pos + 1);
                if (frame_end > wave.frameCount) {
                    frame_end = wave.frameCount;
                }

                for (unsigned frame = pixel_frames * x_pos; frame <  frame_end; ++frame) {
                    unsigned amp_l = wave_data[frame * 2 + 0];
                    unsigned amp_r = wave_data[frame * 2 + 1];

                    if (amp_l > amp_r) {
                        amp_r = amp_l;
                    }

                    max_amp += amp_r;
                }

                max_amp /= pixel_frames;

                float amp_ratio = max_amp / (float)UINT16_MAX;
                cached_wave_ratios.data[x_pos] = amp_ratio;
            }

            cached_wave_ratios.count = window_width;
        }

        for (unsigned x_pos = 0; x_pos < cached_wave_ratios.count; ++x_pos) {
            float amp_ratio = cached_wave_ratios.data[x_pos];
            int wave_height = (int)(bar_height * amp_ratio);
            int wave_space  = bar_height - wave_height;

            DrawLine(x_pos, window_height - wave_space / 2, x_pos, window_height - bar_height + wave_space / 2, WAVE_COLOR);
        }

        float music_played = GetMusicTimePlayed(music);

        int cursor_x = (int)(window_width * (music_played / music_length)) - (cursor_width / 2);

        DrawRectangle(cursor_x, window_height - bar_height, cursor_width, bar_height, CURSOR_COLOR);

        EndDrawing();  
    }

    CloseWindow();
    CloseAudioDevice();

    printf("\\_/\n V\n");
    return 0;
}
