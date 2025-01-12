#include <raylib.h>

#define OLIVEC_IMPLEMENTATION
#include <olive.c>

#include "core/utils.h"
#include "core/dck.h"

#include <stdio.h>
#include <math.h>
#include <time.h>

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
    O(Man,    "res/man.png") \
    O(Forest, "res/forest.png") \
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
    TILE_COUNT,
    TILE_GENERATED,
} tile_t;

#define PROP_NAME     (1 << 0)
#define PROP_MOVING   (1 << 1)
#define PROP_THINKING (1 << 2)
#define PROP_FORM     (1 << 3)

#define TILE_RES 32

#define ZOOM_SPEED 4.0f

#define UPDATE_FREQ 4
#define SIGHT_RADIUS 10

#define NAME_FORMAT "%s %s"

#define GEN_BUF_CAPACITY 64
#define ATLAS_SIZE 8

struct {
    Font font;
} glob = {0};

#define MAP_WIDTH  64
#define MAP_HEIGHT 64

typedef enum
{
    fdl_Tree,
    fdl_Rock,
    fdl_Man,
} fdl_class_t;

typedef enum
{
    fdl_fruit_None = 0,
    fdl_fruit_Apple,
    fdl_fruit_Orange,
    fdl_fruit_Lemon,

    FDL_FRUIT_COUNT
} fdl_fruit_t;

static u32
fdl_fruit_color(fdl_fruit_t fruit)
{
    switch (fruit) {
        case fdl_fruit_Apple:  return 0xFF0000FF;
        case fdl_fruit_Orange: return 0xFF00AAFF;
        case fdl_fruit_Lemon:  return 0xFF00FFFF;

        default: return 0;
    }
}

typedef struct
{
    f32 height;
    f32 thickness;
    f32 crown_span;
    f32 leafness;

    fdl_fruit_t fruit;

    u32 seed; // Defines whether something looks exactly as something else.
} fdl_tree_t;

typedef struct
{
    f32 roundness;
    f32 height;
    f32 width;

    u32 seed; // Defines whether something looks exactly as something else.
} fdl_rock_t;

typedef struct
{
} fdl_man_t;

typedef struct
{
    fdl_class_t class;
    union {
        fdl_man_t  man;
        fdl_tree_t tree;
        fdl_rock_t rock;
    };
} fdl_t;

typedef struct
{
    i32 x, y;
} ivec2_t;


#define MAX_REL_POS 3

typedef struct
{
    fdl_t fdl;
} homun_t;


typedef enum
{
    action_Move,
} action_tag_t;

typedef struct
{
    action_tag_t tag;
    union {
        struct {
            i32 x;
            i32 y;
        } move;
    };
} action_t;

typedef enum
{
    stimul_Sight,
} stimul_tag_t;

typedef struct
{
    stimul_tag_t tag;
    union {
        struct {
            i32 x;
            i32 y;
        } sight;
    };
} stimul_t;

typedef struct
{
    u32 lol;
} mind_data_t;

typedef action_t (*process_t)(mind_data_t *data, stimul_t input);

typedef enum
{
    mind_movement_Right,
    mind_movement_Up,
    mind_movement_Left,
    mind_movement_Down,
} mind_movement_t;

typedef struct
{
    mind_movement_t movement;
} mind_output_t;

typedef struct
{
    mind_data_t data;
    dck_stretchy_t (process_t, i32) processes;
    mind_output_t output;
} mind_t;

typedef struct
{
    i32 x_pos;
    i32 y_pos;

    tile_t texture_tile;
    u32    atlas_id;

    u64 properties;

    u32 name_offset;
    u32 mind_index;
    u32 form_index;
} person_t;

typedef dck_stretchy_t (char, u32) person_names_t;

#define PERSON_COUNT 10

typedef struct {
    b32    map_collision[MAP_WIDTH * MAP_HEIGHT];
    tile_t map_tiles    [MAP_WIDTH * MAP_HEIGHT];
    f32    map_heights  [MAP_WIDTH * MAP_HEIGHT];

    person_names_t person_names;
    dck_stretchy_t (mind_t,   u32) person_minds;
    dck_stretchy_t (fdl_t,    u32) person_forms;
    dck_stretchy_t (person_t, u32) persons;
} world_t;

static void
mind_raise_stimul(mind_t *mind, stimul_t stimul)
{
    
}

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

static inline u32
hash_pcg(u32 input)
{
    u32 state = input * 747796405 + 2891336453;
    u32 word = ((state >> ((state >> 28) + 4)) ^ state) * 277803737;
    return (word >> 22) ^ word;
}

static inline f32
hash_pcg_norm(u32 input)
{
    return (hash_pcg(input) % 0xFFFF) / (f32)(0xFFFF);
}

static fdl_tree_t
generate_tree_fdl(u32 seed)
{
    u32 seed_off = hash_pcg(seed);

    return (fdl_tree_t) {
        .height     = hash_pcg_norm(seed_off + 0),
        .thickness  = hash_pcg_norm(seed_off + 1),
        .crown_span = hash_pcg_norm(seed_off + 2),
        .leafness   = hash_pcg_norm(seed_off + 3),
        .fruit      = hash_pcg(seed_off + 4) % FDL_FRUIT_COUNT,
        .seed       = seed,
    };
}


