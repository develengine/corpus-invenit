#include <raylib.h>

#include "core/utils.h"
#include "core/dck.h"
#include "core/lina.h"

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
    O(Forest, "res/forest.png") \
    \
    O(Tree,  "res/tree.png") \
    O(Rock,  "res/rock.png") \
    O(Apple, "res/apple.png") \
    \
    O(ManFront, "res/man_front.png") \
    O(ManBack,  "res/man_back.png") \
    O(ManLeft,  "res/man_left.png") \
    O(ManRight, "res/man_right.png") \
    \
    O(ManFrontGrab, "res/man_front_grab.png") \
    O(ManBackGrab,  "res/man_back_grab.png") \
    O(ManLeftGrab,  "res/man_left_grab.png") \
    O(ManRightGrab, "res/man_right_grab.png") \
/**/

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
    tile_None,
#define O(m_name, m_path) \
    tile_##m_name,
    TILE_TABLE
#undef O
    TILE_COUNT,
    TILE_GENERATED,
} tile_t;

#define TILE_RES 32

#define ZOOM_SPEED 1.125f

#define UPDATE_FREQ 4

#define NAME_FORMAT "%s %s"

struct {
    Font font;
} glob = {0};

typedef enum
{
    action_None,

    action_Up,
    action_Down,
    action_Left,
    action_Right,

    action_TurnForward,
    action_TurnBack,
    action_TurnLeft,
    action_TurnRight,

    action_Grab,
} action_t;

typedef enum
{
    dir_Front,
    dir_Back,
    dir_Left,
    dir_Right,

    DIR_COUNT
} dir_t;

typedef struct
{
    action_t action;
} output_t;

typedef struct
{
    tile_t tile;
} object_t;

typedef enum
{
    goal_Find,
    goal_GoTo,
    goal_PickUp,
    goal_Eat,
} goal_type_t;

typedef struct
{
    object_t object;
} goal_find_t;

typedef struct
{
    i32 x;
    i32 y;
} goal_go_to_t;

typedef struct
{
    goal_type_t type;
    union {
        goal_go_to_t go_to;
        goal_find_t  find;
    };
} goal_t;

#define GOAL_STACK_CAP 16
#define VIEW_DIST 7
#define VIEW_SIDE (VIEW_DIST * 2 + 1)

typedef enum
{
    symbol_Unknown,

    symbol_None,
    symbol_Tree,
    symbol_Rock,
    symbol_Apple,
    symbol_Person,
    symbol_Forest,
} symbol_t;

typedef struct
{
    goal_t goals_stack[GOAL_STACK_CAP];
    u32 goals_top;

    symbol_t view[VIEW_SIDE * VIEW_SIDE];
    ivec2_t  view_orig;
} soul_t;

static inline symbol_t
soul_view_get(soul_t *soul, ivec2_t coord)
{
    i32 x = (soul->view_orig.x + coord.x) % VIEW_SIDE;
    i32 y = (soul->view_orig.y + coord.y) % VIEW_SIDE;

    return soul->view[x + y * VIEW_SIDE];
}

static inline void
soul_view_set(soul_t *soul, ivec2_t coord, symbol_t symbol)
{
    i32 x = (soul->view_orig.x + coord.x) % VIEW_SIDE;
    i32 y = (soul->view_orig.y + coord.y) % VIEW_SIDE;

    soul->view[x + y * VIEW_SIDE] = symbol;
}

static inline void
soul_view_move(soul_t *soul, ivec2_t offset)
{
    soul->view_orig.x = (soul->view_orig.x + VIEW_SIDE + offset.x) % VIEW_SIDE;
    soul->view_orig.y = (soul->view_orig.y + VIEW_SIDE + offset.y) % VIEW_SIDE;
}

typedef struct
{
    ivec2_t pos;
    dir_t dir;
    b32 grabbing;

    u32 name_offset;

    i32 soul_index;

    output_t output;
} person_t;

