#ifndef VTT_PARSER_H_
#define VTT_PARSER_H_

#include "core/utils.h"
#include "core/dck.h"

typedef struct
{
    u32 text_offset;
    u32 text_size;
    f32 time_start, time_end;
} vtt_word_t;

typedef struct
{
    dck_stretchy_t (u8,         u32) text;
    dck_stretchy_t (vtt_word_t, u32) words;
} vtt_data_t;

typedef struct
{
    u32 word_offset;
    u32 word_count;
} vtt_chunk_t;

vtt_chunk_t
vtt_parse_file(vtt_data_t *data, const char *file_path);

#endif // VTT_PARSER_H_

#ifdef VTT_PARSER_IMPL

#include <string.h>

#define IO_IMPLEMENTATION
#include "core/io.h"

#define VTT_PARSE_FAIL 0xFFFFFFFF

typedef struct
{
    u8 *text;
    u32 size;
    u32 pos;
} vtt_parser_t;

static b32
vtt_parser_empty(vtt_parser_t *parser)
{
    return parser->pos == parser->size;
}

static b32
vtt_parse_char(vtt_parser_t *parser, u8 c)
{
    if (vtt_parser_empty(parser) || parser->text[parser->pos] != c)
        return false;

    parser->pos++;
    return true;
}

static b32
vtt_parse_string(vtt_parser_t *parser, const char *str)
{
    u32 o = 0;

    while (parser->pos + o != parser->size && str[o] != 0) {
        if (str[o] != parser->text[parser->pos + o])
            return false;

        ++o;
    }

    if (str[o] == 0) {
        parser->pos += o;
        return true;
    }

    return false;
}

static void
vtt_parse_until_and_over(vtt_parser_t *parser, char c)
{
    while (!vtt_parser_empty(parser) && parser->text[parser->pos++] != c)
        ;;
}

static void
vtt_parse_next_line(vtt_parser_t *parser)
{
    vtt_parse_until_and_over(parser, '\n');
}

static vtt_word_t
vtt_parse_line_to_word(vtt_parser_t *parser, vtt_data_t *data)
{
    vtt_word_t res = {
        .text_offset = data->text.count,
    };

    u32 parser_off = parser->pos;
    u32 parser_end = parser->pos;

    while (!vtt_parser_empty(parser)) {
        u8 c = parser->text[parser->pos];
        parser_end = parser->pos;
        parser->pos++;

        if (c == '\n')
            break;
    }

    res.text_size = parser_end - parser_off;

    dck_stretchy_reserve(data->text, res.text_size);
    memcpy(data->text.data + res.text_offset,
           parser->text + parser_off,
           res.text_size);

    data->text.count += res.text_size;

    return res;
}

static u32
vtt_parse_number(vtt_parser_t *parser, u32 digits)
{
    u32 res = 0;

    for (u32 i = 0; i < digits; ++i) {
        if (vtt_parser_empty(parser))
            return VTT_PARSE_FAIL;

        u8 c = parser->text[parser->pos];
        if (c < '0' || c > '9')
            return VTT_PARSE_FAIL;

        parser->pos++;

        res *= 10;
        res += c - '0';
    }

    return res;
}

static u32
vtt_parse_time_stamp(vtt_parser_t *parser)
{
    u32 hours = vtt_parse_number(parser, 2);
    if (hours == VTT_PARSE_FAIL || !vtt_parse_char(parser, ':'))
        return VTT_PARSE_FAIL;

    u32 mins = vtt_parse_number(parser, 2);
    if (mins == VTT_PARSE_FAIL || !vtt_parse_char(parser, ':'))
        return VTT_PARSE_FAIL;

    u32 secs = vtt_parse_number(parser, 2);
    if (secs == VTT_PARSE_FAIL || !vtt_parse_char(parser, '.'))
        return VTT_PARSE_FAIL;

    u32 msecs = vtt_parse_number(parser, 3);
    if (msecs == VTT_PARSE_FAIL)
        return VTT_PARSE_FAIL;

    return msecs
         + secs  * 1000
         + mins  * 1000 * 60
         + hours * 1000 * 60 * 60;
}

static b32
vtt_parse_time_range(vtt_parser_t *parser, u32 *start, u32 *end)
{
    *start = vtt_parse_time_stamp(parser);
    if (*start == VTT_PARSE_FAIL)
        return false;

    if (!vtt_parse_string(parser, " --> "))
        return false;

    *end = vtt_parse_time_stamp(parser);
    if (*end == VTT_PARSE_FAIL)
        return false;

    return true;
}

vtt_word_t
vtt_parse_word(vtt_parser_t *parser, vtt_data_t *data)
{
    vtt_word_t res = {
        .text_offset = data->text.count,
    };

    u32 parser_off = parser->pos;

    while (!vtt_parser_empty(parser)) {
        u8 c = parser->text[parser->pos];

        if (c == '\n' || c == '<')
            break;

        parser->pos++;
    }

    res.text_size = parser->pos - parser_off;

    dck_stretchy_reserve(data->text, res.text_size);
    memcpy(data->text.data + res.text_offset,
           parser->text + parser_off,
           res.text_size);
    data->text.count += res.text_size;

    return res;
}

void
vtt_parse_line(vtt_parser_t *parser, vtt_data_t *data, u32 ts_start, u32 ts_end)
{
    if (vtt_parser_empty(parser))
        return;

    if (parser->text[parser->pos] == '\n') {
        parser->pos++;
        return;
    }

    vtt_word_t word = vtt_parse_word(parser, data);
    word.time_start = ts_start / 1000.0f;

    while (!vtt_parser_empty(parser)) {
        if (parser->text[parser->pos] == '\n') {
            parser->pos++;
            break;
        }

        if (!vtt_parse_char(parser, '<')) {
            vtt_word_t word_extension = vtt_parse_word(parser, data);
            word.text_size += word_extension.text_size;
        }

        u32 time_stamp = vtt_parse_time_stamp(parser);
        if (time_stamp == VTT_PARSE_FAIL) {
            vtt_parse_until_and_over(parser, '>');
            continue;
        }

        if (!vtt_parse_char(parser, '>')) {
            // The time stamp text is corrupted, skip to the end of the line and finish
            vtt_parse_next_line(parser);
            break;
        }

        if (!vtt_parse_string(parser, "<c>"))
            continue;

        word.time_end = time_stamp / 1000.0f;
        dck_stretchy_push(data->words, word);

        word = vtt_parse_word(parser, data);
        word.time_start = time_stamp / 1000.0f;

        if (!vtt_parse_string(parser, "</c>"))
            continue;
    }

    word.time_end = ts_end / 1000.0f;
    dck_stretchy_push(data->words, word);
}

vtt_chunk_t
vtt_parse_file(vtt_data_t *data, const char *file_path)
{
    size_t file_size;
    u8 *file_data = io_read_file(file_path, &file_size);
    ASSERT(file_data);

    vtt_parser_t parser = {
        .text = file_data,
        .size = (u32)file_size,
    };

    vtt_chunk_t res = {
        .word_offset = data->words.count,
    };

    while (!vtt_parser_empty(&parser)) {
        u32 ts_start, ts_end;

        if (!vtt_parse_time_range(&parser, &ts_start, &ts_end)) {
            vtt_parse_next_line(&parser);
            continue;
        }

        vtt_parse_next_line(&parser); // Finish parsing the time range header.

        vtt_parse_next_line(&parser); // Skip the previous text line.

        vtt_parse_line(&parser, data, ts_start, ts_end);
    }

    free(file_data);

    res.word_count = data->words.count - res.word_offset;
    return res;
}

#endif // VTT_PARSER_IMPL

