#ifndef WINDOWS_H
#define WINDOWS_H

#include <ncurses.h>
#include "commit_graph_walk.h"


typedef struct {
    WINDOW *commit_info;
    WINDOW *file_content;
    int y_offset;
    commit_graph_walk_t *walk;
} commit_display;

commit_display *init_commit_display(int height, int width, int starty, int startx, commit_graph_walk_t *walk);
void commit_display_free(commit_display *display);
void commit_display_refresh(commit_display *display);



#endif
