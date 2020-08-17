#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include "list.h"

#define handle_error(msg)                                                      \
	do {                                                                   \
		perror(msg);                                                   \
		exit(EXIT_FAILURE);                                            \
	} while (0)

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct client *last = NULL;
struct client *clients = NULL;

struct client *append_node(int conn_id)
{
	struct client *node = (struct client *)malloc(sizeof(struct client));
	if (node == NULL)
		handle_error("malloc");

	node->conn_id = conn_id;
	node->next = NULL;

	int en;

	if ((en = pthread_mutex_lock(&mutex)) != 0) {
		errno = en;
		handle_error("pthread_mutex_lock");
	}

	node->prev = last;
	/* Append to client list in O(1) time by keeping
	 * reference to last node
	 */
	if (last)
		last->next = node;
	else
		clients = node;

	last = node;

	if ((en = pthread_mutex_unlock(&mutex)) != 0) {
		errno = en;
		handle_error("pthread_mutex_unlock");
	}

	return node;
}

void remove_node(struct client *node)
{
	int en;
	if ((en = pthread_mutex_lock(&mutex)) != 0) {
		errno = en;
		handle_error("pthread_mutex_unlock");
	}

	/* Remove from list in O(1) time by passing
	 * reference to the node that should be removed
	 */
	if (node->next)
		node->next->prev = node->prev;
	else
		last = node->prev;

	if (node->prev)
		node->prev->next = node->next;
	else
		clients = node->next;

	if ((en = pthread_mutex_unlock(&mutex)) != 0) {
		errno = en;
		handle_error("pthread_mutex_unlock");
	}

	free(node);
}

void remove_all()
{
	while (clients) {
		struct client *tmp = clients;
		clients = clients->next;
		free(tmp);
	}
}
