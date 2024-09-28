#include <raylib.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <math.h>

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

#define DCC_COLOR ((Color) { \
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

typedef struct
{
    u32 offset, size;
} wave_mip_level_t;

#define FFT_POW 12
#define FFT_SIZE (1 << FFT_POW)

typedef struct
{
    f32 r, i;
} cn_t;

static cn_t fft_buffer[FFT_SIZE * 2];

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

    PlayMusicStream(music);

    InitWindow(1920, 1080, "Naive DFT");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    SetTargetFPS(60);

    f32 music_length = GetMusicTimeLength(music);

    dck_stretchy_t (f32,              u32) wave_mip_values = {0};
    dck_stretchy_t (wave_mip_level_t, u32) wave_mip_levels = {0};
    {
        u32 wave_size = wave.frameCount;

        dck_stretchy_reserve(wave_mip_values, wave_size);

        for (u32 i = 0; i < wave_size; ++i) {
            u16 amp_l = wave_data[i * 2 + 0];
            u16 amp_r = wave_data[i * 2 + 1];

            f32 amp_ratio = (amp_l + amp_r) / (f32)(UINT16_MAX * 2);
            wave_mip_values.data[wave_mip_values.count + i] = amp_ratio;
        }

        dck_stretchy_push(wave_mip_levels, (wave_mip_level_t) {
            .offset = wave_mip_values.count,
            .size   = wave_size,
        });

        wave_mip_values.count += wave_size;

        while (wave_size > 1) {
            wave_size /= 2;

            dck_stretchy_reserve(wave_mip_values, wave_size);

            wave_mip_level_t prev_level = wave_mip_levels.data[wave_mip_levels.count - 1];

            for (u32 i = 0; i < wave_size; ++i) {
                f32 a = wave_mip_values.data[prev_level.offset + i * 2 + 0];
                f32 b = wave_mip_values.data[prev_level.offset + i * 2 + 1];
                wave_mip_values.data[wave_mip_values.count + i] = (a + b) * 0.5f;
            }

            dck_stretchy_push(wave_mip_levels, (wave_mip_level_t) {
                .offset = wave_mip_values.count,
                .size   = wave_size,
            });

            wave_mip_values.count += wave_size;
        }
    }

    UnloadWave(wave);

    u8 text_buffer[256];

    dck_stretchy_t (f32, u32) cos_cross = {0};

    while (!WindowShouldClose()) {
        UpdateMusicStream(music);

        i32 bar_height      = 200;
        i32 cursor_width    = 4;
        i32 captions_height = 64;

        i32 window_width  = GetScreenWidth();
        i32 window_height = GetScreenHeight();

        if (IsKeyPressed(KEY_Q))
            break;

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

        u32 mip_level_i = wave_mip_levels.count - 1;
        wave_mip_level_t mip_level = wave_mip_levels.data[mip_level_i];

        while (mip_level.size < window_width && mip_level_i != 0) {
            --mip_level_i;
            mip_level = wave_mip_levels.data[mip_level_i];
        }

        for (u32 x_pos = 0; x_pos < window_width; ++x_pos) {
            u32 amp_index = mip_level.offset + (x_pos * mip_level.size) / window_width;
            f32 amp_ratio = wave_mip_values.data[amp_index];
            i32 wave_height = (i32)(bar_height * amp_ratio);
            i32 wave_space  = bar_height - wave_height;

            DrawLine(x_pos, window_height - wave_space / 2, x_pos, window_height - bar_height + wave_space / 2, WAVE_COLOR);
        }

        f32 music_played = GetMusicTimePlayed(music);

        i32 cursor_x = (i32)(window_width * (music_played / music_length)) - (cursor_width / 2);

        DrawRectangle(cursor_x, window_height - bar_height, cursor_width, bar_height, CURSOR_COLOR);

        u32 wave_pos = wave.frameCount * (music_played / music_length);
        f32 *wave_values = wave_mip_values.data + wave_pos;

#if 0
        cos_cross.count = 0;
        dck_stretchy_reserve(cos_cross, window_width);
        f32 max_cross = 0.0f;

        for (u32 x_pos = 0; x_pos < window_width; ++x_pos) {
            f32 freq = x_pos / (f32)window_width;

            f32 cross_r = 0.0f;
            f32 cross_i = 0.0f;

            for (u32 x_pos_2 = 0; x_pos_2 < window_width; ++x_pos_2) {
                f32 val = wave_values[x_pos_2] * 2.0 - 1.0f;
                cross_r += cosf(freq * M_PI * 2.0f * x_pos_2) * val;
                cross_i += sinf(freq * M_PI * 2.0f * x_pos_2) * val;
            }

            f32 cross = sqrtf(cross_r * cross_r + cross_i * cross_i);

            if (cross > max_cross) {
                max_cross = cross;
            }

            cos_cross.data[x_pos] = cross;
        }

        cos_cross.count = window_width;

        for (u32 x_pos = 0; x_pos < window_width; ++x_pos) {
            f32 val = cos_cross.data[x_pos];

            DrawLine(x_pos, window_height - bar_height, x_pos, window_height - bar_height - val, DCC_COLOR);
        }
#else
        for (u32 i = 0; i < FFT_SIZE; ++i) {
            fft_buffer[i] = (cn_t) {
                .r = wave_values[i],
                .i = 0.0f,
            };
        }

        cn_t *fft_src = fft_buffer + 0;
        cn_t *fft_dst = fft_buffer + FFT_SIZE;

        u32 fft_span   = FFT_SIZE / 2;
        u32 fft_blocks = 1;

        while (fft_span > 0) {
            for (u32 block = 0; block < fft_blocks; ++block) {
                u32 block_offset = block * fft_span * 2;

                for (u32 off = 0; off < fft_span; ++off) {
                    u32 index = block_offset + off;

                    f32 angle = 2.0f * M_PI * (index / (f32)FFT_SIZE);
                    cn_t mf = {
                        .r =  cosf(angle),
                        .i = -sinf(angle),
                    };

                    cn_t a = fft_src[index];
                    cn_t b = fft_src[index + fft_span];
                    cn_t b_2 = {
                        .r = mf.r * b.r - mf.i * b.i,
                        .i = mf.r * b.i + mf.i * b.r,
                    };

                    fft_dst[index] = (cn_t) {
                        .r = a.r + b_2.r,
                        .i = a.i + b_2.i,
                    };

                    fft_dst[index + fft_span] = (cn_t) {
                        .r = a.r - b_2.r,
                        .i = a.i - b_2.i,
                    };
                }
            }

            fft_span   /= 2;
            fft_blocks *= 2;

            cn_t *tmp = fft_dst;
            fft_dst = fft_src;
            fft_src = tmp;
        }

        for (u32 x_pos = 0; x_pos < window_width; ++x_pos) {
            cn_t cn = fft_src[(x_pos * FFT_SIZE) / window_width];
            f32 val = sqrtf(cn.r * cn.r + cn.i * cn.i);

            DrawLine(x_pos, window_height - bar_height, x_pos, window_height - bar_height - val, DCC_COLOR);
        }
#endif

        if (vtt_chunk.word_count != 0) {
            vtt_word_t vtt_word = vtt_data.words.data[0];

            for (u32 i = 1; i < vtt_chunk.word_count; ++i) {
                vtt_word_t vtt_word_2 = vtt_data.words.data[i];

                if (vtt_word_2.time_start > music_played)
                    break;

                vtt_word = vtt_word_2;
            }

            snprintf(text_buffer, sizeof(text_buffer), SV_FMT, vtt_word.text_size, vtt_data.text.data + vtt_word.text_offset);

            i32 captions_width = MeasureText(text_buffer, captions_height);

            i32 cap_x = (window_width - captions_width) / 2;
            i32 cap_y = window_height - bar_height - captions_height;

            DrawText(text_buffer, cap_x, cap_y, captions_height, TEXT_FG_COLOR);
        }

        EndDrawing();  
    }

    CloseWindow();
    CloseAudioDevice();

    printf("\\_/\n V\n");
    return 0;
}
