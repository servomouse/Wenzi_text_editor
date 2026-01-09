#include "core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NUM_BUFS 1000

text_buffer_t *root_buf;
text_buffer_t *curr_buf;
uint32_t curr_id;
uint32_t num_untitled;

extern void show_error(const char* message);

void set_current_buf(uint32_t id) {
    text_buffer_t *p = root_buf;
    for(;;) {
        if (p->id == id) {
            curr_buf = p;
            return;
        }
        p = p->next;
    }
    char err_buf[128] = {0};
    snprintf(err_buf, 128, "Error: requested non-existing buffer: %d! (%s %s:%d)\n", id, __FILE__, __FUNCTIONW__, __LINE__);
    show_error(err_buf);
}

text_buffer_t *get_current_buf(uint32_t id) {
    text_buffer_t *p = root_buf;
    for(;;) {
        if (p->id == id) {
            return p;
        }
        p = p->next;
    }
    char err_buf[128] = {0};
    snprintf(err_buf, 128, "Error: requested non-existing buffer: %d! (%s %s:%d)\n", id, __FILE__, __FUNCTIONW__, __LINE__);
    show_error(err_buf);
}

void add_buf(text_buffer_t *buf) {
    text_buffer_t *p = root_buf;
    uint32_t counter = 0;
    for(;;) {
        if (p->next == NULL) {
            p->next = buf;
            return;
        }
        p = p->next;
        if (counter++ > MAX_NUM_BUFS) {
            break;
        }
    }
}

void create_buf(char *name) {
    text_buffer_t *p = (text_buffer_t *)calloc(1 ,sizeof(text_buffer_t));
    p->name = name;
    p->id = curr_id++;
    if (name == NULL) {
        p->on_disk = 0;
        char *name_buf = (char *)calloc(16, sizeof(char));
        snprintf(name_buf, 16, "Untitled-%d", num_untitled++);
        p->name = name_buf;
    } else {
        p->on_disk = 1;
        p->name = strdup(name);
    }
    add_buf(p);
}

int is_buf_updated(uint32_t id) {

}

void save_buf(uint32_t id) {

}

void close_buf(uint32_t id) {

}
