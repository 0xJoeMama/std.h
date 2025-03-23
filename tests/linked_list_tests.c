#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define LL_IMPL
#include "../include/linked_list.h"

static void print_list(LinkedList_t l) {
  ListNode_t *curr = l;
  while (curr) {
    printf(" %d ->", *(int *)curr->data);
    curr = curr->next;
  }
  printf(". \n");
}

int main(void) {
  LinkedList_t l;
  ll_init(&l);

  int i = 0;
  assert(ll_push(&l, &i, sizeof(int)));

  int contents[] = {0};

  ListNode_t *curr = l;
  for (size_t i = 0; i < sizeof(contents) / sizeof(int); i++, curr = curr->next)
    assert(*(int *)curr->data == contents[i]);

  int i_out;
  assert(ll_pop(&l, &i_out, sizeof(int)));
  assert(i_out == i);
  assert(!ll_pop(&l, &i_out, 1));

  ll_deinit(&l);

  ll_init(&l);
  for (int i = 0; i < 100; i++) {
    if (i == 3)
      continue;

    ll_push(&l, &i, sizeof(int));
  }

  print_list(l);

  i = 3;
  assert(ll_insert(&l, 96, &i, sizeof(int)));
  print_list(l);

  int *data = ll_remove(&l, 96);
  assert(data);
  printf("%d\n", *data);
  free(data);
  print_list(l);

  ll_deinit(&l);

  return 0;
}
