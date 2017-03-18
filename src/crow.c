/* -*- mode: c; c-file-style: "openbsd" -*- */
/*
 * Copyright (c) 2017, Alexander Memer <mkoaleksedos@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "crow.h"

#include <curl/curl.h>
#include <wsclient/wsclient.h>
#include <json.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>

extern const char *__progname;

CURL *curl;
struct curl_slist *headers;
struct curl_fetch_st curl_fetch; 
struct curl_fetch_st *cf = &curl_fetch;

const char *token;
char *bot_prefix;
int done;
user_t bot; // Our bot user

bool startsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

// I'm not in this memory shit so I copypasted it
size_t curl_callback (void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;                             /* calculate buffer size */
    curl_fetch_t *p = (struct curl_fetch_st *) userp;   /* cast pointer to fetch struct */

    /* expand buffer */
    p->payload = (char *) realloc(p->payload, p->size + realsize + 1);

    /* check buffer */
    if (p->payload == NULL) {
      /* this isn't good */
      fprintf(stderr, "ERROR: Failed to expand buffer in curl_callback");
      /* free buffer */
      free(p->payload);
      /* return */
      return -1;
    }

    /* copy contents to buffer */
    memcpy(&(p->payload[p->size]), contents, realsize);

    /* set new buffer size */
    p->size += realsize;

    /* ensure null termination */
    p->payload[p->size] = 0;

    /* return size */
    return realsize;
}

/* fetch and return url body via curl */
CURLcode curl_fetch_url(CURL *ch, const char *url, struct curl_fetch_st *fetch) {
    CURLcode rcode;

    fetch->payload = (char *) calloc(1, sizeof(fetch->payload));

    if (fetch->payload == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate payload in curl_fetch_url");
        return CURLE_FAILED_INIT;
    }

    fetch->size = 0;

    curl_easy_setopt(ch, CURLOPT_URL, url);

    curl_easy_setopt(ch, CURLOPT_WRITEFUNCTION, curl_callback);

    curl_easy_setopt(ch, CURLOPT_WRITEDATA, (void *) fetch);

    curl_easy_setopt(ch, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    curl_easy_setopt(ch, CURLOPT_TIMEOUT, 5);

    curl_easy_setopt(ch, CURLOPT_FOLLOWLOCATION, 1);

    curl_easy_setopt(ch, CURLOPT_MAXREDIRS, 1);

    rcode = curl_easy_perform(ch);

    return rcode;
}

void
send_message(char* channel_id, char* text) {
	CURLcode res;

	json_object *tosend;
	
	tosend = json_object_new_object();

	json_object_object_add(tosend, "content", json_object_new_string(text));

	char target[1024];

	snprintf(target, sizeof(target), "https://discordapp.com/api/channels/%s/messages", channel_id);

	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_object_to_json_string(tosend));

	res = curl_fetch_url(curl, target, cf);

	if (res != CURLE_OK || cf->size < 1) {
		fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s",
			target, curl_easy_strerror(res));
	}

	if (cf->payload != NULL) {
		log_debug("curl", "CURL returned %s", cf->payload);
	}
}

void 
on_discord_message(message_t msg) {
	if (strcmp(msg.author.id, bot.id)) {
		if (startsWith(msg.content, bot_prefix)) {
			msg.content = msg.content + strlen(bot_prefix);

			if (!strcmp("ping", msg.content)) {
			send_message(msg.channel_id, "Pong!");
			}
			if (startsWith(msg.content, "echo")) {
				send_message(msg.channel_id, msg.content);
			}
			if (!strcmp("hi", msg.content)) {
				char to_send[1024];

				snprintf(to_send, sizeof(to_send), "Hi, %s!", msg.author.username);

				send_message(msg.channel_id, to_send);
			}
		}
	}
}

