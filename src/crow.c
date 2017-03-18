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

extern const char *__progname;

const char *token;
int done;
user_t bot; // Our bot user

void 
on_discord_message(message_t msg) {
	
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

	log_init(debug, __progname);

	if (!token) {
		fprintf(stderr, "no token provided\n");
		usage();
		exit(1);
	}

	log_debug("main", "Starting with token %s...", token);

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