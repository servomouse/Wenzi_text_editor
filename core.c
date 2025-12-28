#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

text_buffer_t *root_buf;
text_buffer_t *curr_buf;
uint32_t curr_id;

void set_current_buf(uint32_t id);
text_buffer_t get_current_buf(uint32_t id);
void create_buf(char *name);
int is_buf_updated(uint32_t id);
void save_buf(uint32_t id);
void close_buf(uint32_t id);
