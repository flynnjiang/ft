#ifndef __FT_LIST_H__
#define __FT_LIST_H__


struct ft_list {
    struct ft_list *prev;
    struct ft_list *next;
};

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
#endif

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})


#define ft_list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define ft_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define ft_list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define ft_list_for_each_entry(pos, head, member)				\
	for (pos = ft_list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = ft_list_entry(pos->member.next, typeof(*pos), member))

#define ft_list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = ft_list_entry((head)->next, typeof(*pos), member),	\
		n = ft_list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = ft_list_entry(n->member.next, typeof(*n), member))



static inline void ft_list_init(struct ft_list *list)
{
    list->next = list;
    list->prev = list;
}

static inline void _ft_list_add(struct ft_list *new,
			      struct ft_list *prev,
			      struct ft_list *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}


static inline void ft_list_add(struct ft_list *new, struct ft_list *head)
{
    _ft_list_add(new, head, head->next);
}

static inline void ft_list_add_tail(struct ft_list *new, struct ft_list *head)
{
	_ft_list_add(new, head->prev, head);
}

static inline void _ft_list_del(struct ft_list *prev, struct ft_list *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void ft_list_del(struct ft_list *entry)
{
	_ft_list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;    
}
static inline int ft_list_empty(struct ft_list *list)
{
	return list->next == list;
}

#endif /* __FT_LIST_H__ */
