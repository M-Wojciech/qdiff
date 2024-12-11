#include <stdio.h>
#include <git2.h>
#include <unistd.h>
#include <ncurses.h>
#include <libgen.h>
#include "commit_graph_walk.h"
#include "windows.h"

commit_display *l_display;
commit_display *r_display;

void handle_resize(int sig)
{
    endwin();
    clear();
    if (r_display)
    {
        mvvline(0, COLS/2, '|', LINES);
    }
    refresh();
    if (l_display)
    {
        wresize(l_display->commit_info, 2, r_display ? COLS / 2 : COLS);
        wresize(l_display->file_content, LINES - 2, r_display ? COLS / 2 : COLS);
        mvwin(l_display->file_content, 2, 0);
        commit_display_refresh(l_display);
    }
    if (r_display)
    {
        wresize(r_display->commit_info, 2, (COLS - 1) / 2);
        mvwin(l_display->commit_info, 0, COLS / 2 + 1);
        wresize(r_display->file_content, LINES - 2, (COLS - 1) / 2);
        mvwin(r_display->file_content, 2, COLS / 2 + 1);
        commit_display_refresh(r_display);
    }
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
    commit_graph_walk_t *walk = commit_graph_walk_init(head_commit, filename);
    git_commit_free(head_commit);

    // ncurses initialization and setup
    initscr();
    signal(SIGWINCH, handle_resize);
    cbreak();
    noecho();
    curs_set(0);
    l_display = init_commit_display(LINES, COLS, 0, 0, walk);
    start_color();
    use_default_colors();
    init_pair(1, COLOR_CYAN, -1);
    refresh();

    // Display starting commit
    commit_display_refresh(l_display);


    // User input loop
    int user_input;
    while ((user_input = getch()) != 'q')
    {
        switch (user_input)
        {
        case 'h':
            if (walk->current->ancestor_count > 0)
            {
            }
            else
            {
                beep();
            }
            break;
        case 'l':
            if (walk->current->descendant)
            {
            }
            break;
        case 'j':
            if (walk->current->descendant)
            {
            }
            else
            {
                beep();
            }
            break;
        case 'k':
            if (walk->current->descendant)
            {
            }
            break;
        }
    }

    // Cleanup
    clear();
    endwin();
    commit_graph_walk_free(walk);
    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0;
}