typedef dck_stretchy_t (char, u32) person_names_t;

#define MAP_WIDTH  64
#define MAP_HEIGHT 64

#define PERSON_COUNT 2
#define ROCK_COUNT   50
#define TREE_COUNT   25
#define APPLE_COUNT  25

typedef struct {
    tile_t   map_tiles    [MAP_WIDTH * MAP_HEIGHT];
    b32      map_collision[MAP_WIDTH * MAP_HEIGHT];
    object_t map_objects  [MAP_WIDTH * MAP_HEIGHT];

    person_names_t person_names;
    dck_stretchy_t (person_t, u32) persons;
    dck_stretchy_t (soul_t,   u32) souls;
} world_t;

static inline symbol_t
world_view_get(world_t *world, person_t *person, ivec2_t coords)
{
    ivec2_t abs_pos = {
        .x = person->pos.x + coords.x,
        .y = person->pos.y + coords.y,
    };

    if (abs_pos.x < 0 || abs_pos.x >= MAP_WIDTH
     || abs_pos.y < 0 || abs_pos.y >= MAP_WIDTH)
        return symbol_Forest;

    unsigned tile_index = abs_pos.x + abs_pos.y * MAP_WIDTH;

    if (world->map_tiles[tile_index] == tile_Forest)
        return symbol_Forest;

    object_t object = world->map_objects[tile_index];
    switch (object.tile) {
        case tile_Rock:  return symbol_Rock;
        case tile_Tree:  return symbol_Tree;
        case tile_Apple: return symbol_Apple;
        case tile_None:  return symbol_None;

        default: return symbol_Unknown;
    }
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

    static dck_stretchy_t (u32, u32) codepoint_buffer = {0};

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
            if (x_pos == 0 || x_pos == MAP_WIDTH  - 1
             || y_pos == 0 || y_pos == MAP_HEIGHT - 1)
            {
                world->map_tiles    [x_pos + y_pos * MAP_WIDTH] = tile_Forest;
                world->map_collision[x_pos + y_pos * MAP_WIDTH] = true;
            }
            else {
                world->map_tiles    [x_pos + y_pos * MAP_WIDTH] = tile_Grass;
            }
        }
    }

    for (u32 i = 0; i < ROCK_COUNT; ++i) {
        ivec2_t pos = world_rand_pos(world);

        world->map_objects[pos.x + pos.y * MAP_WIDTH] = (object_t) {
            .tile = tile_Rock,
        };
    }

    for (u32 i = 0; i < TREE_COUNT; ++i) {
        ivec2_t pos = world_rand_pos(world);

        world->map_objects[pos.x + pos.y * MAP_WIDTH] = (object_t) {
            .tile = tile_Tree,
        };
    }

    for (u32 i = 0; i < APPLE_COUNT; ++i) {
        ivec2_t pos = world_rand_pos(world);

        world->map_objects[pos.x + pos.y * MAP_WIDTH] = (object_t) {
            .tile = tile_Apple,
        };
    }

    for (u32 i = 0; i < PERSON_COUNT; ++i) {
        ivec2_t pos = world_rand_pos(world);

        u32 soul_index = -1;

        if (i != 0) { // NOTE: Player doesn't have a soul.
            soul_index = world->souls.count;
            dck_stretchy_push(world->souls, (soul_t) {0});
        }

        dck_stretchy_push(world->persons, (person_t) {
            .pos          = pos,
            .name_offset  = generate_name(&(world->person_names)),
            .soul_index   = soul_index,
        });
    }
}

b32
action_is_move(action_t action)
{
    switch (action) {
        case action_Down:  return true;
        case action_Up:    return true;
        case action_Left:  return true;
        case action_Right: return true;

        default: return false;
    }
}

dir_t
move_to_dir(action_t action)
{
    switch (action) {
        case action_Down:  return dir_Front;
        case action_Up:    return dir_Back;
        case action_Left:  return dir_Left;
        case action_Right: return dir_Right;

        default: ASSERT(!"Illegal action type for moving!");
    }
}

