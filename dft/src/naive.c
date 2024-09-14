#include <raylib.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#include "core/utils.h"
#include "core/dck.h"
#include "core/sv.h"

#define VTT_PARSER_IMPL
#include "vtt_parser.h"

// cc src/naive.c ../raylib/lib/libraylib.a -o naive.exe -I. -I../raylib/include -lm -ldl -lpthread && ./naive.exe

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

#define TEXT_FG_COLOR ((Color) { \
    .r = 255, \
    .g = 255, \
    .b = 255, \
    .a = 255, \
})

i32
main(void)
{
    SetTraceLogLevel(LOG_WARNING); // KYS

    const char *audio_file   = "../oneyplays/witch_hunt_test/audio.ogg";
    const char *caption_file = "../oneyplays/witch_hunt_test/text.en.vtt";
    // const char *caption_file = "../oneyplays/witch_hunt_test/short.en.vtt";

    InitAudioDevice();

    Music music = LoadMusicStream(audio_file);
    if (IsMusicReady(music)) {
        printf("'%s' loaded successfully as music.\n", audio_file);
    }
    else {
        fprintf(stderr, "'%s' is not ready as music!\n", audio_file);
        exit(1);
    }

    Wave wave = LoadWave(audio_file);
    if (IsWaveReady(wave)) {
        printf("'%s' loaded successfully as wave.\n", audio_file);
    }
    else {
        fprintf(stderr, "'%s' is not ready as wave!\n", audio_file);
        exit(1);
    }

    assert(wave.sampleSize == 16);
    assert(wave.channels   == 2);

    u16 *wave_data = wave.data;

    vtt_data_t vtt_data = {0};
    vtt_chunk_t vtt_chunk = vtt_parse_file(&vtt_data, caption_file);

    for (u32 i = 0; i < vtt_chunk.word_count; ++i) {
        vtt_word_t word = vtt_data.words.data[vtt_chunk.word_offset + i];
        printf("%f --> %f: "SV_FMT"\n",
               word.time_start, word.time_end,
               word.text_size, vtt_data.text.data + word.text_offset);
    }

    PlayMusicStream(music);

    InitWindow(1920, 1080, "Naive DFT");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    f32 music_length = GetMusicTimeLength(music);

    dck_stretchy_t (f32, u32) cached_wave_ratios = {0};

    u8 text_buffer[256];

    while (!WindowShouldClose()) {
        UpdateMusicStream(music);

        i32 bar_height   = 200;
        i32 cursor_width = 4;

        i32 window_width  = GetScreenWidth();
        i32 window_height = GetScreenHeight();

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
            i32 mouse_x = GetMouseX();
            i32 mouse_y = GetMouseY();

            if (mouse_y > window_height - bar_height) {
                f32 ratio = mouse_x / (f32)window_width;
                f32 seek_pos = ratio * music_length;
                SeekMusicStream(music, seek_pos);
            }
        }

        BeginDrawing();

        ClearBackground(BG_COLOR);

        DrawRectangle(0, window_height - bar_height, window_width, bar_height, BAR_BG_COLOR);

        if (cached_wave_ratios.count != window_width) {
            cached_wave_ratios.count = 0;
            dck_stretchy_reserve(cached_wave_ratios, window_width);

            u32 pixel_frames = wave.frameCount / window_width;

            for (u32 x_pos = 0; x_pos < window_width; ++x_pos) {
                u32 max_amp = 0;

                u32 frame_end = pixel_frames * (x_pos + 1);
                if (frame_end > wave.frameCount) {
                    frame_end = wave.frameCount;
                }

                for (u32 frame = pixel_frames * x_pos; frame <  frame_end; ++frame) {
                    u32 amp_l = wave_data[frame * 2 + 0];
                    u32 amp_r = wave_data[frame * 2 + 1];

                    if (amp_l > amp_r) {
                        amp_r = amp_l;
                    }

                    max_amp += amp_r;
                }

                max_amp /= pixel_frames;

                f32 amp_ratio = max_amp / (f32)UINT16_MAX;
                cached_wave_ratios.data[x_pos] = amp_ratio;
            }

            cached_wave_ratios.count = window_width;
        }

        for (u32 x_pos = 0; x_pos < cached_wave_ratios.count; ++x_pos) {
            f32 amp_ratio = cached_wave_ratios.data[x_pos];
            i32 wave_height = (i32)(bar_height * amp_ratio);
            i32 wave_space  = bar_height - wave_height;

            DrawLine(x_pos, window_height - wave_space / 2, x_pos, window_height - bar_height + wave_space / 2, WAVE_COLOR);
        }

        f32 music_played = GetMusicTimePlayed(music);

        i32 cursor_x = (i32)(window_width * (music_played / music_length)) - (cursor_width / 2);

        DrawRectangle(cursor_x, window_height - bar_height, cursor_width, bar_height, CURSOR_COLOR);

        if (vtt_chunk.word_count != 0) {
            vtt_word_t vtt_word = vtt_data.words.data[0];

            for (u32 i = 1; i < vtt_chunk.word_count; ++i) {
                vtt_word_t vtt_word_2 = vtt_data.words.data[i];

                if (vtt_word_2.time_start > music_played)
                    break;

                vtt_word = vtt_word_2;
            }

            snprintf(text_buffer, sizeof(text_buffer), SV_FMT, vtt_word.text_size, vtt_data.text.data + vtt_word.text_offset);
            DrawText(text_buffer, 0, 0, 64, TEXT_FG_COLOR);
        }

        EndDrawing();  
    }

    CloseWindow();
    CloseAudioDevice();

    printf("\\_/\n V\n");
    return 0;
}
