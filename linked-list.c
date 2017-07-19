#include <stdio.h>
#include <stdlib.h>

typedef struct _ll_node {
	void*			 item;
	struct _ll_node* next;
} ll_node;

ll_node* ll_node_new(void* item, ll_node* next) {
	ll_node* n = malloc(sizeof(ll_node));
	n->item = item;
	n->next = next;
	return n;
}

typedef struct {
	size_t   len;
	ll_node* first;
	ll_node* last;
} linked_list;

linked_list* ll_new() {
	linked_list* self = malloc(sizeof(linked_list));
	self->len = 0;
	self->first = NULL;
	self->last = NULL;
	return self;
}

void ll_add(linked_list* self, void* item) {
	ll_node* n = ll_node_new(item, NULL);
	if (self->first == NULL)
		self->last = self->first = n;
	else
		self->last = self->last->next = n;
	self->len++;
}

#define ll_for(type, name, list, body) { \
	ll_node* __node = list->first; \
	type* name = (type*) (__node->item); \
	for ( \
			int __i = 0; \
			__i < list->len; \
			__i++, \
			__node = __node->next, \
			name = __node == NULL ? NULL : (type*) (__node->item) \
		) \
		body; \
}

#define ll_for_no_item(type, list, body) { \
	ll_node* __node = list->first; \
	for ( \
			int __i = 0; \
			__i < list->len; \
			__i++, \
			__node = __node->next \
		) \
		body; \
}

void ll_free(linked_list* self) {
	ll_for_no_item(void*, self, {
		free(__node);
	});
	free(self);
}

void ll_free_with_items(linked_list* self) {
	ll_for(void*, i, self, {
		free(i);
		free(__node);
	});
	free(self);
}

#define def_ptr_fun(type) type* type ## _ptr_new(type i) { \
	type * p = malloc(sizeof(type)); \
	*p = i; \
	return p; \
}

def_ptr_fun(int)
def_ptr_fun(short)
def_ptr_fun(long)
def_ptr_fun(float)
def_ptr_fun(double)
def_ptr_fun(char)

// new pointer
#define np(type, val) type ## _ptr_new(val)

/*
int main(int argc, char** argv) {
	
	linked_list* l = ll_new();
	ll_add(l, np(int, 1));
	ll_add(l, np(int, 2));
	ll_add(l, np(int, 3));
 	
	ll_for(int, i, l, {
		printf("%d\n", *i);
	});
	
	linked_list* l2 = ll_new();
	ll_add(l2, np(double, 1.1));
	ll_add(l2, np(double, 2.2));
	ll_add(l2, np(double, 3.3));
 	
	ll_for(double, i, l2, {
		printf("%f\n", *i);
	});
	
	return 0;
}
*/





