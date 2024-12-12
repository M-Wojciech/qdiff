#include <stdio.h>
#include <git2.h>
#include <unistd.h>
#include <ncurses.h>
#include <libgen.h>
#include "commit_graph_walk.h"
#include "windows.h"

commit_display *l_display;
commit_display *r_display;

void handle_resize_sig(int sig)
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
    commit_graph_walk_t *l_walk = commit_graph_walk_init(head_commit, filename);
    git_commit_free(head_commit);

    // ncurses initialization and window setup
    initscr();
    signal(SIGWINCH, handle_resize_sig);
    cbreak();
    noecho();
    curs_set(0);
    l_display = commit_display_init(LINES, COLS, 0, 0, l_walk);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_CYAN, -1);
    clear();
    refresh();
    commit_display *active = l_display;

    // Display starting commit
    commit_display_update(l_display);
    doupdate();

    // User input loop
    int user_input;
    while ((user_input = getch()) != 'q')
    {
        switch (user_input)
        {
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
                active = l_display;
                commit_display_free(r_display);
                r_display = NULL;
                handle_resize_sig(-1);
            }
            else
            {
                commit_graph_walk_t *r_walk = malloc(sizeof(commit_graph_walk_t));
                r_walk->current = l_walk->current;
                r_display = commit_display_init(LINES, COLS, 0, 0, r_walk);
                commit_display_update(r_display);
                handle_resize_sig(-1);
            }
            break;
        default:
            if (active->menu_state)
            {
                switch (user_input) // handle h, j, k, l on menu display
                {
                case 'h':
                    break;
                case 'l':
                    break;
                case 'j':
                    break;
                case 'k':
                    break;
                }
            }
            else
            {
                switch (user_input) // handle h, j, k, l on file display
                {
                case 'h':
                    if (active->walk->current->ancestor_count > 0)
                    {
                    }
                    else
                    {
                        beep();
                    }
                    break;
                case 'l':
                    if (active->walk->current->descendant)
                    {
                    }
                    else
                    {
                        beep();
                    }
                    break;
                case 'j':
                    break;
                case 'k':
                    break;
                }
            }
            break;
        }
    }

    // Cleanup
    clear();
    endwin();
    commit_graph_walk_free(l_walk);
    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0;
}