b32
action_is_turn(action_t action)
{
    switch (action) {
        case action_TurnForward: return true;
        case action_TurnBack:    return true;
        case action_TurnLeft:    return true;
        case action_TurnRight:   return true;

        default: return false;
    }
}

dir_t
turn_to_dir(action_t action)
{
    switch (action) {
        case action_TurnForward: return dir_Front;
        case action_TurnBack:    return dir_Back;
        case action_TurnLeft:    return dir_Left;
        case action_TurnRight:   return dir_Right;

        default: ASSERT(!"Illegal action type for turning!");
    }
}

ivec2_t
facing_pos(ivec2_t pos, dir_t dir)
{
    switch (dir) {
        case dir_Front: return (ivec2_t) { .x = pos.x, .y = pos.y + 1 };
        case dir_Back:  return (ivec2_t) { .x = pos.x, .y = pos.y - 1 };
        case dir_Left:  return (ivec2_t) { .x = pos.x - 1, .y = pos.y };
        case dir_Right: return (ivec2_t) { .x = pos.x + 1, .y = pos.y };

        default: UNREACHABLE();
    }
}

f32
object_weight(object_t object)
{
    switch (object.tile) {
        case tile_None:  return 0.0f;
        case tile_Apple: return 0.5f;
        case tile_Rock:  return 50.0f;

        default: return 666.0f;
    }
}

u32
pos_to_index(ivec2_t pos)
{
    return pos.x + pos.y * MAP_WIDTH;
}

void
soul_update(soul_t *soul, person_t *person, world_t *world)
{
    // Shift the view to adjust for previous movement.
    switch (person->output.action) {
        case action_Up: {
            soul_view_move(soul, (ivec2_t) { 0,-1 });
            for (i32 i = 0; i < VIEW_SIDE; ++i) {
                soul_view_set(soul, (ivec2_t) { i, 0 }, symbol_Unknown);
            }
        } break;
        case action_Down: {
            soul_view_move(soul, (ivec2_t) { 0, 1 });
            for (i32 i = 0; i < VIEW_SIDE; ++i) {
                soul_view_set(soul, (ivec2_t) { i, VIEW_SIDE - 1 }, symbol_Unknown);
            }
        } break;
        case action_Left: {
            soul_view_move(soul, (ivec2_t) {-1, 0 });
            for (i32 i = 0; i < VIEW_SIDE; ++i) {
                soul_view_set(soul, (ivec2_t) { 0, i }, symbol_Unknown);
            }
        } break;
        case action_Right: {
            soul_view_move(soul, (ivec2_t) { 1, 0 });
            for (i32 i = 0; i < VIEW_SIDE; ++i) {
                soul_view_set(soul, (ivec2_t) { VIEW_SIDE - 1, i }, symbol_Unknown);
            }
        } break;
        default: break;
    }

    // Scan the view for changes and new stimuly.
    for (i32 y = 0; y < VIEW_SIDE; ++y) {
        for (i32 x = 0; x < VIEW_SIDE; ++x) {
            ivec2_t coords = { x, y };

            symbol_t world_symbol = world_view_get(world, person, coords);
            symbol_t soul_symbol  = soul_view_get(soul, coords);

            soul_view_set(soul, coords, world_symbol);
        }
    }
}

