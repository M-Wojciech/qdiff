#include <git2.h>
#include <stdio.h>

typedef struct commit_stack_node
{
  git_commit *commit;
  struct commit_stack_node *next;
} commit_stack_node_t;

typedef struct commit_stack
{
  commit_stack_node_t *top;
} commit_stack_t;

void push_commit_to_stack(commit_stack_t *stack, git_commit *commit)
{
  commit_stack_node_t *new_node = (commit_stack_node_t *)malloc(sizeof(commit_stack_node_t));
  if (!new_node)
  {
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }
  new_node->commit = commit;
  new_node->next = stack->top;
  stack->top = new_node;
}

git_commit *pop_commit_from_stack(commit_stack_t *stack)
{
  if (!stack->top)
  {
    return NULL;
  }
  commit_stack_node_t *popped = stack->top;
  git_commit *out = popped->commit;
  stack->top = popped->next;
  free(popped);
  return out;
}
