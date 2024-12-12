#ifndef WINDOWS_H
#define WINDOWS_H

#include <ncurses.h>
#include "commit_graph_walk.h"

typedef struct {
    WINDOW *commit_info;
    WINDOW *file_content;
    int y_offset;
    commit_graph_walk_t *walk;
    int menu_state;
} commit_display;

commit_display *l_display;
commit_display *r_display;

commit_display *commit_display_init(int height, int width, int starty, int startx, commit_graph_walk_t *walk);
void commit_display_free(commit_display *display);
void commit_display_update(commit_display *display);
void commit_display_update_info(commit_display *display);
void commit_display_update_file(commit_display *display);
void commit_display_update_file_diff(commit_display *display);
void commit_display_update_menu(commit_display *display);



#endif
