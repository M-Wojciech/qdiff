#include <stdio.h>
#include <git2.h>
#include <unistd.h>
#include <ncurses.h>
#include <libgen.h>
#include "commit_graph_node.h"
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
}

void display_parent_menu(commit_graph_node_t *node)
{
    clear();
    printw("Select a parent:\n");
    for (size_t i = 0; i < node->parent_count; ++i)
    {
        const char *message = git_commit_message(node->parents[i]->commit);
        printw("[%zu] %s\n", i, message);
    }
    printw("Enter the number of the parent to select: ");
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
    commit_graph_walk_t *walk = commit_graph_walk_init(head_commit);

    // ncurses initialization
    initscr();
    cbreak();
    noecho();

    // Display starting commit
    git_tree *commit_tree = NULL;
    commit_graph_node_t *current_node = walk->current;
    display_commit_data(current_node->commit);
    error = git_commit_tree(&commit_tree, current_node->commit);
    libgit_error_check(error);
    display_file_data(repo, commit_tree, filename);

    // User input loop
    int user_input;
    while ((user_input = getch()) != 'q')
    {
        switch (user_input)
        {
        case 'h': // Navigate to a parent commit
            fetch_parents(current_node, repo); // Ensure parents are fetched
            if (current_node->parent_count > 0)
            {
                display_parent_menu(current_node);
                echo();
                int parent_choice;
                scanw("%d", &parent_choice);
                noecho();

                if (parent_choice >= 0 && (size_t)parent_choice < current_node->parent_count)
                {
                    clear();
                    current_node = commit_graph_walk_to_parent(walk, parent_choice);
                    display_commit_data(current_node->commit);
                    error = git_commit_tree(&commit_tree, current_node->commit);
                    libgit_error_check(error);
                    display_file_data(repo, commit_tree, filename);
                }
            }
            break;
        case 'l': // Navigate back to the child commit
            if (current_node->child)
            {
                clear();
                current_node = commit_graph_walk_to_child(walk);
                display_commit_data(current_node->commit);
                error = git_commit_tree(&commit_tree, current_node->commit);
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
