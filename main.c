#include <stdio.h>
#include <git2.h>
#include <unistd.h>
#include <ncurses.h>
#include "commit_stack.h"


void libgit_error_check(int error)
{
    if (error < 0)
    {
        const git_error *e = git_error_last();
        printf("Error %d/%d: %s\n", error, e->klass, e->message);
        exit(error);
    }
}

void display_commit_data(git_commit * commit)
{
    clear();
    const git_oid *commit_oid = git_commit_id(commit);
    const char *message = git_commit_message(commit);
    printw("Commit: %s\nMessage: %s\n", git_oid_tostr_s(commit_oid), message);
}

void load_parent_commit(git_commit **out, git_commit *commit)
{
    git_commit * parent;
    int error = git_commit_parent(&parent, commit, 0);
    libgit_error_check(error);
    display_commit_data(parent);
    *out = parent;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Missing filename\n");
        return 1;
    }
    const char *filename = argv[1];

    // libgit initialization and looking if there is repository in given localization
    git_libgit2_init();
    git_repository *repo = NULL;
    int error = git_repository_open_ext(&repo, filename, 0, NULL);
    libgit_error_check(error);

    // variables for walking git tree
    git_commit * current_commit = NULL;
    git_commit * tmp_commit_holder = NULL;
    commit_stack_t commit_history;
    commit_history.top = NULL;

    // getting starting data (commit pointed by HEAD)
    git_oid commit_oid;
    error = git_reference_name_to_id(&commit_oid, repo, "HEAD");
    libgit_error_check(error);
    error = git_commit_lookup(&current_commit, repo, &commit_oid);
    libgit_error_check(error);

    // ncurses initialization and displaying starting data
    initscr();
    cbreak();
    noecho();
    display_commit_data(current_commit);

    // user input loop
    int user_input;
    while ((user_input = getch()) != 'q')
    {
        switch (user_input)
        {
        case 'h':
            if(git_commit_parentcount(current_commit) > 0)
            {
                load_parent_commit(&tmp_commit_holder, current_commit);
                push_commit_to_stack(&commit_history, current_commit);
                current_commit = tmp_commit_holder;
            }
            break;
        case 'l':
            if (commit_history.top != NULL)
            {
                git_commit_free(current_commit);
                current_commit = pop_commit_from_stack(&commit_history);
                display_commit_data(current_commit);
            }
            break;
        }
    }

    endwin();
    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0;
}
