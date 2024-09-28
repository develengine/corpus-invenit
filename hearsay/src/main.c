#include <raylib.h>

#include "core/utils.h"
#include "core/dck.h"

#include <stdio.h>
#include <math.h>

#define HEX_COL(m_hex) ((Color) { \
    .r = ((m_hex >> 8 * 0) & 0xFF), \
    .g = ((m_hex >> 8 * 1) & 0xFF), \
    .b = ((m_hex >> 8 * 2) & 0xFF), \
    .a = ((m_hex >> 8 * 3) & 0xFF), \
})

#define AIR_COLOR HEX_COL(0xFFFFE6A6)

// #define O(m_name, m_path)
// TILE_TABLE
// #undef O
#define TILE_TABLE \
    O(Grass,  "res/grass.png") \
    O(Stone,  "res/stone.png") \
    O(Tree,   "res/tree.png") \
    O(Person, "res/man.png") \
/**/

typedef struct
{
    union {
        u32 offset;
        u32 base;
        u32 index;
    };
    union {
        u32 size;
        u32 count;
    };
} vu32_t;

const char *first_names[] = {
    "Jakub",
    "Samuel",
    "Adam",
    "Šimon",
    "Michal",
    "Oliver",
    "Tomáš",
    "Filip",
    "Matej",
    "Martin",
};

const char *last_names[] = {
    "Horváth",
    "Kováč",
    "Varga",
    "Tóth",
    "Nagy",
    "Baláž",
    "Szabó",
    "Molnár",
    "Balog",
    "Lukáč",
};

typedef enum
{
#define O(m_name, m_path) \
    tile_##m_name,
    TILE_TABLE
#undef O
    TILE_COUNT
} tile_t;

typedef dck_stretchy_t (char, u32) person_names_t;

typedef struct
{
    i32 x_pos;
    i32 y_pos;
    u32 name_offset;
} person_t;

#define TILE_RES 32

#define MAP_WIDTH  64
#define MAP_HEIGHT 64

#define ZOOM_SPEED 4.0f

#define PERSON_COUNT 10

#define NAME_FORMAT "%s %s"

struct {
    Font font;
} glob = {0};

static u32
generate_name(person_names_t *person_names)
{
    u32 name_offset = person_names->count;

    const char *first_name = first_names[rand() % LENGTH_OF(first_names)];
    const char *last_name  = last_names [rand() % LENGTH_OF(last_names)];

    u32 size = (u32)snprintf(NULL, 0, NAME_FORMAT, first_name, last_name) + 1;
    dck_stretchy_reserve(*person_names, size);
    snprintf(person_names->data + person_names->count, size, NAME_FORMAT, first_name, last_name);
    person_names->count += size;

    return name_offset;
}

static u32
UTF8ToUTF32(const char *s, u32 *size)
{
    if ((s[0] & 0x80) == 0) {
        *size = 1;
        return s[0];
    }

    if ((s[0] & 0xE0) == 0xC0) {
        *size = 2;
        return ((s[0] & 0x1F) << 6) |
                (s[1] & 0x3F);
    }

    if ((s[0] & 0xF0) == 0xE0) {
        *size = 3;
        return ((s[0] & 0x0F) << 12) |
               ((s[1] & 0x3F) << 6)  |
                (s[2] & 0x3F);
    }

    if ((s[0] & 0xF8) == 0xF0) {
        *size = 4;
        return ((s[0] & 0x07) << 18) |
               ((s[1] & 0x3F) << 12) |
               ((s[2] & 0x3F) << 6)  |
                (s[3] & 0x3F);
    }

    *size = 0;

    return 0;
}

static void
draw_text(const char *text, Vector2 pos, f32 tile_size, i32 font_size)
{
    Vector2 span = MeasureTextEx(glob.font, text, font_size, 0.0f);

    Vector2 position = {
        pos.x - (span.x - tile_size) / 2,
        pos.y - (span.y - tile_size) / 2,
    };

    DrawTextEx(glob.font, text, position, font_size, 0.0f, WHITE);
}

static dck_stretchy_t (u32, u32) codepoint_buffer = {0};

static Font
load_font_in_such_a_way_that_i_dont_kill_raysan_with_a_hammer(const char *font_path)
{
    // Why not load all the glyphs by default huh? Are you fucking retarded Raysan?

    const char *all_text = 
        "0123456789"
        " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
        "aáäbcčdďeéfghiíjklĺľmnňoóôpqrŕřsštťuúvwxyýzž"
        "AÁÄBCČDĎEÉFGHIÍJKLĹĽMNŇOÓÔPQRŔŘSŠTŤUÚVWXYÝZŽ"
    ;

    u32 offset = 0;

    while (all_text[offset] != 0) {
        u32 size;
        u32 codepoint = UTF8ToUTF32(all_text + offset, &size);
        offset += size;

        u32 i = 0;
        for (; i < codepoint_buffer.count; ++i) {
            if (codepoint_buffer.data[i] == codepoint)
                break;
        }

        if (i == codepoint_buffer.count) {
            dck_stretchy_push(codepoint_buffer, codepoint);
        }
    }

    return LoadFontEx(font_path, 40, (i32*)codepoint_buffer.data, codepoint_buffer.count);
}

