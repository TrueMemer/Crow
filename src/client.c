#include <crow/client.h>
#include <crow/events.h>
#include <crow/log.h>

#include <nopoll/nopoll.h>
#include <pthread.h>

#include <json-c/json.h>

void handshake(client_t *client) 
{
	log_debug("Handshaking...");

	char *token;

	// TODO: Put it in json object
    asprintf(&token, "{\"op\":2,\"d\":{\"token\":\"%s\",\"properties\":{\"$os\":\"linux\",\"browser\":\"crow\",\"device\":\"crow\",\"referrer\":\"\",\"referring_domain\":\"\"},\"compress\":false,\"large_threshold\":250}}", TOKEN);
	
	json_object *response = json_tokener_parse(token);
	
	log_trace(json_object_get_string(response));

	nopoll_conn_send_text(client->conn, token, strlen(token));

	log_debug("Handshaked!");
}

void on_message(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr user_data) 
{
	log_trace("get message");
	client_t *ptr = user_data;

	unsigned char *payload;

	if (!nopoll_msg_is_final(msg) && nopoll_msg_is_fragment(msg))
	{
		if (!ptr->last_payload)
		{
			ptr->last_payload = nopoll_msg_get_payload(msg);
		} else
		{
			strcat(ptr->last_payload, nopoll_msg_get_payload(msg));
		}
		log_warn("Recieved cut message. Waiting for the final fragment...");
		log_trace(payload);
		return;
	}

	if (nopoll_msg_is_fragment(msg) && nopoll_msg_is_final(msg))
	{
		strcat(ptr->last_payload, nopoll_msg_get_payload(msg));
		log_trace("got final:\n %s", ptr->last_payload);
		payload = ptr->last_payload;
		ptr->last_payload = NULL;
	} else 
	{
		payload = nopoll_msg_get_payload(msg);
	}

    json_object *m = json_tokener_parse(payload);
	
	json_object *_op;

	json_object_object_get_ex(m, "op", &_op);
	

	if (json_object_get_type(_op) == json_type_null) 
	{
		log_warn("recieved a message with unknown op!");
		log_trace(json_object_get_string(m));
		return;
	}

	log_trace(payload);
	
	int op = json_object_get_int(_op);

	switch (op) 
	{

		case DISPATCH: 
		{
			log_debug("Got event");
			dispatch(ptr, m);
			break;
		}

		case HELLO: 
		{
			log_debug("Recieved HELLO");
			ptr->hello = 1;
			ptr->hrtb_acks = 1;
			json_object *hello_;
			json_object *d;
			json_object_object_get_ex(json_tokener_parse(payload), "d", &d);
			json_object_object_get_ex(d, "heartbeat_interval", &hello_);
			ptr->hrtb_interval = json_object_get_int(hello_);
			handshake(ptr);
			break;
		}

		case INVALIDATE_SESSION:
		{
			// NOTE: This is untested
			log_warn("Recieved INVALIDATE_SESSION opcode");

			json_object *_d;
			json_object_object_get_ex(m, "d", &_d);

			int resumable = json_object_get_boolean(_d);

			if (!resumable)
			{
				log_fatal("Current session is not resumable. User action required.");
			} else 
			{
				log_warn("Session is resumable, trying to reconnect...");
				crow_reconnect(ptr);
			}

			break;
		}

		case HEARTBEAT_ACK:
		{
			log_debug("heartbeat acks!");
			ptr->hrtb_acks = 1;
			break;
		}
		
		case RECONNECT:
		{
			log_warn("got RESUME event! reconnecting...");
			crow_reconnect(ptr);
			break;
		}

	}

    return;
}

// void 
// onclose(wsclient *c) 
// {
// 	client_t *ptr = c->client_ptr;
//     log_warn("ws closed: %d", c->sockfd);
//     ptr->done = 1;
// }

// void 
// onerror(wsclient *c, wsclient_error *err) 
// {
// 	client_t *ptr = c->client_ptr;
//     log_error("error %d has occuried: %s!\n", err->code, err->str);
//     ptr->done = 1;
// }

nopoll_bool on_ready(noPollCtx *ctx, noPollConn *conn, noPollPtr user_data)
{
	log_info("Websocket server is ready");
	return nopoll_true;
}

void crow_reconnect(client_t *client)
{
	/* Fix this for NoPoll

	client->hello = 0;

	libwsclient_finish(client->ws);
	
	client->ws = libwsclient_new("wss://gateway.discord.gg/?v=6&encoding=json", (void *)&client);

	log_info("Reconnecting to websocket server...");

	if(!client->ws) 
	{
		log_fatal("unable to initialize new WS client!");
		return NULL;
	}

	// libwsclient_onopen(client->ws, &onopen);
	// libwsclient_onmessage(client->ws, &onmessage);
	// libwsclient_onerror(client->ws, &onerror);
	// libwsclient_onclose(client->ws, &onclose);

	crow_run(client);

	json_object *resume = json_object_new_object();

	json_object_object_add(resume, "token", json_object_new_string(TOKEN));
	json_object_object_add(resume, "session_id", json_object_new_string(client->session_id));
	json_object_object_add(resume, "seq", json_object_new_int(client->seq));

	libwsclient_send(client->ws, json_object_to_json_string(resume));

	log_debug(json_object_to_json_string(resume));

	client->hello = 1;
	*/

}

