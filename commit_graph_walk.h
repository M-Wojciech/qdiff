#ifndef COMMIT_GRAPH_WALK_H
#define COMMIT_GRAPH_WALK_H

/*
Naming might be confusing as walking to descendants
is walking to root here and walking to ancestors is
walking to leafs. Its like this since its taken
from how commit structure in git looks like and
we start by 'youngest' as root and find its ancestors.
*/

// Structure representing a single node in the commit graph
typedef struct commit_graph_node
{
    git_commit *commit;                     // Pointer to the associated Git commit
    const git_tree_entry *entry;                  // entry of tha file we create graph for.
    struct commit_graph_node *descendant;   // Pointer to the descendant node (commit from which this was found)
    struct commit_graph_node **ancestors;   // Pointer to an array of ancestor nodes
    size_t ancestor_count;                  // Number of ancestors (size of the parents array)
    int ancestors_fetched;                  // Flag indicating if ancestors have been fetched (0 = not fetched, 1 = fetched)
} commit_graph_node_t;

typedef struct
{
    commit_graph_node_t *current;
} commit_graph_walk_t;

// helper for searching through graph
typedef struct
{
    const git_oid **list;       // Array of pointers to git_oid
    size_t size;          // Current number of elements
    size_t capacity;      // Current capacity of the list
} visited_set_t;

commit_graph_walk_t *commit_graph_walk_init(git_commit *start_commit, const char *filename);
void commit_graph_walk_free(commit_graph_walk_t *walk);
int commit_graph_walk_to_ancestor(commit_graph_walk_t *walk, int ancestor_index);
int commit_graph_walk_to_descendant(commit_graph_walk_t *walk);
int commit_graph_fetch_ancestors(commit_graph_node_t *node);

#endif
