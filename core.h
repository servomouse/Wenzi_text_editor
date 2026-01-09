#pragma once

#include <stdint.h>

typedef struct {
    char *line;
    struct line_t *next;
} line_t;

typedef struct {
    uint32_t row;
    uint32_t column;
    struct cursor_t *next;
} cursor_t;

typedef struct {
    char *name;
    line_t *text;
    uint32_t current_string;    // The idx of the string that appears at the top of the current screen
    uint32_t cursor_row;
    uint32_t cursor_column;
    uint32_t id;
    int on_disk;
    struct text_buffer_t *next;
} text_buffer_t;

void set_current_buf(uint32_t id);
text_buffer_t *get_current_buf(uint32_t id);
void create_buf(char *name);
int is_buf_updated(uint32_t id);
void save_buf(uint32_t id);
void close_buf(uint32_t id);