void heartbeat(void *client_ptr) 
{

	client_t *client = client_ptr;

	log_debug("HEARTBEAT: %d, %d", client->hello, client->hrtb_acks);

	while(!client->done) 
	{
		if (client->hello == 1 && client->hrtb_acks == 1) 
		{
			json_object *heartbeat;

			log_debug("%d", client->hrtb_interval);

			heartbeat = json_object_new_object();
			json_object_object_add(heartbeat, "op", json_object_new_int(1));
			json_object_object_add(heartbeat, "d", json_object_new_int(client->seq));

			const char *to_send = json_object_to_json_string(heartbeat);

			nopoll_conn_send_text(client->conn, to_send, strlen(to_send));

			log_debug("sleeping for %d", client->hrtb_interval * 1000);

			json_object_put(heartbeat);
			client->hrtb_acks = 0;
			fflush(stdout);
			usleep(client->hrtb_interval * 1000);
		}
	}
}

void crow_run(client_t *client) 
{ 
	
	noPollConnOpts * opts;

	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_set_ssl_protocol(opts, NOPOLL_METHOD_TLSV1_2);
	nopoll_conn_opts_ssl_peer_verify(opts, nopoll_false);

	nopoll_ctx_set_on_msg(client->ctx, &on_message, (void *)&client);
	nopoll_ctx_set_on_ready(client->ctx, &on_ready, (void *)&client);

	client->conn = nopoll_conn_tls_new(client->ctx, opts, "gateway.discord.gg", "443", NULL, "/?v=6&encoding=json", NULL, NULL);

	if (!nopoll_conn_is_ok(client->conn))
	{
		log_fatal("Can't connect to websocket server!");
		log_fatal("%d", nopoll_conn_get_close_status(client->conn));
		nopoll_ctx_unref(client->ctx);
		return;
	}

	log_info("Connecting to websocket server...");

	if (!nopoll_conn_wait_until_connection_ready(client->conn, 5))
	{

	}

	log_info("WS client thread is running...");

	while (!client->hello) 
	{
		noPollMsg *msg = nopoll_conn_get_msg(client->conn);
		if (msg)
		{
			on_message(client->ctx, client->conn, msg, (void *)client);
		} else 
		{
			log_debug("Waiting for HELLO...");
			sleep(5);
		}
	}

	pthread_t heartbeat_thread;

	if (pthread_create(&heartbeat_thread, NULL, &heartbeat, (void *)client))
	{
		log_fatal("Failed to create heartbeat thread");
		return;
	}

	// while (!client->done)
	// {
	// 	noPollMsg *msg = nopoll_conn_get_msg(client->conn);
	// 	if (msg)
	// 	{
	// 		on_message(client, msg);
	// 		nopoll_msg_unref(msg);
	// 	} else
	// 	{
	// 		if (!nopoll_conn_is_ok(client->done))
	// 		{
	// 			int status = nopoll_conn_get_close_status(client->conn);
	// 			const char *reason = nopoll_conn_get_close_reason(client->conn);
	// 			log_warn("Websocket connection is not ok (closed or lost). Stoping I/O loop and heartbeat thread.\nCode: %d\nReason:%s", status, reason);
	// 			client->done = 1;
	// 		}
	// 	}
	// }
	while (nopoll_true)
	{
		int err = nopoll_loop_wait(client->ctx, 0);

		if (err == -4)
		{
			log_error("I/O error");
			continue;
		}
	}
	

	client->done = 1;
}

void crow_finish(client_t *client) 
{
	
}

client_t *crow_new() 
{

	client_t *client = malloc(sizeof(client_t));

	client->hello = 0;
	client->hrtb_interval = 0;
	client->hrtb_acks = 0;
	client->seq = 0;
	client->done = 0;

	client->on_message = NULL;
	client->on_ready = NULL;

	//client->ws = libwsclient_new("wss://gateway.discord.gg/?v=6&encoding=json", (void *)&client);
	
	log_info("initializing Websocket client...");

	client->ctx = nopoll_ctx_new();

	if(!client->ctx) 
	{
		log_fatal("unable to initialize new WS client!");
		return NULL;
	}

	nopoll_log_enable(client->ctx, nopoll_true);
	nopoll_log_color_enable(client->ctx, nopoll_true);

	/*
	libwsclient_onopen(client->ws, &onopen);
	libwsclient_onmessage(client->ws, &onmessage);
	libwsclient_onerror(client->ws, &onerror);
	libwsclient_onclose(client->ws, &onclose);
	*/

	return client;

}