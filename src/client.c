#include "../include/client.h"
#include "../include/events.h"
#include "../include/log.h"

#include <json.h>

client_t *g_client;

void handshake(client_t *client) {
	char _token[1024];
    snprintf(_token, sizeof(_token), "{\"op\":2,\"d\":{\"token\":\"%s\",\"v\":4,\"encoding\":\"etf\",\"properties\":{\"$os\":\"linux\",\"browser\":\"crow\",\"device\":\"crow\",\"referrer\":\"\",\"referring_domain\":\"\"},\"compress\":false,\"large_threshold\":250,\"shard\":[0,1]}}", TOKEN);
    json_object *response = json_tokener_parse(_token);
    libwsclient_send(client->ws, json_object_to_json_string(response));
}

int onmessage(wsclient *c, wsclient_message *msg) {
    json_object *m = json_tokener_parse(msg->payload);

	json_object *_op;

	json_object_object_get_ex(m, "op", &_op);

	if (json_object_get_type(_op) == json_type_null) {
		log_warn("recieved a message with unknown op!");
		return;
	}

	int op = json_object_get_int(_op);

	switch (op) {
		case DISPATCH:
			dispatch(g_client, m);
			break;
		case HELLO:
			if (!g_client->hello) {
				g_client->hello = 1;
				json_object *hello_;
				json_object *d;
				json_object_object_get_ex(json_tokener_parse(msg->payload), "d", &d);
				json_object_object_get_ex(d, "heartbeat_interval", &hello_);
				g_client->hrtb_interval = json_object_get_int(hello_);

				json_object_put(hello_);
				json_object_put(d);
				handshake(g_client);
				break;
			}
		case HEARTBEAT_ACK:
			log_debug("heartbeat acks!");
			g_client->hrtb_acks = 1;
			break;
		
		case RECONNECT:
			log_warn("got RESUME event! reconnecting...");
			reconnect(g_client);
			break;
	}

    return;
}

void 
onclose(wsclient *c) {
    log_warn("ws closed: %d", c->sockfd);
    g_client->done = 1;
}

void 
onerror(wsclient *c, wsclient_error *err) {
    log_error("error %d has occuried: %s!\n", err->code, err->str);
    g_client->done = 1;
}

void 
onopen(wsclient *c) {
    log_info("onopen websocket function is called!");
	return;
}

void reconnect(client_t *client) {
	client->hello = 0;

	client_finish(client);
	
	client_init(client);

	client_run(client);

	json_object *resume = json_object_new_object();

	json_object_object_add(resume, "token", json_object_new_string(TOKEN));
	json_object_object_add(resume, "session_id", json_object_new_string(client->session_id));
	json_object_object_add(resume, "seq", json_object_new_int(client->seq));

	libwsclient_send(client->ws, json_object_to_json_string(resume));

	log_debug(json_object_to_json_string(resume));

	client->hello = 1;
}

void heartbeat(client_t *client) {
	while(!client->done) {
		if (client->hello) {
			if (!client->hrtb_acks) {
				log_debug("zombie! reconnecting...");
				reconnect(client);
				continue;
			}
			json_object *heartbeat;

			heartbeat = json_object_new_object();
			json_object_object_add(heartbeat, "op", json_object_new_int(1));
			json_object_object_add(heartbeat, "d", json_object_new_int(client->seq));

			libwsclient_send(client->ws, json_object_to_json_string(heartbeat));
			fflush(stdout);
			log_debug("sleeping for %d", client->hrtb_interval * 1000);
			//log_debug("hrtb: %s", json_object_to_json_string(heartbeat));
			json_object_put(heartbeat);
			client->hrtb_acks = 0;
			usleep(client->hrtb_interval * 1000);
		}
	}
}

void client_run(client_t *client) { libwsclient_run(client->ws); log_info("WS client thread is running..."); heartbeat(client); }
void client_finish(client_t *client) { libwsclient_finish(client->ws); client->done = 1; }

int client_init(client_t *client) {

	g_client = client;

	client->hrtb_interval = 2000;
	client->hrtb_acks = 1;
	client->seq = 0;

	client->ws = libwsclient_new("wss://gateway.discord.gg/?v=6&encoding=json");

	log_info("initializing Websocket client...");

	if(!client->ws) {
		log_fatal("unable to initialize new WS client!");
		return 0;
	}

	libwsclient_onopen(client->ws, &onopen);
	libwsclient_onmessage(client->ws, &onmessage);
	libwsclient_onerror(client->ws, &onerror);
	libwsclient_onclose(client->ws, &onclose);

	return 1;
}