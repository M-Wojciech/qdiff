#include <stdio.h>
#include <git2.h>
#include <unistd.h>
#include <ncurses.h>
#include <libgen.h>
#include "commit_graph_walk.h"
#include "windows.h"

void libgit_error_check(int error)
{
    if (error < 0)
    {
        endwin();
        const git_error *e = git_error_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
        git_libgit2_shutdown();
        exit(error);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Missing filepath\n");
        return 1;
    }
    const char *filepath = argv[1];
    const char *filename = basename(argv[1]);

    // libgit initialization and looking if there is a repository in the given location
    git_libgit2_init();
    git_repository *repo = NULL;
    int error = git_repository_open_ext(&repo, filepath, 0, NULL);
    libgit_error_check(error);

    // Getting starting data (commit pointed by HEAD)
    git_commit *head_commit = NULL;
    git_oid commit_oid;
    error = git_reference_name_to_id(&commit_oid, repo, "HEAD");
    libgit_error_check(error);
    error = git_commit_lookup(&head_commit, repo, &commit_oid);
    libgit_error_check(error);

    // Initialize commit graph walk and free unnecesary commit object
    commit_graph_walk_t *hold_walk = commit_graph_walk_init(head_commit, filename);
    git_commit_free(head_commit);

    // ncurses initialization and window setup
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    commit_display *l_display = NULL;
    commit_display *r_display = NULL;
    l_display = commit_display_init(LINES, COLS, 0, 0, hold_walk);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_CYAN, -1);
    init_pair(2, COLOR_GREEN, -1);
    init_pair(3, COLOR_RED, -1);
    clear();
    refresh();
    commit_display *active = l_display;

    // Display starting commit
    commit_display_load_buffer(l_display);
    commit_display_update(l_display);
    doupdate();

    // User input loop
    int user_input;
    while ((user_input = getch()) != 'q')
    {
        switch (user_input)
        {
        case KEY_RESIZE:
            handle_resize(l_display, r_display);
            break;
        case 'i':
            if (r_display)
            {
                if (active == l_display)
                {
                    active = r_display;
                }
                else
                {
                    active = l_display;
                }
            }
            break;
        case 's':
            if (r_display)
            {
                if (active == l_display)
                {
                    commit_display_free(r_display);
                }
                else
                {
                    commit_display_free(l_display);
                    l_display = r_display;
                    l_display->walk->current = r_display->walk->current;
                    active = r_display;
                }
                r_display = NULL;
                commit_display_reset_diff(active);
                handle_resize(l_display, r_display);
            }
            else
            {
                hold_walk->current = active->walk->current;
                r_display = commit_display_init(LINES, COLS, 0, 0, hold_walk);
                commit_display_load_buffer(r_display);
                commit_display_update(r_display);
                handle_resize(l_display, r_display);
            }
            break;
        default:
            if (active->menu_state)
            {
                switch (user_input) // handle h, j, k, l on menu display
                {
                case 'h':
                case KEY_LEFT:
                    commit_graph_walk_to_ancestor(active->walk, active->menu_state - 1);
                    active->menu_state = 0;
                    active->y_offset = 0;
                    commit_display_load_buffer(active);
                    commit_display_get_diff(l_display, r_display);
                    if (r_display)
                    {
                        commit_display_update_file(active == r_display ? l_display : r_display);
                    }
                    commit_display_update(active);
                    doupdate();
                    break;
                case KEY_RIGHT:
                case 'l':
                    active->menu_state = 0;
                    commit_display_update_file(active);
                    doupdate();
                    break;
                case KEY_DOWN:
                case 'j':
                    if (active->menu_state < active->walk->current->ancestor_count)
                    {
                        active->menu_state++;
                        commit_display_update_menu(active);
                        doupdate();
                    }
                    else
                    {
                        beep();
                    }
                    break;
                case KEY_UP:
                case 'k':
                    if (active->menu_state > 1)
                    {
                        active->menu_state--;
                        commit_display_update_menu(active);
                        doupdate();
                    }
                    else
                    {
                        beep();
                    }
                    break;
                }
            }
            else
            {
                switch (user_input) // handle h, j, k, l on file display
                {
                case KEY_LEFT:
                case 'h':
                    if (active->walk->current->ancestor_count > 1)
                    {
                        active->menu_state = 1;
                        commit_display_update_menu(active);
                        doupdate();
                    }
                    else if (active->walk->current->ancestor_count > 0)
                    {
                        commit_graph_walk_to_ancestor(active->walk, 0);
                        active->y_offset = 0;
                        commit_display_load_buffer(active);
                        commit_display_get_diff(l_display, r_display);
                        if (r_display)
                        {
                            commit_display_update_file(active == r_display ? l_display : r_display);
                        }
                        commit_display_update(active);
                        doupdate();
                    }
                    else
                    {
                        beep();
                    }
                    break;
                case KEY_RIGHT:
                case 'l':
                    if (active->walk->current->descendant)
                    {
                        commit_graph_walk_to_descendant(active->walk);
                        active->y_offset = 0;
                        commit_display_load_buffer(active);
                        commit_display_get_diff(l_display, r_display);
                        if (r_display)
                        {
                            commit_display_update_file(active == r_display ? l_display : r_display);
                        }
                        commit_display_update(active);
                        doupdate();
                    }
                    else
                    {
                        beep();
                    }
                    break;
                case KEY_DOWN:
                case 'j':
                    if (active->y_offset < active->buffer_lines_count)
                    {
                        active->y_offset++;
                        commit_display_update(active);
                        doupdate();
                    }
                    else
                    {
                        beep();
                    }
                    break;
                case KEY_UP:
                case 'k':
                    if (active->y_offset > 0)
                    {
                        active->y_offset--;
                        commit_display_update(active);
                        doupdate();
                    }
                    else
                    {
                        beep();
                    }
                    break;
                }
            }
            break;
        }
    }

    // Cleanup
    clear();
    nocbreak();
    attrset(A_NORMAL);
    endwin();
    system("stty sane");
    commit_graph_walk_free(hold_walk);
    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0;
}
