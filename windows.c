#include <string.h>
#include "windows.h"

#define DIFF_CONTEXT 0
#define DIFF_ADDITION 1
#define DIFF_DELETION 2

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
    display->buffer = NULL;
    display->buffer_lines_count = 0;

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
    wclear(display->file_content);
    int y = 0, x;
    int max_y = getmaxy(display->file_content);
    for (int i = display->y_offset; i < display->buffer_lines_count && y < max_y - 1; i++)
    {
        for (int j = 0; j < display->buffer[i]->lines_before && y < max_y - 1; j++)
        {
            wprintw(display->file_content, "\n");
            y++;
        }
        if (y >= max_y - 1)
        {
            break;
        }
        // Apply color based on diff mark
        switch (display->buffer[i]->diif_mark)
        {
        case DIFF_ADDITION:
            wattron(display->file_content, COLOR_PAIR(2)); // Green for additions
            break;
        case DIFF_DELETION:
            wattron(display->file_content, COLOR_PAIR(3)); // Red for deletions
            break;
        }

        // Print the line
        wprintw(display->file_content, display->buffer[i]->text);

        // Reset color
        wattroff(display->file_content, COLOR_PAIR(2));
        wattroff(display->file_content, COLOR_PAIR(3));

        getyx(display->file_content, y, x);
    }
    wnoutrefresh(display->file_content);
}

void commit_display_load_buffer(commit_display *display)
{
    // get blob data
    git_blob *blob = NULL;
    git_blob_lookup(&blob, git_commit_owner(display->walk->current->commit), git_tree_entry_id(display->walk->current->entry));
    size_t blob_size = git_blob_rawsize(blob);
    const char *blob_content = git_blob_rawcontent(blob);

    // count the number of lines in blob
    size_t blob_lines = 0;
    for (size_t i = 0; i < blob_size; i++)
    {
        if (blob_content[i] == '\n')
        {
            blob_lines++;
        }
    }
    blob_lines += (blob_size > 0 && blob_content[blob_size - 1] != '\n') ? 1 : 0;

    // free unused lines in buffer (terminal downsized)
    for (int i = blob_lines; i < display->buffer_lines_count; i++)
    {
        free(display->buffer[i]->text);
        free(display->buffer[i]);
    }
    // reallocate to match lines in blob
    display->buffer = realloc(display->buffer, blob_lines * sizeof(line_data *));
    // allocate for each line (terminal upsized)
    for (int i = display->buffer_lines_count; i < blob_lines; i++)
    {
        display->buffer[i] = malloc(sizeof(line_data));
    }
    display->buffer_lines_count = blob_lines;

    // copying from blob to buffer
    size_t line_start = 0;
    size_t line_index = 0;
    for (size_t i = 0; i < blob_size; i++)
    {
        if (blob_content[i] == '\n' || i == blob_size - 1)
        {
            size_t line_length = i + 1 - line_start;
            display->buffer[line_index]->text = realloc(display->buffer[line_index]->text, line_length + 1);
            memcpy(display->buffer[line_index]->text, blob_content + line_start, line_length);
            display->buffer[line_index]->text[line_length] = '\0';
            display->buffer[line_index]->length = line_length;

            line_start += line_length;
            display->buffer[line_index]->diif_mark = 0;
            line_index++;
        }
    }

    git_blob_free(blob);
}

void commit_display_reset_diff(commit_display *display)
{
    if (!display)
    {
        return;
    }
    for (int i = 0; i < display->buffer_lines_count; i++)
    {
        display->buffer[i]->diif_mark = DIFF_CONTEXT;
        display->buffer[i]->lines_before = 0;
    }
}

void commit_display_get_diff(commit_display *old_display, commit_display *new_display)
{
    if (!old_display || !new_display)
    {
        return;
    }
    git_blob *old_blob = NULL;
    git_blob *new_blob = NULL;
    git_blob_lookup(&old_blob, git_commit_owner(old_display->walk->current->commit), git_tree_entry_id(old_display->walk->current->entry));
    git_blob_lookup(&new_blob, git_commit_owner(new_display->walk->current->commit), git_tree_entry_id(new_display->walk->current->entry));

    commit_display_reset_diff(old_display);
    commit_display_reset_diff(new_display);

    diff_payload *payload = malloc(sizeof(diff_payload));
    payload->old_display = old_display;
    payload->new_display = new_display;
    payload->lines_sync = 0;

    git_diff_blobs(old_blob, NULL, new_blob, NULL, NULL, NULL, NULL, NULL, diff_line_cb, payload);

    git_blob_free(old_blob);
    git_blob_free(new_blob);
    free(payload);
}

int diff_line_cb(const git_diff_delta *delta, const git_diff_hunk *hunk, const git_diff_line *line, void *payload)
{
    diff_payload *diff_data = (diff_payload *)payload;
    commit_display *old_display = diff_data->old_display;
    commit_display *new_display = diff_data->new_display;

    switch (line->origin)
    {
    case GIT_DIFF_LINE_ADDITION:
    case GIT_DIFF_LINE_ADD_EOFNL:
        if (line->new_lineno > 0 && line->new_lineno <= new_display->buffer_lines_count)
        {
            new_display->buffer[line->new_lineno - 1]->diif_mark = DIFF_ADDITION;
            diff_data->lines_sync++;
        }
        break;
    case GIT_DIFF_LINE_DELETION:
    case GIT_DIFF_LINE_DEL_EOFNL:
        if (line->old_lineno > 0 && line->old_lineno <= old_display->buffer_lines_count)
        {
            old_display->buffer[line->old_lineno - 1]->diif_mark = DIFF_DELETION;
            diff_data->lines_sync--;
        }
        break;
    case GIT_DIFF_LINE_CONTEXT:
        if (line->old_lineno > 0 && line->old_lineno <= old_display->buffer_lines_count)
        {
            old_display->buffer[line->old_lineno - 1]->diif_mark = DIFF_CONTEXT;
            old_display->buffer[line->old_lineno - 1]->lines_before = diff_data->lines_sync > 0 ? diff_data->lines_sync : 0;
        }
        if (line->new_lineno > 0 && line->new_lineno <= new_display->buffer_lines_count)
        {
            new_display->buffer[line->new_lineno - 1]->diif_mark = DIFF_CONTEXT;
            new_display->buffer[line->new_lineno - 1]->lines_before = diff_data->lines_sync < 0 ? -diff_data->lines_sync : 0;
        }
        diff_data->lines_sync = 0;
        break;
    }

    return 0;
}

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

void handle_resize(commit_display *l_display, commit_display *r_display)
{
    endwin();
    clear();
    refresh();
    if (l_display)
    {
        wresize(l_display->commit_info, 2, r_display ? COLS / 2 : COLS);
        mvwin(l_display->commit_info, 0, 0);
        wresize(l_display->file_content, LINES - 2, r_display ? COLS / 2 : COLS);
        mvwin(l_display->file_content, 2, 0);
        commit_display_update(l_display);
    }
    if (r_display)
    {
        wresize(r_display->commit_info, 2, (COLS - 1) / 2);
        mvwin(r_display->commit_info, 0, COLS / 2 + 1);
        wresize(r_display->file_content, LINES - 2, (COLS - 1) / 2);
        mvwin(r_display->file_content, 2, COLS / 2 + 1);
        commit_display_update(r_display);
    }
    doupdate();
}
