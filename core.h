#pragma once

#include <stdint.h>

typedef struct line_t {
    char *line;
    line_t *next;
};

typedef struct cursor_t {
    uint32_t row;
    uint32_t column;
    cursor_t *next;
};

typedef struct {
    char *name;
    line_t *text_buffer;
    uint32_t current_string;    // The idx of the string that appears at the top of the current screen
    uint32_t cursor_row;
    uint32_t cursor_column;
} text_buffer_t;
