#include "windows.h"


commit_display *init_commit_display(int height, int width, int starty, int startx, commit_graph_walk_t *walk)
{
    commit_display *display = malloc(sizeof(commit_display));
    if (!display)
    {
        return NULL;
    }
    display->commit_info = newwin(2, width, starty, startx);
    display->file_content = newwin(height-2, width, starty+2, startx);
    refresh();
    display->y_offset = 0;
    display->walk = walk;

    wattron(display->commit_info, COLOR_PAIR(1));

    return display;
}

void commit_display_refresh(commit_display *display)
{
    if (!display)
    {
        return;
    }
    // clear windows
    wclear(display->commit_info);
    wclear(display->file_content);
    // print commit info
    const git_oid *comit_oid = git_commit_id(display->walk->current->commit);
    const char *message = git_commit_message(display->walk->current->commit);
    wprintw(display->commit_info ,"Commit: %s\nMessage: %s\n", git_oid_tostr_s(comit_oid), message);
    attroff(COLOR_PAIR(1));
    //print file data
    git_blob *blob = NULL;
    git_blob_lookup(&blob, git_commit_owner(display->walk->current->commit), git_tree_entry_id(display->walk->current->entry));
    wprintw(display->file_content, "File content:\n%s\n", (const char *)git_blob_rawcontent(blob));

    wrefresh(display->commit_info);
    wrefresh(display->file_content);
}