void
world_update(world_t *world)
{
    // Update souls first.
    for (u32 i = 0; i < world->persons.count; ++i) {
        person_t *person = world->persons.data + i;

        if (person->soul_index != -1) { // Skip soulless persons
            soul_t *soul = world->souls.data + person->soul_index;
            soul_update(soul, person, world);
        }
    }

    // Then update the earthly realm.
    for (u32 i = 0; i < world->persons.count; ++i) {
        person_t *person = world->persons.data + i;

        output_t output = person->output;

        action_t action = output.action;

        if (action == action_None)
            continue;

        if (action_is_move(action)) {
            u32 current_index = pos_to_index(person->pos);

            dir_t move_dir = move_to_dir(action);
            ivec2_t next_pos = facing_pos(person->pos, move_dir);
            u32 next_index = pos_to_index(next_pos);

            ivec2_t front_pos = facing_pos(person->pos, person->dir);
            u32 front_index = pos_to_index(front_pos);
            object_t object = world->map_objects[front_index];

            if (!person->grabbing || object.tile == tile_None) {
                if (!world->map_collision[next_index]) {
                    world->map_collision[current_index] = false;
                    world->map_collision[next_index]    = true;
                    person->pos = next_pos;
                }
            }
            else {
                f32 weight = object_weight(object);
                ivec2_t next_obj_pos = facing_pos(front_pos, move_dir);
                u32 next_obj_index = pos_to_index(next_obj_pos);

                if (weight < 100.0f) {
                    if ((!world->map_collision[next_index]     && !world->map_collision[next_obj_index])
                     || (!world->map_collision[next_index]     && next_obj_index == current_index)
                     || (!world->map_collision[next_obj_index] && next_index == front_index))
                    {
                        world->map_collision[current_index] = false;
                        world->map_collision[front_index]   = false;

                        world->map_collision[next_index]     = true;
                        world->map_collision[next_obj_index] = true;

                        person->pos = next_pos;
                        world->map_objects[next_obj_index] = world->map_objects[front_index];
                        world->map_objects[front_index] = (object_t) {0};
                    }
                }
            }
        }
        else if (action_is_turn(action)) {
            dir_t turn_dir = turn_to_dir(action);

            ivec2_t front_pos = facing_pos(person->pos, person->dir);
            u32 front_index = pos_to_index(front_pos);
            object_t object = world->map_objects[front_index];

            if (person->grabbing && object.tile != tile_None) {
                f32 weight = object_weight(object);
                ivec2_t next_obj_pos = facing_pos(person->pos, turn_dir);
                u32 next_obj_index = pos_to_index(next_obj_pos);

                if (weight < 100.0f && !world->map_collision[next_obj_index]) {
                    world->map_collision[front_index]    = false;
                    world->map_collision[next_obj_index] = true;

                    world->map_objects[next_obj_index] = world->map_objects[front_index];
                    world->map_objects[front_index] = (object_t) {0};
                }
                else if (front_index != next_obj_index) {
                    person->grabbing = false;
                }
            }

            person->dir = turn_dir;
        }
        else if (output.action == action_Grab) {
            person->grabbing = !person->grabbing;
        }
        else {
            fprintf(stderr, "Unknown action: (%d)!\n", action);
        }
    }
}

