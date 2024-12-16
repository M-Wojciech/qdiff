#include <stdlib.h>
#include <git2.h>
#include <stdio.h>
#include <unistd.h>
#include "commit_graph_walk.h"

commit_graph_node_t *commit_graph_node_init(void)
{
    commit_graph_node_t *node = malloc(sizeof(commit_graph_node_t));
    if (!node)
    {
        return NULL;
    }
    node->commit = NULL;
    node->entry = NULL;
    node->descendant = NULL;
    node->ancestors = NULL;
    node->ancestor_count = 0;
    node->ancestors_fetched = 0;
    return node;
}

commit_graph_walk_t *commit_graph_walk_init(git_commit *start_commit, const char *_filename)
{
    if (!_filename)
    {
        return NULL;
    }
    commit_graph_walk_t *walk = malloc(sizeof(commit_graph_walk_t));
    if (!walk)
    {
        return NULL;
    }
    commit_graph_node_t *root = commit_graph_node_init();
    if (!root)
    {
        free(walk);
        return NULL;
    }

    git_tree *commit_tree = NULL;
    git_commit_tree(&commit_tree, start_commit);
    const git_tree_entry *tmp_entry = git_tree_entry_byname(commit_tree, _filename);
    git_tree_entry_dup(&(root->entry), tmp_entry);
    git_tree_free(commit_tree);
    git_commit_dup(&(root->commit), start_commit);
    commit_graph_fetch_ancestors(root);
    walk->current = root;
    return walk;
}

// Recursive free function for freeing graph nodes and all its children
void commit_graph_free_node(commit_graph_node_t *node)
{
    if (!node)
    {
        return;
    }
    for (size_t i = 0; i < node->ancestor_count; i++)
    {
        commit_graph_free_node(node->ancestors[i]);
    }
    free(node->ancestors);
    git_commit_free(node->commit);
    git_tree_entry_free(node->entry);
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
    commit_graph_free_node(walk->current);
    free(walk);
}

int commit_graph_fetch_ancestors(commit_graph_node_t *node)
{
    if (node->ancestors_fetched)
    {
        return 1;
    }
    if (node->ancestor_count != 0 || node->ancestors != NULL)
    {
        return -1;
    }
    // Initialize visited set
    visited_set_t *visited = visited_set_init();

    // check all parents (node children) with recursive function
    size_t parent_count = git_commit_parentcount(node->commit);
    git_commit *parent = NULL;
    const char *entry_name = git_tree_entry_name(node->entry);
    for (size_t i = 0; i < parent_count; i++)
    {
        git_commit_parent(&parent, node->commit, i); // freed in recursive function if wont be used
        search_commits_recursive(parent, node, visited, entry_name);
    }
    // Mark as fetched and clean up
    node->ancestors_fetched = 1;
    visited_set_free(visited);
    return 0;
}

