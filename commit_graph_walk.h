#ifndef COMMIT_GRAPH_WALK_H
#define COMMIT_GRAPH_WALK_H

#include "commit_graph_node.h"

typedef struct {
    commit_graph_node_t *current; // Pointer to the current position in the graph
} commit_graph_walk_t;

// Function declarations
commit_graph_walk_t *commit_graph_walk_init(git_commit *start_commit);
void commit_graph_walk_free(commit_graph_walk_t *walk);
commit_graph_node_t *commit_graph_walk_to_parent(commit_graph_walk_t *walk, int parent_index);
commit_graph_node_t *commit_graph_walk_to_child(commit_graph_walk_t *walk);
void fetch_parents(commit_graph_node_t *node, git_repository *repo);

#endif