i32
main(void)
{
    srand(time(0));

    SetTraceLogLevel(LOG_WARNING);

    InitWindow(1920, 1080, "HearSay");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(60);

    const char *font_path = "res/JetBrainsMono-Regular.ttf";

    glob.font = load_font_in_such_a_way_that_i_dont_kill_raysan_with_a_hammer(font_path);

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

    f32 update_waiter = 0.0f;

    output_t player_output = {0};

    b32 pan = false;

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_Q))
            break;

        f32 dt = GetFrameTime();

        screen_width  = GetScreenWidth(); 
        screen_height = GetScreenHeight();

        tile_size += (tile_size * ZOOM_SPEED - tile_size) * GetMouseWheelMove();

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 delta = GetMouseDelta();
            screen_center.x -= delta.x / tile_size;
            screen_center.y -= delta.y / tile_size;
        }

        if (IsKeyPressed(KEY_F)) {
            ToggleBorderlessWindowed();
        }

        if (IsKeyPressed(KEY_P)) {
            pan = !pan;
        }

        if (IsKeyPressed(KEY_W)) player_output.action = action_Up;
        if (IsKeyPressed(KEY_A)) player_output.action = action_Left;
        if (IsKeyPressed(KEY_S)) player_output.action = action_Down;
        if (IsKeyPressed(KEY_D)) player_output.action = action_Right;

        if (IsKeyPressed(KEY_UP))    player_output.action = action_TurnBack;
        if (IsKeyPressed(KEY_LEFT))  player_output.action = action_TurnLeft;
        if (IsKeyPressed(KEY_DOWN))  player_output.action = action_TurnForward;
        if (IsKeyPressed(KEY_RIGHT)) player_output.action = action_TurnRight;

        if (IsKeyPressed(KEY_E)) player_output.action = action_Grab;

        if (update_waiter >= (1.0f / UPDATE_FREQ) * 0.5f) {
            if (IsKeyDown(KEY_W)) player_output.action = action_Up;
            if (IsKeyDown(KEY_A)) player_output.action = action_Left;
            if (IsKeyDown(KEY_S)) player_output.action = action_Down;
            if (IsKeyDown(KEY_D)) player_output.action = action_Right;

            if (IsKeyDown(KEY_UP))    player_output.action = action_TurnBack;
            if (IsKeyDown(KEY_LEFT))  player_output.action = action_TurnLeft;
            if (IsKeyDown(KEY_DOWN))  player_output.action = action_TurnForward;
            if (IsKeyDown(KEY_RIGHT)) player_output.action = action_TurnRight;

            if (IsKeyDown(KEY_E)) player_output.action = action_Grab;
        }

        update_waiter += dt;
        if (update_waiter >= (1.0f / UPDATE_FREQ)) {
            update_waiter -= (1.0f / UPDATE_FREQ);

            if (world.persons.count > 0) {
                world.persons.data[0].output = player_output;
            }

            world_update(&world);

            player_output = (output_t) {0};
        }

        BeginDrawing();

        ClearBackground(AIR_COLOR);

        if (!pan && world.persons.count > 0) {
            ivec2_t pos = world.persons.data[0].pos;
            screen_center.x = (f32)pos.x + 0.5f;
            screen_center.y = (f32)pos.y + 0.5f;
        }

        Vector2 screen_origin = {
            .x = (screen_width  / 2) - screen_center.x * tile_size,
            .y = (screen_height / 2) - screen_center.y * tile_size,
        };

        for (u32 y_pos = 0; y_pos < MAP_HEIGHT; ++y_pos) {
            for (u32 x_pos = 0; x_pos < MAP_WIDTH; ++x_pos) {
                u32 map_index = x_pos + y_pos * MAP_WIDTH;

                Vector2 pos = {
                    x_pos * tile_size + screen_origin.x,
                    y_pos * tile_size + screen_origin.y,
                };

                Color col = WHITE;

                tile_t tile = world.map_tiles[map_index];
                if (tile != tile_None) {
                    DrawTextureEx(textures[tile], pos, 0.0f, tile_size / (f32)TILE_RES, col);
                }

                tile_t object_tile = world.map_objects[map_index].tile;
                if (object_tile != tile_None) {
                    DrawTextureEx(textures[object_tile], pos, 0.0f, tile_size / (f32)TILE_RES, col);
                }
            }
        }

        for (u32 person_i = 0; person_i < world.persons.count; ++person_i) {
            person_t person = world.persons.data[person_i];

            Vector2 pos = {
                person.pos.x * tile_size + screen_origin.x,
                person.pos.y * tile_size + screen_origin.y,
            };

            tile_t tile = tile_ManFront + person.dir;
            if (person.grabbing) {
                tile += DIR_COUNT;
            }

            DrawTextureEx(textures[tile], pos, 0.0f, tile_size / (f32)TILE_RES, WHITE);

//            Vector2 text_pos = {
//                pos.x,
//                pos.y - tile_size * 0.75f,
//            };
//
//            draw_text(world.person_names.data + person.name_offset, text_pos, tile_size, tile_size * 0.5);
        }

        EndDrawing();  
    }

    CloseWindow();

    printf("\\_/\n V\n");
    return 0;
}
