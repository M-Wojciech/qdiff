#include "windows.h"

commit_display *commit_display_init(int height, int width, int starty, int startx, commit_graph_walk_t *walk)
{
    commit_display *display = malloc(sizeof(commit_display));
    display->walk = malloc(sizeof(commit_graph_walk_t));
    if (!display)
    {
        return NULL;
    }
    display->commit_info = newwin(2, width, starty, startx);
    display->file_content = newwin(height - 2, width, starty + 2, startx);
    refresh();
    // display->y_offset = 0;
    display->walk->current = walk->current;
    display->menu_state = 0;
    display->blob_buffer = NULL;

    wattron(display->commit_info, COLOR_PAIR(1));

    return display;
}

void commit_display_free(commit_display *display)
{
    delwin(display->commit_info);
    delwin(display->file_content);
    free(display->walk);
    free(display);
    display = NULL;
}

void commit_display_update(commit_display *display)
{
    commit_display_update_info(display);
    if (display->menu_state)
    {
        commit_display_update_menu(display);
    }
    else
    {
        commit_display_update_file(display);
    }
}

void commit_display_update_info(commit_display *display)
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
    int free = COLS - 8;
    mvwprintw(display->commit_info, 0, 0, "Commit: %.*s", free, git_oid_tostr_s(comit_oid));
    mvwprintw(display->commit_info, 1, 0, "Message: %s", message);
    wnoutrefresh(display->commit_info);
}

void commit_display_update_file(commit_display *display)
{
    if (!display)
    {
        return;
    }
    // do diff if split
    // if (r_display)
    // {
    //     wclear(l_display->file_content);
    //     wclear(r_display->file_content);
    //     commit_display_update_file_diff(l_display);
    //     commit_display_update_file_diff(r_display);
    // }
    // else
    // {
    // clear window
    wclear(display->file_content);
    // print file data
    git_blob *blob = NULL;
    git_blob_lookup(&blob, git_commit_owner(display->walk->current->commit), git_tree_entry_id(display->walk->current->entry));
    wprintw(display->file_content, "File content:\n%s\n", (const char *)git_blob_rawcontent(blob));
    // }
    wnoutrefresh(display->file_content);
}

// void commit_display_update_file_diff(commit_display *display)
// {
//     // Get blobs from both displays
//     git_blob *l_blob = NULL;
//     git_blob *r_blob = NULL;
//     git_blob_lookup(&l_blob, git_commit_owner(l_display->walk->current->commit), git_tree_entry_id(l_display->walk->current->entry));
//     git_blob_lookup(&r_blob, git_commit_owner(r_display->walk->current->commit), git_tree_entry_id(r_display->walk->current->entry));

//     diff_payload *payload = malloc(sizeof(diff_payload));
//     payload->display = malloc(sizeof(commit_display *));
//     payload->display = display;
//     payload->blob = malloc(sizeof(git_blob *));
//     payload->blob = l_blob;
//     payload->current_offset = 0;

//     git_diff *diff = NULL;
//     git_diff_blobs(r_blob, NULL, l_blob, NULL, NULL, NULL, NULL, NULL, diff_line_cb, payload);
//     wnoutrefresh(display->file_content);
// }

// int diff_line_cb(const git_diff_delta *delta, const git_diff_hunk *hunk, const git_diff_line *line, void *payload)
// {
//     diff_payload *payload_c = (diff_payload *)payload;
//     const char *blob_content = git_blob_rawcontent(payload_c->blob);
//     size_t blob_size = git_blob_rawsize(payload_c->blob);

//     // Print blob content up to this line
//     while (payload_c->current_offset < blob_size && payload_c->current_offset < line->content_offset)
//     {
//         wprintw(payload_c->display->file_content, "%c", blob_content[payload_c->current_offset]);
//         payload_c->current_offset++;
//     }

//     return 0;
// }

void commit_display_update_menu(commit_display *display)
{
    if (!display)
    {
        return;
    }
    // clear window
    wclear(display->file_content);
    // print menu options
    for (int i = 0; i < display->walk->current->ancestor_count; i++)
    {
        const git_oid *comit_oid = git_commit_id(display->walk->current->ancestors[i]->commit);
        const char *message = git_commit_message(display->walk->current->ancestors[i]->commit);
        if (i + 1 == display->menu_state)
        {
            wattron(display->file_content, COLOR_PAIR(2));
        }
        mvwprintw(display->file_content, i, 0, "%.6s\t%s", git_oid_tostr_s(comit_oid), message);
        wattroff(display->file_content, COLOR_PAIR(2));
    }

    wnoutrefresh(display->file_content);
}