static void
usage(void)
{
	fprintf(stderr, "Usage: %s [OPTIONS]\n",
	    __progname);
	fprintf(stderr, "Version: %s\n", PACKAGE_STRING);
	fprintf(stderr, "\n");
	fprintf(stderr, " -d, --debug        be more verbose.\n");
	fprintf(stderr, " -h, --help         display help and exit\n");
	fprintf(stderr, " -v, --version      print version and exit\n");
	fprintf(stderr, " -t, --token        your bot token. REQUIRED\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "see manual page " PACKAGE "(8) for more information\n");
}

// Ignore deprecated function and discarded qualifiers
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
int 
onmessage(wsclient *c, wsclient_message *msg, CURL *curl) {
    json_object *message;
    message = json_tokener_parse(msg->payload);
    json_object *t;
    t = json_object_object_get(message, "t");
    log_debug("websocket", "Got new event \"%s\"\n", json_object_get_string(t));
    if (strcmp(json_object_get_string(t), "READY") == 0) {
        json_object *output = json_object_object_get(message, "d");
        json_object *output_user = json_object_object_get(output, "user");
        user_t _bot;
		_bot.id = json_object_get_string(json_object_object_get(output_user, "id"));
		_bot.username = json_object_get_string(json_object_object_get(output_user, "username"));
		_bot.discriminator = json_object_get_string(json_object_object_get(output_user, "discriminator"));
		_bot.avatar = json_object_get_string(json_object_object_get(output_user, "avatar"));
		_bot.bot = json_object_get_int(json_object_object_get(output_user, "bot"));
		_bot.mfa_enabled = json_object_get_int(json_object_object_get(output_user, "mfa_enabled"));
		_bot.verified = json_object_get_int(json_object_object_get(output_user, "verified"));
		_bot.email = json_object_get_string(json_object_object_get(output_user, "email"));
        bot = _bot;
    }
    if (strcmp(json_object_get_string(t), "MESSAGE_CREATE") == 0) {
        json_object *output = json_object_object_get(message, "d");
        json_object *_author = json_object_object_get(output, "author");

        user_t author;
		author.id = json_object_get_string(json_object_object_get(_author, "id"));
		author.username = json_object_get_string(json_object_object_get(_author, "username"));
		author.discriminator = json_object_get_string(json_object_object_get(_author, "discriminator"));
		author.avatar = json_object_get_string(json_object_object_get(_author, "avatar"));
		author.bot = json_object_get_int(json_object_object_get(_author, "bot"));
		author.mfa_enabled = json_object_get_int(json_object_object_get(_author, "mfa_enabled"));
		author.verified = json_object_get_int(json_object_object_get(_author, "verified"));
		author.email = json_object_get_string(json_object_object_get(_author, "email"));

		// json_object *mentions;

		// if (json_object_object_get(output, "mentions")) {
		// 	mentions = json_object_object_get(output, "mentions");
		// 	user_t mentions_array[json_object_array_length(mentions)];
		// 	for (int i = 0; i <= json_object_array_length(mentions); i++) {
		// 		json_object *curr = json_object_array_get_idx(mentions, i);
		// 		mentions_array[i].id = json_object_get_string(json_object_object_get(curr, "id"));
		// 		mentions_array[i].username = json_object_get_string(json_object_object_get(curr, "username"));
		// 		mentions_array[i].discriminator = json_object_get_string(json_object_object_get(curr, "discriminator"));
		// 		mentions_array[i].avatar = json_object_get_string(json_object_object_get(curr, "avatar"));
		// 		mentions_array[i].bot = json_object_get_int(json_object_object_get(curr, "bot"));
		// 		mentions_array[i].mfa_enabled = json_object_get_int(json_object_object_get(curr, "mfa_enabled"));
		// 		mentions_array[i].verified = json_object_get_int(json_object_object_get(curr, "verified"));
		// 		mentions_array[i].email = json_object_get_string(json_object_object_get(curr, "email"));
		// 	}
		// }

        message_t msg;
		msg.id = json_object_get_string(json_object_object_get(output, "id"));
		msg.author = author;
		msg.channel_id = json_object_get_string(json_object_object_get(output, "channel_id"));
		msg.timestamp = json_object_get_string(json_object_object_get(output, "timestamp"));
		msg.edited_timestamp = json_object_get_string(json_object_object_get(output, "edited_timestamp"));
		msg.tts = json_object_get_int(json_object_object_get(output, "tts"));
		msg.mention_everyone = json_object_get_int(json_object_object_get(output, "mention_everyone"));
		msg.pinned = json_object_get_int(json_object_object_get(output, "pinned"));
		msg.webhook_id = json_object_get_string(json_object_object_get(output, "webhook_id"));
		msg.content = json_object_get_string(json_object_object_get(output, "content"));

        on_discord_message(msg);
    }
    return 0;
}

void 
onclose(wsclient *c) {
    log_info("websocket", "WS server closed the connection!");
    done = 1;
}

void 
onerror(wsclient *c, wsclient_error *err) {
    log_crit("websocket", "Error %d has occuried: %s!\n", err->code, err->str);
    if(err->extra_code) {
        errno = err->extra_code;
        perror("recv");
    }
    done = 1;
}

void 
onopen(wsclient *c) {
    log_info("websocket", "Onopen websocket function is called!");
    char _token[1024];
    snprintf(_token, sizeof(_token), "{\"op\":2,\"d\":{\"token\":\"%s\",\"v\":4,\"encoding\":\"etf\",\"properties\":{\"$os\":\"linux\",\"browser\":\"crow\",\"device\":\"crow\",\"referrer\":\"\",\"referring_domain\":\"\"},\"compress\":false,\"large_threshold\":250,\"shard\":[0,1]}}", token);
    json_object *response = json_tokener_parse(_token);
    libwsclient_send(c, json_object_to_json_string(response));
}


int
main(int argc, char *argv[])
{
	int debug = 1;
	int ch;

	static struct option long_options[] = {
                { "debug", no_argument, 0, 'd' },
                { "help",  no_argument, 0, 'h' },
                { "version", no_argument, 0, 'v' },
				{ "token", required_argument, 0, 't'},
				{ "prefix", required_argument, 0, 'p'},
		{ 0 }
	};
	while (1) {
		int option_index = 0;
		ch = getopt_long(argc, argv, "hvdD:",
		    long_options, &option_index);
		if (ch == -1) break;
		switch (ch) {
		case 'h':
			usage();
			exit(0);
			break;
		case 'v':
			fprintf(stdout, "%s\n", PACKAGE_VERSION);
			exit(0);
			break;
		case 'd':
			debug++;
			break;
		case 'p':
			bot_prefix = optarg;
			break;
		case 'D':
			log_accept(optarg);
			break;
		case 't':
			token = optarg;
			break;
		default:
			fprintf(stderr, "unknown option `%c'\n", ch);
			usage();
			exit(1);
		}
	}

	if (!bot_prefix) { bot_prefix = '!'; }

	log_init(debug, __progname);

	if (!token) {
		fprintf(stderr, "no token provided\n");
		usage();
		exit(1);
	}

	log_debug("main", "Starting with token %s and prefix %s...", token, bot_prefix);

	if ((curl = curl_easy_init()) == NULL) {
        log_crit("curl", "Failed to create curl init object!");
        exit(1);
	}

	char authorization_header[1024];

	snprintf(authorization_header, sizeof(authorization_header), "Authorization: Bot %s", token);

	headers = curl_slist_append(headers, authorization_header);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	wsclient *client = libwsclient_new("wss://gateway.discord.gg");

	log_info("main", "Initializing Websocket client...");

	if(!client) {
		log_crit("websocket", "Unable to initialize new WS client!");
		exit(1);
	}

	libwsclient_onopen(client, &onopen);
	libwsclient_onmessage(client, &onmessage);
	libwsclient_onerror(client, &onerror);
	libwsclient_onclose(client, &onclose);

	libwsclient_run(client);

	log_info("main", "WS client thread is running...");

	fflush(stdout);
	sleep(2);
    while(!done) {
            libwsclient_send(client, "{\"op\": 1, \"d\": null}");
            fflush(stdout);
            sleep(5);
	}

	libwsclient_finish(client);

	return EXIT_SUCCESS;
}