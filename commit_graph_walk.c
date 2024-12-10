#include <stdlib.h>
#include <git2.h>
#include <stdio.h>
#include <unistd.h>
#include "commit_graph_walk.h"

commit_graph_walk_t *commit_graph_walk_init(git_commit *start_commit)
{
    commit_graph_walk_t *walk = malloc(sizeof(commit_graph_walk_t));
    if (!walk)
    {
        return NULL;
    }

    commit_graph_node_t *root = malloc(sizeof(commit_graph_node_t));
    if (!root)
    {
        free(walk);
        return NULL;
    }

    root->commit = start_commit;
    root->descendant = NULL;
    root->ancestors = NULL;
    root->ancestor_count = 0;
    fetch_parents(root);

    walk->current = root;
    return walk;
}

// Recursive free function for graph nodes
void commit_graph_free_node(commit_graph_node_t *node)
{
    if (!node)
    {
        return;
    }
    for (size_t i = 0; i < node->ancestor_count; i++)
    {
        free_node(node->ancestors[i]);
    }
    free(node->ancestors);
    git_commit_free(node->commit);
    free(node);
}

void commit_graph_walk_free(commit_graph_walk_t *walk)
{
    if (!walk)
    {
        return;
    }
    // get graph root
    while (walk->current->descendant != NULL)
    {
        walk->current = walk->current->descendant;
    }
    free_node(walk->current);
    free(walk);
}

int commit_graph_fetch_ancestors_by_file(commit_graph_node_t *node, const char *filename)
{
    if (node->ancestors_fetched)
    {
        return 1;
    }
    if (node->ancestor_count != 0 || node->ancestors != NULL)
    {
        return -1;
    }


    // 'ancestor_count' = 0
    // make list for commits to check 'search_list'
    // make empty list of visited commits 'visited'
    // get parents of 'descendant', add them to 'search_list', add them to 'visited'

    // loop (end if 'search_list' is empty)
        // chcek if top commit in 'search_list' has file named like 'file'
        // if not remove it from 'search_list' and continue
        // else if that file is different add it to 'ancestors', remove it from 'search_list', increment 'ancestor_count' and continue
        // else go through top commit in 'search_stack' parents
            // if given parent was visited go to next
            // else add it to 'search_stack'
            // remove commit from 'search_stack'


    size_t parent_count = git_commit_parentcount(node->commit);
    node->parents = malloc(parent_count * sizeof(commit_graph_node_t *));
    if (!node->parents)
    {
        perror("Walking commit graph failure - memory allocation failed for parents while fetching them");
        exit(EXIT_FAILURE);
    }

    node->parent_count = parent_count;
    for (size_t i = 0; i < parent_count; i++)
    {
        git_commit *parent_commit = NULL;
        int error = git_commit_parent(&parent_commit, node->commit, i);
        if (error < 0)
        {
            const git_error *e = git_error_last();
            printf("Error %d/%d: %s\n", error, e->klass, e->message);
            git_libgit2_shutdown();
            exit(error);
        }

        commit_graph_node_t *parent_node = malloc(sizeof(commit_graph_node_t));
        parent_node->commit = parent_commit;
        parent_node->child = node;
        parent_node->parents = NULL;
        parent_node->parent_count = 0;
        parent_node->parents_fetched = 0;

        node->parents[i] = parent_node;
    }
    node->parents_fetched = 1;
}

int commit_graph_walk_to_ancestor(commit_graph_walk_t *walk, int ancestor_index)
{
    if (!walk || !walk->current || ancestor_index < 0)
    {
        return 1;
    }
    if (ancestor_index >= walk->current->ancestor_count)
    {
        return 2;
    }
    walk->current = walk->current->ancestors[ancestor_index];
    fetch_parents(walk->current);
    return 0;
}

int commit_graph_walk_to_descendant(commit_graph_walk_t *walk)
{
    if (!walk || !walk->current || !walk->current->descendant)
    {
        return 1;
    }
    walk->current = walk->current->descendant;
    return 0;
}