static void
draw_tree_tile(Olivec_Canvas canvas, fdl_tree_t tree)
{
    u32 trunk_width  = (canvas.width  * tree.thickness) / 2 + 1;
    u32 trunk_height = canvas.height * tree.height + 2;

    olivec_rect(canvas,
        (canvas.width - trunk_width) / 2,
        canvas.height - trunk_height,
        trunk_width,
        trunk_height,
        0xFF0066AA
    );

    u32 min_crown_r = canvas.width / 6;

    u32 crown_radius = min_crown_r + (canvas.width - min_crown_r) * tree.crown_span / 2;

    olivec_circle(canvas,
        canvas.width / 2,
        canvas.height - trunk_height,
        crown_radius,
        0xFF22CC11
    );

    if (tree.fruit != fdl_fruit_None) {
        u32 hash_off = hash_pcg(tree.seed);
        u32 fruit_color = fdl_fruit_color(tree.fruit);

        for (u32 i = 0; i < 5; ++i) {
            f32 angle = M_PI * 2.0f * hash_pcg_norm(hash_off + i * 2 + 0);
            f32 dist  = hash_pcg_norm(hash_off + i * 2 + 1);
            u32 x_off = cosf(angle)  * crown_radius * dist;
            u32 y_off = -sinf(angle) * crown_radius * dist;

            olivec_circle(canvas,
                canvas.width / 2 + x_off,
                canvas.height - trunk_height + y_off,
                2,
                fruit_color
            );
        }
    }
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

static u32
rand_color(void)
{
    return ((rand() % 256) << (0 * 8))
         | ((rand() % 256) << (1 * 8))
         | ((rand() % 256) << (2 * 8))
         | (255            << (3 * 8));
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
        "aáäbcčdďeéfghiíjklĺľmnňoóôpqrŕřsśštťuúvwxyýzźž"
        "AÁÄBCČDĎEÉFGHIÍJKLĹĽMNŇOÓÔPQRŔŘSŚŠTŤUÚVWXYÝZŹŽ"
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


static ivec2_t
world_rand_pos(world_t *world)
{
    ivec2_t res;

    do {
        res.x = rand() % MAP_WIDTH;
        res.y = rand() % MAP_HEIGHT;
    } while (world->map_collision[res.x + res.y * MAP_WIDTH]);

    world->map_collision[res.x + res.y * MAP_WIDTH] = true;

    return res;
}


void
world_init(world_t *world)
{
    *world = (world_t) {0};

    for (i32 y_pos = 0; y_pos < MAP_HEIGHT; ++y_pos) {
        for (i32 x_pos = 0; x_pos < MAP_WIDTH; ++x_pos) {
            i32 x_dist = x_pos - (MAP_WIDTH  / 2);
            i32 y_dist = y_pos - (MAP_HEIGHT / 2);
            f32 dist   = sqrtf(x_dist * x_dist + y_dist * y_dist);
            f32 random = (rand() % 2048) / 2047.0f;
            f32 dist_scale = dist / MAP_WIDTH / 2;
            world->map_heights[x_pos + y_pos * MAP_WIDTH] = random * (dist_scale);

            if (x_pos == 0 || x_pos == MAP_WIDTH  - 1
             || y_pos == 0 || y_pos == MAP_HEIGHT - 1)
            {
                world->map_tiles    [x_pos + y_pos * MAP_WIDTH] = tile_Forest;
                world->map_collision[x_pos + y_pos * MAP_WIDTH] = true;
            }
        }
    }

    for (u32 i = 0; i < PERSON_COUNT; ++i) {
        ivec2_t pos = world_rand_pos(world);

        u32 mind_index = world->person_minds.count;
        dck_stretchy_push(world->person_minds, (mind_t) {0});

        dck_stretchy_push(world->persons, (person_t) {
            .x_pos        = (MAP_WIDTH  / 4) + pos.x / 2,
            .y_pos        = (MAP_HEIGHT / 4) + pos.y / 2,
            .texture_tile = tile_Man,

            .properties  = PROP_NAME | PROP_THINKING,
            .name_offset = generate_name(&(world->person_names)),
            .mind_index  = mind_index,
        });
    }
}

void
world_update(world_t *world)
{
    for (u32 person_i = 0; person_i < world->persons.count; ++person_i) {
        person_t *person_p = world->persons.data + person_i;

        if (person_p->properties & PROP_THINKING) {
            mind_t *mind = world->person_minds.data + person_p->mind_index;

            for (u32 other_i = 0; other_i < world->persons.count; ++other_i) {
                
            }
        }
    }
}

i32
main(void)
{
    // srand(time(0));

    SetTraceLogLevel(LOG_WARNING);

    InitWindow(1920, 1080, "HearSay");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    const char *font_path = "res/JetBrainsMono-Regular.ttf";

    glob.font = load_font_in_such_a_way_that_i_dont_kill_raysan_with_a_hammer(font_path);

    Image atlas_image = GenImageColor(ATLAS_SIZE * TILE_RES, ATLAS_SIZE * TILE_RES, RED);
    Texture2D atlas = LoadTextureFromImage(atlas_image);
    UnloadImage(atlas_image);

    Texture2D textures[TILE_COUNT] = {
    #define O(m_name, m_path) \
        [tile_##m_name] = LoadTexture(m_path),
        TILE_TABLE
    #undef O
    };

    i32 screen_width;
    i32 screen_height;

    f32 tile_size = 128.0f;
    Vector2 screen_center = { MAP_WIDTH / 2, MAP_HEIGHT / 2 };

    world_t world;
    world_init(&world);

    u32 gen_buffer[TILE_RES * TILE_RES * GEN_BUF_CAPACITY];
    u32 gen_buffer_ids[GEN_BUF_CAPACITY];
    u32 gen_buffer_count = 0;

    u32 atlas_tile_count = 0;

    f32 update_waiter = 0.0f;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_Q))
            break;

        if (atlas_tile_count < ATLAS_SIZE * ATLAS_SIZE) {
            gen_buffer_count = 1;
            for (u32 tile_i = 0; tile_i < gen_buffer_count; ++tile_i) {
                u32 buf_offset = TILE_RES * TILE_RES * tile_i;
                Olivec_Canvas canvas = olivec_canvas(gen_buffer + buf_offset, TILE_RES, TILE_RES, TILE_RES);

                olivec_fill(canvas, 0);

                fdl_tree_t tree = generate_tree_fdl(rand());
                draw_tree_tile(canvas, tree);

                gen_buffer_ids[tile_i] = atlas_tile_count;

                u32 form_index = world.person_forms.count;
                dck_stretchy_push(world.person_forms, (fdl_t) {
                    .class = fdl_Tree,
                    .tree  = tree,
                });

                ivec2_t pos = world_rand_pos(&world);

                dck_stretchy_push(world.persons, (person_t) {
                    .x_pos        = pos.x,
                    .y_pos        = pos.y,
                    .texture_tile = TILE_GENERATED,
                    .atlas_id     = atlas_tile_count,
                    .properties   = PROP_FORM,
                    .form_index   = form_index,
                });

                ++atlas_tile_count;
            }
        }

        while (gen_buffer_count != 0) {
            u32 gen_buffer_i = gen_buffer_count - 1;
            u32 atlas_id = gen_buffer_ids[gen_buffer_i];

            if (atlas_id >= ATLAS_SIZE * ATLAS_SIZE)
                break;

            Rectangle atlas_rect = {
                .x      = (atlas_id % ATLAS_SIZE) * TILE_RES,
                .y      = (atlas_id / ATLAS_SIZE) * TILE_RES,
                .width  = TILE_RES,
                .height = TILE_RES,
            };

            UpdateTextureRec(atlas, atlas_rect, gen_buffer + TILE_RES * TILE_RES * gen_buffer_i);

            --gen_buffer_count;
        }

        f32 dt = GetFrameTime();

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

        update_waiter += dt;
        if (update_waiter >= (1.0f / UPDATE_FREQ)) {
            update_waiter -= (1.0f / UPDATE_FREQ);
            world_update(&world);
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
                tile_t tile = world.map_tiles[map_index];

                Vector2 pos = {
                    x_pos * tile_size + screen_origin.x,
                    y_pos * tile_size + screen_origin.y,
                };

                Color col = WHITE;
                col.a *= (1.0f - world.map_heights[map_index]);

                DrawTextureEx(textures[tile], pos, 0.0f, tile_size / (f32)TILE_RES, col);
            }
        }

        for (u32 person_i = 0; person_i < world.persons.count; ++person_i) {
            person_t person = world.persons.data[person_i];

            Vector2 pos = {
                person.x_pos * tile_size + screen_origin.x,
                person.y_pos * tile_size + screen_origin.y,
            };

            if (person.texture_tile == TILE_GENERATED) {
                Rectangle dest = {
                    .x      = pos.x,
                    .y      = pos.y,
                    .width  = tile_size,
                    .height = tile_size,
                };

                Rectangle source = {
                    .x      = (person.atlas_id % ATLAS_SIZE) * TILE_RES,
                    .y      = (person.atlas_id / ATLAS_SIZE) * TILE_RES,
                    .width  = TILE_RES,
                    .height = TILE_RES,
                };

                DrawTexturePro(atlas, source, dest, (Vector2) {0}, 0.0f, WHITE);
            }
            else {
                DrawTextureEx(textures[person.texture_tile], pos, 0.0f, tile_size / (f32)TILE_RES, WHITE);
            }

            if ((person.properties & PROP_NAME) != 0) {
                Vector2 text_pos = {
                    pos.x,
                    pos.y - tile_size * 0.75f,
                };

                draw_text(world.person_names.data + person.name_offset, text_pos, tile_size, tile_size * 0.5);
            }
        }

        EndDrawing();  
    }

    CloseWindow();

    printf("\\_/\n V\n");
    return 0;
}
