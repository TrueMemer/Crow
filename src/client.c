#include <crow/client.h>
#include <crow/events.h>
#include <crow/log.h>

#include <json-c/json.h>

void handshake(client_t *client) {
	log_debug("Handshaking...");
	char _token[1024];
    snprintf(_token, sizeof(_token), "{\"op\":2,\"d\":{\"token\":\"%s\",\"v\":4,\"encoding\":\"etf\",\"properties\":{\"$os\":\"linux\",\"browser\":\"crow\",\"device\":\"crow\",\"referrer\":\"\",\"referring_domain\":\"\"},\"compress\":false,\"large_threshold\":250,\"shard\":[0,1]}}", TOKEN);
    json_object *response = json_tokener_parse(_token);
    libwsclient_send(client->ws, json_object_to_json_string(response));
	log_debug("Handshaked!");
}

int onmessage(wsclient *c, wsclient_message *msg) {

	client_t *ptr = c->client_ptr;

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
			dispatch(ptr, m);
			break;
		case 10: {
				log_debug("Recieved HELLO");
				ptr->hello = 1;
				ptr->hrtb_acks = 1;
				json_object *hello_;
				json_object *d;
				json_object_object_get_ex(json_tokener_parse(msg->payload), "d", &d);
				json_object_object_get_ex(d, "heartbeat_interval", &hello_);
				ptr->hrtb_interval = json_object_get_int(hello_);
				handshake(ptr);
			break;
		}

		case HEARTBEAT_ACK:
			log_debug("heartbeat acks!");
			ptr->hrtb_acks = 1;
			break;
		
		case RECONNECT:
			log_warn("got RESUME event! reconnecting...");
			reconnect(ptr);
			break;
	}

    return;
}

void 
onclose(wsclient *c) {
	client_t *ptr = c->client_ptr;
    log_warn("ws closed: %d", c->sockfd);
    ptr->done = 1;
}

void 
onerror(wsclient *c, wsclient_error *err) {
	client_t *ptr = c->client_ptr;
    log_error("error %d has occuried: %s!\n", err->code, err->str);
    ptr->done = 1;
}

void 
onopen(wsclient *c) {
    //log_info("onopen websocket function is called!");
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
	log_debug("HEARTBEAT: %d, %d", client->hello, client->hrtb_acks);
	while(!client->done) {
		if (client->hello == 1 && client->hrtb_acks == 1) {

			json_object *heartbeat;

			log_debug("%d", client->hrtb_interval);

			heartbeat = json_object_new_object();
			json_object_object_add(heartbeat, "op", json_object_new_int(1));
			json_object_object_add(heartbeat, "d", json_object_new_int(client->seq));

			libwsclient_send(client->ws, json_object_to_json_string(heartbeat));
			log_debug("sleeping for %d", client->hrtb_interval * 1000);
			json_object_put(heartbeat);
			client->hrtb_acks = 0;
			fflush(stdout);
			usleep(client->hrtb_interval * 1000);
		}
	}
}

void client_run(client_t *client) { 

	libwsclient_run(client->ws);
	log_info("WS client thread is running...");

	client->ws->client_ptr = client;

	while (!client->hello) {
		log_debug("Waiting for HELLO...");
		sleep(5);
	}
	heartbeat(client);

}

void client_finish(client_t *client) { libwsclient_finish(client->ws); client->done = 1; }

client_t *client_init() {

	client_t *client = malloc(sizeof(client_t));

	client->hello = 0;
	client->hrtb_interval = 0;
	client->hrtb_acks = 0;
	client->seq = 0;
	client->done = 0;

	client->on_message = NULL;
	client->on_ready = NULL;

	client->ws = libwsclient_new("wss://gateway.discord.gg/?v=6&encoding=json", (void *)&client);

	log_info("initializing Websocket client...");

	if(!client->ws) {
		log_fatal("unable to initialize new WS client!");
		return NULL;
	}

	libwsclient_onopen(client->ws, &onopen);
	libwsclient_onmessage(client->ws, &onmessage);
	libwsclient_onerror(client->ws, &onerror);
	libwsclient_onclose(client->ws, &onclose);

	return client;
}