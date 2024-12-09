#ifndef COMMIT_GRAPH_NODE_H
#define COMMIT_GRAPH_NODE_H

#include <git2.h>

// Structure representing a single node in the commit graph
typedef struct commit_graph_node {
    git_commit *commit;                      // Pointer to the associated Git commit
    struct commit_graph_node *child;         // Pointer to the child node (commit from which this was found)
    struct commit_graph_node **parents;      // Pointer to an array of parent nodes
    size_t parent_count;                     // Number of parents (size of the parents array)
    int parents_fetched;                     // Flag indicating if parents have been fetched (0 = not fetched, 1 = fetched)
} commit_graph_node_t;

#endif