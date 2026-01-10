#pragma once

#include <stdint.h>

/* Operations:
 * - Insert one or multiple characters
 * - Insert text with new lines (increase the amount of strings)
 * - Delete one or several characters
 * - Delete some text with new lines (decrease the amount of strings)
*/

typedef struct {
    char *buf;
    struct lines {
        uint32_t fln;   // First line number
        uint32_t nl;    // Num lines
        uint32_t offsets[256];
    };
    uint32_t allocated_size;
    struct line_node_t *next;
} line_node_t;

typedef struct {
    line_node_t *first_node;
    uint32_t num_nodes;
    uint32_t line_range[2];
    struct group_node_t *next;
} group_node_t;

typedef struct {
    char *filaname;
    char *path;
    uint32_t id;
    uint32_t flags;
    group_node_t *root_node;
} text_buffer_t;

void set_current_buf(uint32_t id);
text_buffer_t *get_current_buf(uint32_t id);
void create_buf(char *name);
int is_buf_updated(uint32_t id);
void save_buf(uint32_t id);
void close_buf(uint32_t id);
