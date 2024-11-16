#include <stdio.h>
#include <git2.h>
#include <unistd.h>
#include <ncurses.h>
#include <libgen.h>
#include "commit_stack.h"


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

void display_commit_data(git_commit * commit)
{
    const git_oid *commit_oid = git_commit_id(commit);
    const char *message = git_commit_message(commit);
    printw("Commit: %s\nMessage: %s\n", git_oid_tostr_s(commit_oid), message);
}

void display_file_data(git_repository *repo, git_tree *tree, const char *file)
{
    git_tree_entry *entry = NULL;
    git_blob *blob = NULL;

    int error = git_tree_entry_bypath(&entry, tree, file);
    libgit_error_check(error);
    error = git_blob_lookup(&blob, repo, git_tree_entry_id(entry));
    libgit_error_check(error);
    printw("File content:\n%s\n", (const char *)git_blob_rawcontent(blob));
}

int load_parent_commit(git_commit **out, git_commit *commit)
{
    int error;
    git_commit * parent;
    if ((error = git_commit_parent(&parent, commit, 0)) < 0)
    {
        return error;
    }
    *out = parent;
    return 0;
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

    // libgit initialization and looking if there is repository in given localization
    git_libgit2_init();
    git_repository *repo = NULL;
    int error = git_repository_open_ext(&repo, filepath, 0, NULL);
    libgit_error_check(error);

    // variables for walking git tree
    git_commit * current_commit = NULL;
    commit_stack_t commit_history;
    commit_history.top = NULL;
    git_tree *commit_tree = NULL;

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
    error = git_commit_tree(&commit_tree, current_commit);
    libgit_error_check(error);
    display_file_data(repo, commit_tree, filename);

    // user input loop
    int user_input;
    while ((user_input = getch()) != 'q')
    {
        switch (user_input)
        {
        case 'h':
            if(git_commit_parentcount(current_commit) > 0)
            {
                clear();
                push_commit_to_stack(&commit_history, current_commit);
                error = load_parent_commit(&current_commit, current_commit);
                libgit_error_check(error);
                display_commit_data(current_commit);
                error = git_commit_tree(&commit_tree, current_commit);
                libgit_error_check(error);
                display_file_data(repo, commit_tree, filename);
            }
            break;
        case 'l':
            if (commit_history.top != NULL)
            {
                clear();
                git_commit_free(current_commit);
                current_commit = pop_commit_from_stack(&commit_history);
                display_commit_data(current_commit);
                error = git_commit_tree(&commit_tree, current_commit);
                libgit_error_check(error);
                display_file_data(repo, commit_tree, filename);
            }
            break;
        }
    }

    endwin();
    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0;
}
