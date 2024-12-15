#ifndef WINDOWS_H
#define WINDOWS_H

#include <ncurses.h>
#include "commit_graph_walk.h"

typedef struct windows
{
    char *text;
    int length;
    int diif_mark;
    int lines_before;
} line_data;

typedef struct
{
    WINDOW *commit_info;
    WINDOW *file_content;
    commit_graph_walk_t *walk;
    line_data **buffer;
    int buffer_lines_count;
    int y_offset;
    int menu_state;
} commit_display;

typedef struct
{
    commit_display *old_display;
    commit_display *new_display;
} diff_payload;

commit_display *commit_display_init(int height, int width, int starty, int startx, commit_graph_walk_t *walk);
void commit_display_free(commit_display *display);
void commit_display_update(commit_display *display);
void commit_display_load_buffer(commit_display *display);
void commit_display_update_info(commit_display *display);
void commit_display_update_file(commit_display *display);
void commit_display_update_menu(commit_display *display);
int diff_line_cb(const git_diff_delta *delta, const git_diff_hunk *hunk, const git_diff_line *line, void *payload);
void handle_resize(commit_display * l_display, commit_display *r_display);
void commit_display_get_diff(commit_display *old_display, commit_display *new_display);
void commit_display_reset_diff(commit_display *display);
int diff_line_cb(const git_diff_delta *delta, const git_diff_hunk *hunk, const git_diff_line *line, void *payload);

#endif
