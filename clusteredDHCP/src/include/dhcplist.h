
#ifndef __DHCPLIST_H__
#define __DHCPLIST_H__


#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

struct _list_head {
    struct _list_head *next, *prev;
};

typedef struct _list_head  list_head;

#define INIT_LIST_HEAD(ptr) { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
}

static inline void __list_add(list_head *new_head,
                   list_head *prev,
                   list_head *next)
{
    next->prev = new_head;
    new_head->next = next;
    new_head->prev = prev;
    prev->next = new_head;
}


static inline void list_add_tail( list_head *new_head,  list_head *head)
{
    __list_add(new_head, head->prev, head);
}


static inline void __list_del( list_head * prev,  list_head * next)
{
    next->prev = prev;
    prev->next = next;
}


static inline void list_del( list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static inline int list_empty( list_head *head)
{
    return head->next == head;
}


#define list_entry(ptr, type, member) \
    ((type *)((void *)((char *)(ptr) - offsetof(type, member))))

#define list_for_each_safe(pos, n, head) for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

#endif