void search_commits_recursive(git_commit *commit, commit_graph_node_t *search_root, visited_set_t *visited, const char *filename)
{
    if (!commit || visited_set_contains(visited, git_commit_id(commit)))
    {
        return; // Stop if node is null or already visited
    }

    // Add current commit to visited
    visited_set_add(visited, git_commit_id(commit));

    // Check if the file was modified in this commit
    git_tree *tree;
    git_commit_tree(&tree, commit);
    const git_tree_entry *entry = git_tree_entry_byname(tree, filename);
    if (!entry)
    {
        // // check for rename
        // if (entry) // renamed in commit
        // {
        //     search_root->ancestor_count++;
        //     search_root->ancestors = realloc(search_root->ancestors, search_root->ancestor_count * sizeof(commit_graph_node_t *));
        //     search_root->ancestors[search_root->ancestor_count - 1] = commit_graph_node_init();
        //     search_root->ancestors[search_root->ancestor_count - 1]->commit = commit;
        //     git_tree_entry *tmp_entry;
        //     git_tree_entry_dup(&tmp_entry, entry);
        //     search_root->ancestors[search_root->ancestor_count - 1]->entry = tmp_entry;
        //     search_root->ancestors[search_root->ancestor_count - 1]->descendant = search_root;
        //     git_tree_free(tree);
        //     return;
        // }
        // File doesn't exist in this commit, stop recursion
        git_tree_free(tree);
        git_commit_free(commit);
        return;
    }

    // If file was modified, add to ancestors and stop recursion
    if (!git_tree_entry_cmp(search_root->entry, entry))
    {
        search_root->ancestor_count++;
        search_root->ancestors = realloc(search_root->ancestors, search_root->ancestor_count * sizeof(commit_graph_node_t *));
        search_root->ancestors[search_root->ancestor_count - 1] = commit_graph_node_init();
        search_root->ancestors[search_root->ancestor_count - 1]->commit = commit;
        git_tree_entry *tmp_entry;
        git_tree_entry_dup(&tmp_entry, entry);
        search_root->ancestors[search_root->ancestor_count - 1]->entry = tmp_entry;
        search_root->ancestors[search_root->ancestor_count - 1]->descendant = search_root;
        git_tree_free(tree);
        return;
    }
    git_tree_free(tree);

    // Recurse into parents
    size_t parent_count = git_commit_parentcount(commit);
    git_commit *parent_commit;
    for (size_t i = 0; i < parent_count; i++)
    {
        git_commit_parent(&parent_commit, commit, i);
        search_commits_recursive(parent_commit, search_root, visited, filename);
    }
    git_commit_free(commit);
    return;
}

void add_ancestor(commit_graph_node_t *search_root, git_commit *commit, const git_tree_entry *entry)
{
    search_root->ancestor_count++;
    search_root->ancestors = realloc(search_root->ancestors, search_root->ancestor_count * sizeof(commit_graph_node_t *));
    commit_graph_node_t *new_node = commit_graph_node_init();
    search_root->ancestors[search_root->ancestor_count - 1] = new_node;
    new_node->commit = commit;
    git_tree_entry_dup(&(new_node->entry), entry);
    new_node->descendant = search_root;
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
    commit_graph_fetch_ancestors(walk->current);
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

// Initialize a visited set
visited_set_t *visited_set_init(void)
{
    visited_set_t *visited = malloc(sizeof(visited_set_t));
    if (!visited)
    {
        perror("Failed to allocate memory for visited set");
        exit(EXIT_FAILURE);
    }
    visited->list = malloc(16 * sizeof(git_oid *));
    if (!visited->list)
    {
        perror("Failed to allocate memory for visited set list");
        exit(EXIT_FAILURE);
    }
    visited->size = 0;
    visited->capacity = 16;
    return visited;
}

// Add an OID to the visited set
void visited_set_add(visited_set_t *visited, const git_oid *oid)
{
    if (visited->size == visited->capacity)
    {
        visited->capacity *= 2;
        visited->list = realloc(visited->list, visited->capacity * sizeof(git_oid *));
        if (!visited->list)
        {
            perror("Failed to reallocate memory for visited set list");
            exit(EXIT_FAILURE);
        }
    }
    visited->list[visited->size] = malloc(sizeof(git_oid));
    if (!visited->list[visited->size])
    {
        perror("Failed to allocate memory for git_oid");
        exit(EXIT_FAILURE);
    }
    git_oid_cpy(visited->list[visited->size++], oid);
}

// Check if an OID is in the visited set
int visited_set_contains(visited_set_t *visited, const git_oid *oid)
{
    for (size_t i = 0; i < visited->size; i++)
    {
        if (git_oid_equal(visited->list[i], oid))
        {
            return 1;
        }
    }
    return 0;
}

// Free the visited set
void visited_set_free(visited_set_t *visited)
{
    for (size_t i = 0; i < visited->size; i++)
    {
        free(visited->list[i]);
    }
    free(visited->list);
    free(visited);
}
