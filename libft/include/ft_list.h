#ifndef __FT_LIST_H__
#define __FT_LIST_H__

#ifdef __cplusplus
extern "C" {
#endif


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
#ifndef container_of
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif



#define ft_list_entry(ptr, type, member) \
	container_of(ptr, type, member)
	

#define ft_list_first_entry(ptr, type, member) \
	ft_list_entry((ptr)->next, type, member)


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



static inline void ft_list_init(struct ft_list *head)
{
	if (head) {
		head->prev = head;
		head->next = head;
	}
}


static inline void ft_list_insert(struct ft_list *node,
			                      struct ft_list *prev,
			                      struct ft_list *next)
{
    if (NULL == node || NULL == prev || NULL == next) {
        return;
    }

	next->prev = node;
	node->next = next;
	node->prev = prev;
	prev->next = node;
}


static inline void ft_list_add(struct ft_list *node, struct ft_list *head)
{
    ft_list_insert(node, head, head->next);
}

static inline void ft_list_add_tail(struct ft_list *node, struct ft_list *head)
{
    ft_list_insert(node, head->prev, head);
}

static inline void ft_list_add_after(struct ft_list *node, struct ft_list *prev)
{
    ft_list_insert(node, prev, prev->next);
}

static inline void ft_list_add_before(struct ft_list *node, struct ft_list *next)
{
    ft_list_insert(node, next->prev, next);
}


static inline void ft_list_del(struct ft_list *node)
{
	if (NULL == node)
		return;
	
    if (node->prev)
        node->prev->next = node->next;

    if (node->next)
        node->next->prev = node->prev;

    node->prev = node;
    node->next = node;
}


static inline void ft_list_move(struct ft_list *node, struct ft_list *head)
{
    if (NULL == node || NULL == head)
        return;

    ft_list_del(node);

    ft_list_add(node, head);
}


static inline void ft_list_move_tail(struct ft_list *node, struct ft_list *head)
{
    if (NULL == node || NULL == head)
        return;

    ft_list_del(node);

    ft_list_add_tail(node, head);
}


static inline int ft_list_is_last(const struct ft_list *node, const struct ft_list *head)
{
	return (NULL == node || node->next == head);
}


static inline int ft_list_empty(const struct ft_list *head)
{
	return (NULL == head || head->next == head);
}



#ifdef __cplusplus
}
#endif

#endif /* __FT_LIST_H__ */