i32
main(void)
{
    SetTraceLogLevel(LOG_WARNING);

    InitWindow(1920, 1080, "HearSay");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    const char *font_path = "res/JetBrainsMono-Regular.ttf";

    glob.font = load_font_in_such_a_way_that_i_dont_kill_raysan_with_a_hammer(font_path);


    Texture2D textures[TILE_COUNT] = {
    #define O(m_name, m_path) \
        [tile_##m_name] = LoadTexture(m_path),
        TILE_TABLE
    #undef O
    };

    tile_t map_tiles  [MAP_WIDTH * MAP_HEIGHT] = {0};
    f32    map_heights[MAP_WIDTH * MAP_HEIGHT];

    for (i32 y_pos = 0; y_pos < MAP_HEIGHT; ++y_pos) {
        for (i32 x_pos = 0; x_pos < MAP_WIDTH; ++x_pos) {
            i32 x_dist = x_pos - (MAP_WIDTH  / 2);
            i32 y_dist = y_pos - (MAP_HEIGHT / 2);
            f32 dist   = sqrtf(x_dist * x_dist + y_dist * y_dist);
            f32 random = (rand() % 2048) / 2047.0f;
            f32 dist_scale = dist / MAP_WIDTH / 2;
            map_heights[x_pos + y_pos * MAP_WIDTH] = random * (dist_scale);
        }
    }

    f32 tile_size = 128.0f;
    Vector2 screen_center = { MAP_WIDTH / 2, MAP_HEIGHT / 2 };

    SetTargetFPS(60);

    i32 screen_width;
    i32 screen_height;

    person_names_t person_names = {0};
    dck_stretchy_t (person_t, u32) persons = {0};

    for (u32 i = 0; i < PERSON_COUNT; ++i) {
        dck_stretchy_push(persons, (person_t) {
            .x_pos = (rand() % (MAP_WIDTH  / 2)) + (MAP_WIDTH  / 4),
            .y_pos = (rand() % (MAP_HEIGHT / 2)) + (MAP_HEIGHT / 4),
            .name_offset = generate_name(&person_names),
        });
    }

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_Q))
            break;

        screen_width  = GetScreenWidth(); 
        screen_height = GetScreenHeight();

        tile_size += GetMouseWheelMove() * ZOOM_SPEED;

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            screen_center.x -= delta.x / tile_size;
            screen_center.y -= delta.y / tile_size;
        }

        if (IsKeyPressed(KEY_F)) {
            ToggleBorderlessWindowed();
        }

        BeginDrawing();

        ClearBackground(AIR_COLOR);

        Vector2 screen_origin = {
            .x = (screen_width  / 2) - screen_center.x * tile_size,
            .y = (screen_height / 2) - screen_center.y * tile_size,
        };

        for (u32 y_pos = 0; y_pos < MAP_HEIGHT; ++y_pos) {
            for (u32 x_pos = 0; x_pos < MAP_WIDTH; ++x_pos) {
                u32 map_index = x_pos + y_pos * MAP_WIDTH;
                tile_t tile = map_tiles[map_index];

                Vector2 pos = {
                    x_pos * tile_size + screen_origin.x,
                    y_pos * tile_size + screen_origin.y,
                };

                Color col = WHITE;
                col.a *= (1.0f - map_heights[map_index]);

                DrawTextureEx(textures[tile], pos, 0.0f, tile_size / (f32)TILE_RES, col);
            }
        }

        for (u32 person_i = 0; person_i < persons.count; ++person_i) {
            person_t person = persons.data[person_i];

            Vector2 pos = {
                person.x_pos * tile_size + screen_origin.x,
                person.y_pos * tile_size + screen_origin.y,
            };

            DrawTextureEx(textures[tile_Person], pos, 0.0f, tile_size / (f32)TILE_RES, WHITE);

            Vector2 text_pos = {
                pos.x,
                pos.y - tile_size * 0.75f,
            };

            draw_text(person_names.data + person.name_offset, text_pos, tile_size, tile_size * 0.5);
        }

        EndDrawing();  
    }

    CloseWindow();

    printf("\\_/\n V\n");
    return 0;
}
