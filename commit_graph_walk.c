#include "commit_graph_walk.h"


commit_graph_walk_t *commit_graph_walk_init(git_commit *start_commit) {
    commit_graph_walk_t *walk = malloc(sizeof(commit_graph_walk_t));
    if (!walk) return NULL;

    commit_graph_node_t *root = malloc(sizeof(commit_graph_node_t));
    if (!root) {
        free(walk);
        return NULL;
    }

    root->commit = start_commit;
    root->child = NULL;
    root->parents = NULL;
    root->parent_count = 0;
    root->parents_fetched = 0;

    walk->current = root;
    return walk;
}

#include <stdlib.h>
#include <git2.h>
#include "commit_graph_node.h"

// Recursive free function for graph nodes
void free_node(commit_graph_node_t *node) {
    if (!node) return;
    // Recursively free parent nodes
    for (size_t i = 0; i < node->parent_count; ++i) {
        free_node(node->parents[i]);
    }
    // Free the array of parent pointers
    free(node->parents);
    // Free the Git commit object
    git_commit_free(node->commit);
    // Free the node itself
    free(node);
}

// Free the commit graph walk
void commit_graph_walk_free(commit_graph_walk_t *walk) {
    if (!walk) return;
    // Free the current node recursively
    free_node(walk->current);
    // Free the walk structure
    free(walk);
}

void fetch_parents(commit_graph_node_t *node, git_repository *repo) {
    if (node->parents_fetched) return;

    size_t parent_count = git_commit_parentcount(node->commit);
    node->parents = malloc(parent_count * sizeof(commit_graph_node_t *));
    if (!node->parents) return; // Handle allocation failure

    node->parent_count = parent_count;
    for (size_t i = 0; i < parent_count; ++i) {
        git_commit *parent_commit = NULL;
        int error = git_commit_parent(&parent_commit, node->commit, i);
        if (error < 0) {
            // Handle error appropriately
            continue;
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

commit_graph_node_t *commit_graph_walk_to_parent(commit_graph_walk_t *walk, int parent_index) {
    if (!walk || !walk->current || parent_index < 0 || parent_index >= walk->current->parent_count) {
        return NULL;
    }

    fetch_parents(walk->current, NULL); // Assuming the repository is globally available
    walk->current = walk->current->parents[parent_index];
    return walk->current;
}


commit_graph_node_t *commit_graph_walk_to_child(commit_graph_walk_t *walk) {
    if (!walk || !walk->current || !walk->current->child) {
        return NULL;
    }

    walk->current = walk->current->child;
    return walk->current;
}
