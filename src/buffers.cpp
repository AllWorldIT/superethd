#include "buffers.hpp"

extern "C" {
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
}

#include "common.hpp"

void initialize_buffer_list(BufferList* buffer_list, size_t count, size_t buffer_size) {
	// Initialize the list data
	buffer_list->size = 0;
	buffer_list->head = NULL;
	buffer_list->tail = NULL;

	// Next the pthread mutex
	if (pthread_mutex_init(&buffer_list->mutex, NULL) != 0) {
		// Handle error, e.g., print an error message
		perror("initialize_buffer_list(): Mutex initialization failed");
		exit(EXIT_FAILURE);
	}
	// Initialize the pthread condition
	if (pthread_cond_init(&buffer_list->cond_new, NULL) != 0) {
		// Handle error, e.g., print an error message
		perror("initialize_buffer_list(): Condition initialization failed");
		exit(EXIT_FAILURE);
	}

	// Now inititalize the nodes
	for (size_t i = 0; i < count; ++i) {
		BufferNode* node = (BufferNode*)calloc(1, sizeof(BufferNode));
		if (node == NULL) {
			perror("initialize_buffer_list(): Error allocating memory for buffers");
			exit(EXIT_FAILURE);
		}

		// Allocate buffer and its contents
		node->buffer = (Buffer*)calloc(1, sizeof(Buffer));
		node->buffer->contents = (char*)malloc(buffer_size);

		if (buffer_list->tail == NULL) {
			buffer_list->head = node;
			buffer_list->tail = node;
		} else {
			buffer_list->tail->next = node;
			buffer_list->tail = node;
		}
		buffer_list->size++;
	}
}

// Return buffer list size
ssize_t get_buffer_list_size(BufferList* buffer_list) {
	pthread_mutex_lock(&buffer_list->mutex);
	ssize_t size = buffer_list->size;
	pthread_mutex_unlock(&buffer_list->mutex);
	return size;
}

// Internal function with no locking to grab the buffer node head
BufferNode* _get_buffer_node_head(BufferList* buffer_list) {
	// If the buffer list is empty, just return NULL
	if (buffer_list->head == NULL) {
		return NULL;
	}

	BufferNode* node = buffer_list->head;
	buffer_list->head = node->next;
	if (buffer_list->head == NULL) {
		buffer_list->tail = NULL;
	}
	buffer_list->size--;

	// Make sure we didn't get to a nevative size
	if (buffer_list->size < 0) {
		fprintf(stderr, "Buffer list size is negative\n");
		exit(EXIT_FAILURE);
	}

	// Clear node next
	node->next = NULL;

	return node;
}

// Return the buffer node head
BufferNode* get_buffer_node_head(BufferList* buffer_list) {
	// Lock, get and unlock
	pthread_mutex_lock(&buffer_list->mutex);
	BufferNode* node = _get_buffer_node_head(buffer_list);
	pthread_mutex_unlock(&buffer_list->mutex);

	return node;
}

// Grab the buffer node head, but wait until one is available
int get_buffer_node_head_wait(BufferList* buffer_list, BufferNode** buffer_node, uint16_t tv_sec, uint16_t tv_nsec) {
	pthread_mutex_lock(&buffer_list->mutex);

	// NK: Quick short circuit, check first if we actually have a node we can hand out
	*buffer_node = _get_buffer_node_head(buffer_list);
	if (*buffer_node) {
		pthread_mutex_unlock(&buffer_list->mutex);
		return 0;
	}

	// Check if we're doing a timed wait or just an indefinite wait
	int result;
	if (!tv_sec && !tv_nsec) {
		result = pthread_cond_wait(&buffer_list->cond_new, &buffer_list->mutex);

	} else {
		// Wait for the signal or timeout
		struct timespec timeout;
		clock_gettime(CLOCK_REALTIME, &timeout);
		timeout.tv_sec += tv_sec;
		timeout.tv_nsec += tv_nsec;

		result = pthread_cond_timedwait(&buffer_list->cond_new, &buffer_list->mutex, &timeout);
	}

	// Set pointer to new buffer node
	*buffer_node = _get_buffer_node_head(buffer_list);

	pthread_mutex_unlock(&buffer_list->mutex);

	return result;
}

void append_buffer_node(BufferList* buffer_list, BufferNode* node) {
	// We can safely modify the node outside of the mutex
	node->next = NULL;

	pthread_mutex_lock(&buffer_list->mutex);

	if (buffer_list->tail == NULL) {
		buffer_list->head = node;
		buffer_list->tail = node;
	} else {
		buffer_list->tail->next = node;
		buffer_list->tail = node;
	}

	buffer_list->size++;

	// Trigger any listeners
	pthread_cond_signal(&buffer_list->cond_new);

	pthread_mutex_unlock(&buffer_list->mutex);
}

void free_buffers(BufferList* buffer_list) {
	BufferNode* node;

	while (buffer_list->head != NULL) {
		// Remove node
		node = buffer_list->head;
		buffer_list->head = node->next;

		// Free entire node
		free(node->buffer->contents);
		free(node->buffer);
		free(node);
	}

	pthread_cond_destroy(&buffer_list->cond_new);
	pthread_mutex_destroy(&buffer_list->mutex);
}
