#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

typedef struct ListNode {
  struct ListNode *next;
  void *data;
} ListNode_t;

typedef ListNode_t *LinkedList_t;

int ll_init(LinkedList_t *head);
int ll_push(LinkedList_t *head, void *item, size_t item_sz);
int ll_pop(LinkedList_t *head, void *item_out, size_t item_sz);
LinkedList_t ll_sublist(LinkedList_t *head, size_t idx);
int ll_get(LinkedList_t *head, size_t idx, void *item_out, size_t item_sz);
int ll_insert(LinkedList_t *head, size_t idx, void *item, size_t item_sz);
void *ll_remove(LinkedList_t *head, size_t idx);
void ll_deinit(LinkedList_t *head);

#ifdef LL_IMPL
int ll_init(LinkedList_t *head) {
  *head = NULL;
  return 1;
}

int ll_push(LinkedList_t *head, void *item, size_t item_sz) {
  ListNode_t *new_node = (ListNode_t *)calloc(1, sizeof(ListNode_t));

  if (!new_node)
    return 0;

  new_node->data = calloc(1, item_sz);
  if (!new_node->data) {
    free(new_node);
    return 0;
  }

  new_node->next = *head;
  memcpy(new_node->data, item, item_sz);
  *head = new_node;
  return 1;
}

int ll_pop(LinkedList_t *head, void *item_out, size_t item_sz) {
  if (!*head)
    return 0;

  memcpy(item_out, (*head)->data, item_sz);
  ListNode_t *new_head = (*head)->next;
  free((*head)->data);
  free(*head);
  *head = new_head;

  return 1;
}

LinkedList_t ll_sublist(LinkedList_t *head, size_t idx) {
  ListNode_t *curr = *head;
  for (size_t i = 0; i < idx && curr; i++, curr = curr->next)
    ;

  return curr;
}

int ll_get(LinkedList_t *head, size_t idx, void *item_out, size_t item_sz) {
  ListNode_t *curr = ll_sublist(head, idx);
  if (!curr)
    return 0;

  memcpy(item_out, curr->data, item_sz);
  return 1;
}

int ll_insert(LinkedList_t *head, size_t idx, void *item, size_t item_sz) {
  if (idx == 0)
    return ll_push(head, item, item_sz);

  LinkedList_t sublist = ll_sublist(head, idx - 1);
  if (!sublist)
    return 0;

  LinkedList_t new_node = sublist->next;

  if (!ll_push(&new_node, item, item_sz))
    return 0;

  sublist->next = new_node;

  return 1;
}

void *ll_remove(LinkedList_t *head, size_t idx) {
  if (idx == 0) {
    ListNode_t old_head = **head;
    free(*head);
    *head = old_head.next;

    return old_head.data;
  }

  LinkedList_t sublist = ll_sublist(head, idx - 1);
  if (!sublist)
    return NULL;

  ListNode_t *old_head = sublist->next;

  if (old_head) {
    sublist->next = old_head->next;
  } else {
    sublist->next = NULL;
  }

  void *data = old_head->data;
  free(old_head);
  return data;
}

void ll_deinit(LinkedList_t *head) {
  ListNode_t *curr = *head;

  while (curr) {
    ListNode_t *next = curr->next;
    free(curr->data);
    free(curr);
    curr = next;
  }

  *head = NULL;
}

#endif //  LL_IMPL

#endif //  LINKED_LIST_H
