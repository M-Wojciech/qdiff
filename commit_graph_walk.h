#ifndef COMMIT_GRAPH_WALK_H
#define COMMIT_GRAPH_WALK_H

#include <git2.h>

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
    git_commit *commit;                   // Pointer to the associated Git commit
    git_tree_entry *entry;                // entry of the file we create graph for.
    struct commit_graph_node *descendant; // Pointer to the descendant node (commit from which this was found)
    struct commit_graph_node **ancestors; // Pointer to an array of ancestor nodes
    size_t ancestor_count;                // Number of ancestors (size of the parents array)
    int ancestors_fetched;                // Flag indicating if ancestors have been fetched (0 = not fetched, 1 = fetched)
} commit_graph_node_t;

typedef struct
{
    commit_graph_node_t *current;
} commit_graph_walk_t;

// helper for searching through graph
typedef struct
{
    git_oid **list;  // Array of pointers to git_oid
    size_t size;     // Current number of elements
    size_t capacity; // Current capacity of the list
} visited_set_t;

commit_graph_node_t *commit_graph_node_init(void);
commit_graph_walk_t *commit_graph_walk_init(git_commit *start_commit, const char *_filename);
void commit_graph_walk_free(commit_graph_walk_t *walk);
int commit_graph_walk_to_ancestor(commit_graph_walk_t *walk, int ancestor_index);
int commit_graph_walk_to_descendant(commit_graph_walk_t *walk);
int commit_graph_fetch_ancestors(commit_graph_node_t *node);
void search_git_tree_for_changed_file(git_commit *commit, commit_graph_node_t *search_root, visited_set_t *visited, git_tree_entry *file);
int search_git_tree_for_oldest_with_entry(git_commit *commit, commit_graph_node_t *for_result, visited_set_t *visited, const git_tree_entry *entry);
void add_ancestor(commit_graph_node_t *node, git_commit *commit, const git_tree_entry *entry);

visited_set_t *visited_set_init(void);
void visited_set_add(visited_set_t *visited, const git_oid *oid);
int visited_set_contains(visited_set_t *visited, const git_oid *oid);
void visited_set_free(visited_set_t *visited);

#endif
