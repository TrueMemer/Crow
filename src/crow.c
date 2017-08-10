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

#include "../include/crow.h"

#include "../deps/libwsclient/wsclient.h"

#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>

// Better but still gross

client_t bot;

int startsWith(const char *a, const char *b)
{
   if(strncmp(a, b, strlen(b)) == 0) return 1;
   return 0;
}

static void
usage(void)
{
	fprintf(stderr, "Usage: ./crow [OPTIONS]\n");
	//fprintf(stderr, "Version: %s\n", PACKAGE_STRING);
	fprintf(stderr, "\n");
	fprintf(stderr, " -d, --debug        be more verbose.\n");
	fprintf(stderr, " -h, --help         display help and exit\n");
	fprintf(stderr, " -v, --version      print version and exit\n");
	fprintf(stderr, " -t, --token        your bot token. REQUIRED\n");
	fprintf(stderr, " -s, --save         save internal config struct to ~/.crowrc\n");
	fprintf(stderr, "\n");
	//fprintf(stderr, "see manual page " PACKAGE "(8) for more information\n");
}

void handshake(wsclient *c) {
	char _token[1024];
    snprintf(_token, sizeof(_token), "{\"op\":2,\"d\":{\"token\":\"%s\",\"v\":4,\"encoding\":\"etf\",\"properties\":{\"$os\":\"linux\",\"browser\":\"crow\",\"device\":\"crow\",\"referrer\":\"\",\"referring_domain\":\"\"},\"compress\":false,\"large_threshold\":250,\"shard\":[0,1]}}", cfg_get(bot.config, "token"));
    json_object *response = json_tokener_parse(_token);
    libwsclient_send(c, json_object_to_json_string(response));
}

int
onmessage(wsclient *c, wsclient_message *msg) {
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
			dispatch(&bot, m);
			break;
		case HELLO:
			if (!bot.hello) {
				bot.hello = 1;
				json_object *hello_;
				json_object *d;
				json_object_object_get_ex(json_tokener_parse(msg->payload), "d", &d);
				json_object_object_get_ex(d, "heartbeat_interval", &hello_);
				bot.hrtb_interval = json_object_get_int(hello_);

				json_object_put(hello_);
				json_object_put(d);
				handshake(c);
				break;
			}
		case HEARTBEAT_ACK:
			log_debug("heartbeat acks!");
			bot.hrtb_acks = 1;
			break;
	}

    return;
}

void 
onclose(wsclient *c) {
    log_warn("ws closed: %d", c->sockfd);
    bot.done = 1;
}

void 
onerror(wsclient *c, wsclient_error *err) {
    log_error("error %d has occuried: %s!\n", err->code, err->str);
    if(err->extra_code) {
        errno = err->extra_code;
        perror("recv");
    }
    bot.done = 1;
}

void 
onopen(wsclient *c) {
    log_info("onopen websocket function is called!");
	return;
}

void reconnect(client_t *bot) {
	libwsclient_finish(bot->ws);

	bot->ws = libwsclient_new("wss://gateway.discord.gg/?v=6&encoding=json");

	if(!bot->ws) {
		log_fatal("unable to reinitialize new WS client!");
		exit(1);
	}

	libwsclient_onopen(bot->ws, &onopen);
	libwsclient_onmessage(bot->ws, &onmessage);
	libwsclient_onerror(bot->ws, &onerror);
	libwsclient_onclose(bot->ws, &onclose);

	libwsclient_run(bot->ws);

	log_info("reconnected!");
}

int
main(int argc, char *argv[])
{
	bot.config = cfg_init();
	bot.hrtb_interval = 2000;
	bot.hrtb_acks = 1;
	bot.seq = 0;

	int ch;
	int save = 0;
	int no_config;
	const char *token;

	static struct option long_options[] = {
                { "debug", no_argument, 0, 'd' },
                { "help",  no_argument, 0, 'h' },
                { "version", no_argument, 0, 'v' },
				{ "token", optional_argument, 0, 't'},
				{ "config", optional_argument, 0, 'c'},
				{ "save", no_argument, 0, 's' },
		{ 0 }
	};
	while (1) {
		int option_index = 0;
		ch = getopt_long(argc, argv, "hvsdD:",
		    long_options, &option_index);
		if (ch == -1) break;
		switch (ch) {
		case 'h':
			usage();
			exit(0);
			break;
		// case 'v':
		// 	fprintf(stdout, "%s\n", PACKAGE_VERSION);
		// 	exit(0);
		// 	break;
		case 'd':
			log_set_level(LOG_DEBUG);
			break;
		case 's':
			save = 1;
			break;
		case 'c':
			bot.homedir = optarg;
			break;
		case 't':
			token = optarg;
			cfg_set(bot.config, "token", optarg);
			break;
		default:
			fprintf(stderr, "unknown option `%c'\n", ch);
			usage();
			exit(1);
		}
	}

	if (!bot.homedir) {
		if ((bot.homedir = getenv("HOME")) == NULL) {
    		bot.homedir = getpwuid(getuid())->pw_dir;
		}
		strcat(bot.homedir, "/.crowrc");
	}

	int loaded = cfg_load(bot.config, bot.homedir);

	if (loaded < 0) {
		no_config = 1;
	} else {
		// if (!token) {
		// 	cfg_set(bot.config, "token", cfg_get(bot.config, "token"));
		// }
		// bot.bot_prefix = cfg_get(bot.config, "bot_prefix");

		// log_debug(bot.bot_prefix);

		// if (bot.bot_prefix == NULL) {
		// 	bot.bot_prefix = "??";
		// }
	}

	log_debug(cfg_get(bot.config, "bot_prefix"));

	bot.bot_prefix = cfg_get(bot.config, "bot_prefix");

	if ( (no_config && !token) || (cfg_get(bot.config, "token") == NULL) ) {
		fprintf(stderr, "no token provided\n");
		usage();
		exit(1);
	}

	if (save) {
		cfg_save(bot.config, bot.homedir);
	}

	token = cfg_get(bot.config, "token");

	init_curl(token);

	log_info("starting with token %s", token);

	bot.ws = libwsclient_new("wss://gateway.discord.gg/?v=6&encoding=json");

	log_info("initializing Websocket client...");

	if(!bot.ws) {
		log_fatal("unable to initialize new WS client!");
		exit(1);
	}

	libwsclient_onopen(bot.ws, &onopen);
	libwsclient_onmessage(bot.ws, &onmessage);
	libwsclient_onerror(bot.ws, &onerror);
	libwsclient_onclose(bot.ws, &onclose);

	libwsclient_run(bot.ws);

	log_info("WS client thread is running...");

	fflush(stdout);
    while(!bot.done) {
		if (bot.hello) {
			if (!bot.hrtb_acks) {
				log_debug("zombie! reconnecting...");
				reconnect(&bot);
				continue;
			}
			json_object *heartbeat;

			heartbeat = json_object_new_object();
			json_object_object_add(heartbeat, "op", json_object_new_int(1));
			json_object_object_add(heartbeat, "d", json_object_new_int(bot.seq));

			libwsclient_send(bot.ws, json_object_to_json_string(heartbeat));
			fflush(stdout);
			log_debug("sleeping for %d", bot.hrtb_interval * 1000);
			//log_debug("hrtb: %s", json_object_to_json_string(heartbeat));
			json_object_put(heartbeat);
			bot.hrtb_acks = 0;
			usleep(bot.hrtb_interval * 1000);
		}
	}

	libwsclient_finish(bot.ws);

	return EXIT_SUCCESS;
}