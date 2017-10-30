/*
 * HandyC - Memory Management
 * 
 * Description:
 *     Implements a map data structure to keep track of malloc calls and frees.
 *     This allows commands to exist which free all malloc'd space at will, or
 *     tell the user if there are other malloc calls which need to be freed.
 *
 *     As of now, the pointer to allocated memory is stored in a CN_List, but
 *     when CN_Maps are done being developed, it will then be stored in that as
 *     its lookup time is O(log n).
 *
 * Author:
 *     Clara Van Nguyen
 */

#include "memory.h"

//Global Variable for storing malloc calls
CN_LIST __malloc_list = NULL; // = cn_list_init(void**);

//Malloc Functions
void* lmalloc (size_t _sz) {
	__initialise_malloc_list();

	void* ptr = malloc(_sz);
	cn_list_push_back(__malloc_list, &ptr);
	return ptr;
}

void* lcalloc (size_t _num, size_t _sz) {
	__initialise_malloc_list();

	void* ptr = calloc(_num, _sz);
	cn_list_push_back(__malloc_list, &ptr);
	return ptr;
}

void* lrealloc(void* ptr, size_t _sz) {
	__initialise_malloc_list();

	//Find entry
	CNL_ITERATOR ii;
	cn_list_traverse(__malloc_list, ii) {
		if (*(void**) ii->data == ptr)
			break;
	}
	ptr = realloc(ptr, _sz);
	memcpy(ii->data, &ptr, sizeof(void*));
	return ptr;
}

//Free Functions
void lfree(void* ptr) {
	if (__malloc_list == NULL)
		return;

	//Find entry
	CNL_ITERATOR ii;
	cn_uint pos = 0;
	cn_list_traverse(__malloc_list, ii) {
		if (*(void**) ii->data == ptr)
			break;
		pos++;
	}
	free(*(void**) ii->data);
	cn_list_delete(__malloc_list, pos);
}

void lfreeall() {
	CNL_NODE* node;
	while (__malloc_list->head != NULL) {
		node = cn_list_begin(__malloc_list);
		free(*(void**) node->data);
		cn_list_pop_front(__malloc_list);
	}
	cn_list_free(__malloc_list);
}

//Malloc Information
cn_uint num_of_malloc() {
	return cn_list_size(__malloc_list);
}

//"free()" Shortcuts
void free_cstr_array(char** data) {
	CN_UINT i = 0;
	for (; data[i] != NULL; i++)
		free(data[i]);
	free(data);
}

//Functions you won't use if you are sane
void __initialise_malloc_list() {
	if (__malloc_list == NULL)
		__malloc_list = cn_list_init(void**);
}
