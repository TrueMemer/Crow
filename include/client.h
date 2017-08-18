#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "../deps/libwsclient/wsclient.h"
#include "types.h"

typedef struct Client {
	wsclient *ws;
    char *session_id;
	int done;
	int hello;
	int hrtb_interval;
	int hrtb_acks;
	int seq;
	user_t self;
	void (*on_message)(struct Client *client, message_t msg);
	void (*on_ready)(struct Client *client);
} client_t;

int client_init(client_t *client);
void client_run(client_t *client);
void client_finish(client_t *client);
void heartbeat(client_t *client);

#endif // _CLIENT_H_