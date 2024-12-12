#include "windows.h"

commit_display *commit_display_init(int height, int width, int starty, int startx, commit_graph_walk_t *walk)
{
    commit_display *display = malloc(sizeof(commit_display));
    if (!display)
    {
        return NULL;
    }
    display->commit_info = newwin(2, width, starty, startx);
    display->file_content = newwin(height - 2, width, starty + 2, startx);
    refresh();
    display->y_offset = 0;
    display->walk = walk;
    display->menu_state = 0;

    wattron(display->commit_info, COLOR_PAIR(1));

    return display;
}
void commit_display_free(commit_display *display)
{
    delwin(display->commit_info);
    delwin(display->file_content);
    free(display);
    display = NULL;
}

void commit_display_update(commit_display *display)
{
    commit_display_info_update(display);
    if (display->menu_state)
    {
        commit_display_menu_update(display);
    }
    else
    {
        commit_display_file_update(display);
    }
}

void commit_display_info_update(commit_display *display)
{
    if (!display)
    {
        return;
    }
    // clear window
    wclear(display->commit_info);
    // print commit info
    const git_oid *comit_oid = git_commit_id(display->walk->current->commit);
    const char *message = git_commit_message(display->walk->current->commit);
    mvwprintw(display->commit_info, 0, 0,"Commit: %s\nMessage: %s\n", git_oid_tostr_s(comit_oid), message);
    wnoutrefresh(display->commit_info);
}

void commit_display_file_update(commit_display *display)
{
    if (!display)
    {
        return;
    }
    // clear window
    wclear(display->file_content);
    // print file data
    git_blob *blob = NULL;
    git_blob_lookup(&blob, git_commit_owner(display->walk->current->commit), git_tree_entry_id(display->walk->current->entry));
    wprintw(display->file_content, "File content:\n%s\n", (const char *)git_blob_rawcontent(blob));
    wnoutrefresh(display->file_content);
}

void commit_display_menu_update(commit_display *display)
{
    if (!display)
    {
        return;
    }
    wnoutrefresh(display->file_content);
}
