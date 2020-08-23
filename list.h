#pragma once

struct client {
	int conn_id;
	struct client *next;
	struct client *prev;
};

struct client *append_node(int conn_id);
void remove_node(struct client *node);
void remove_all();
