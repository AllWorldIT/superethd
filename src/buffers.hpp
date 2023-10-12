#pragma once


extern "C" {
#include <stdint.h>
#include <sys/types.h>
}

#include "common.hpp"

typedef struct {
	char* contents;
	ssize_t length;
} Buffer;

typedef struct BufferNode {
	Buffer* buffer;
	struct BufferNode* next;
} BufferNode;

typedef struct {
	BufferNode* head;
	BufferNode* tail;
	size_t size;
	// Mutex to protect buffer list
	pthread_mutex_t mutex;
	// Condition triggered when a new node is added
	pthread_cond_t cond_new;

} BufferList;

void initialize_buffer_list(BufferList* buffer_list, size_t count, size_t buffer_size);

ssize_t get_buffer_list_size(BufferList* buffer_list);

BufferNode* get_buffer_node_head(BufferList* buffer_list);

int get_buffer_node_head_wait(BufferList* buffer_list, BufferNode** buffer_node, uint16_t tv_sec, uint16_t tv_nsec);

void append_buffer_node(BufferList* buffer_list, BufferNode* node);

void free_buffers(BufferList* buffer_list);
