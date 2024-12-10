#include <stdio.h>
#include <git2.h>
#include <unistd.h>
#include <ncurses.h>
#include <libgen.h>
#include "commit_graph_walk.h"

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

void display_commit_data(git_commit *commit)
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
    git_tree_entry_free(entry);
}

void display_ancestor_menu(commit_graph_node_t *node)
{
    clear();
    printw("Select an ancestor:\n");
    for (size_t i = 0; i < node->ancestor_count; ++i)
    {
        const char *message = git_commit_message(node->ancestors[i]->commit);
        printw("[%zu] %s\n", i, message);
    }
    printw("Enter the number of the ancestor to select: ");
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

    // Initialize commit graph walk
    commit_graph_walk_t *walk = commit_graph_walk_init(head_commit, filename);

    // ncurses initialization
    initscr();
    cbreak();
    noecho();

    // Display starting commit
    git_tree *commit_tree = NULL;
    display_commit_data(walk->current->commit);
    error = git_commit_tree(&commit_tree, walk->current->commit);
    libgit_error_check(error);
    display_file_data(repo, commit_tree, filename);

    // User input loop
    int user_input;
    while ((user_input = getch()) != 'q')
    {
        switch (user_input)
        {
        case 'h': // Navigate to an ancestor commit
            if (walk->current->ancestor_count > 0)
            {
                display_ancestor_menu(walk->current);
                echo();
                int ancestor_choice;
                scanw("%d", &ancestor_choice);
                noecho();

                if (ancestor_choice >= 0 && (size_t)ancestor_choice < walk->current->ancestor_count)
                {
                    clear();
                    commit_graph_walk_to_ancestor(walk, ancestor_choice);
                    display_commit_data(walk->current->commit);
                    error = git_commit_tree(&commit_tree, walk->current->commit);
                    libgit_error_check(error);
                    display_file_data(repo, commit_tree, filename);
                }
            }
            break;
        case 'l': // Navigate back to the descendant commit
            if (walk->current->descendant)
            {
                clear();
                commit_graph_walk_to_descendant(walk);
                display_commit_data(walk->current->commit);
                error = git_commit_tree(&commit_tree, walk->current->commit);
                libgit_error_check(error);
                display_file_data(repo, commit_tree, filename);
            }
            break;
        }
    }

    // Cleanup
    endwin();
    commit_graph_walk_free(walk);
    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0;
}
