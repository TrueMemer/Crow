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

struct cfg_struct *config;
char *homedir;
char *bot_prefix;
int done;
user_t bot; // Our bot user

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

// Ignore deprecated function and discarded qualifiers
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
int 
onmessage(wsclient *c, wsclient_message *msg, CURL *curl) {
    json_object *m = json_tokener_parse(msg->payload);
    json_object *t;
    t = json_object_object_get(m, "t");
    log_debug("Got new event \"%s\"\n", json_object_get_string(t));
    if (strcmp(json_object_get_string(t), "READY") == 0) {
        json_object *output_user = json_object_object_get(json_object_object_get(m, "d"), "user");
        bot = user(output_user);
    }
    if (strcmp(json_object_get_string(t), "MESSAGE_CREATE") == 0) {
        json_object *output = json_object_object_get(m, "d");

        message_t msg = message(output);

        on_discord_message(msg, config);
    }
	// if (strcmp(json_object_get_string(t), "PRESENCE_UPDATE") == 0) {
	// 	log_debug("websocket", msg->payload);
	// 	json_object *output = json_object_object_get(message, "d");
	// 	json_object *output_user = json_object_object_get(output, "user");
	// 	presence_update_t upd;
	// 	user_t upd_user;

	// 	upd_user.id = json_object_get_string(json_object_object_get(output_user, "id"));

	// 	upd.user = upd_user;
	// 	json_object *_roles_array = json_object_object_get(output, "roles");
	// 	array_list *_roles_array_list = json_object_get_array(_roles_array);

	// 	for (int i = 0; i < _roles_array_list->length; i++) {
	// 		upd.roles[i] = json_object_array_get_idx(_roles_array_list, i);
	// 	}

	// 	int is_game_null = json_object_get_type(json_object_object_get(output, "game"));

	// 	json_object *game_object;

	// 	switch (is_game_null) {
	// 		case json_type_null:
	// 			break;
	// 		case json_type_object:
			
	// 			game_object = json_object_object_get(output, "game");
	// 			game_t game;

	// 			game.name = json_object_get_string(json_object_object_get(game_object, "name"));
	// 			game.type = json_object_get_int(json_object_object_get(game_object, "type"));
	// 			game.url = json_object_get_string(json_object_object_get(game_object, "url"));

	// 			upd.game = game;

	// 			break;
	// 		default:
	// 			break;
	// 	}

	// 	upd.guild_id = json_object_get_string(json_object_object_get(output, "guild_id"));
	// 	upd.status = json_object_get_string(json_object_object_get(output, "status"));

	// 	on_presence_update(upd);
	// }
    return 0;
}

void 
onclose(wsclient *c) {
    log_warn("WS server closed the connection!");
    done = 1;
}

void 
onerror(wsclient *c, wsclient_error *err) {
    log_error("error %d has occuried: %s!\n", err->code, err->str);
    if(err->extra_code) {
        errno = err->extra_code;
        perror("recv");
    }
    done = 1;
}

void 
onopen(wsclient *c) {
    log_info("onopen websocket function is called!");
    char _token[1024];
    snprintf(_token, sizeof(_token), "{\"op\":2,\"d\":{\"token\":\"%s\",\"v\":4,\"encoding\":\"etf\",\"properties\":{\"$os\":\"linux\",\"browser\":\"crow\",\"device\":\"crow\",\"referrer\":\"\",\"referring_domain\":\"\"},\"compress\":false,\"large_threshold\":250,\"shard\":[0,1]}}", cfg_get(config, "token"));
    json_object *response = json_tokener_parse(_token);
    libwsclient_send(c, json_object_to_json_string(response));
}


int
main(int argc, char *argv[])
{
	config = cfg_init();

	int ch;
	int save = 0;
	int no_config;
	char *token;

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
			homedir = optarg;
			break;
		case 't':
			token = optarg;
			cfg_set(config, "token", optarg);
			break;
		default:
			fprintf(stderr, "unknown option `%c'\n", ch);
			usage();
			exit(1);
		}
	}

	if (!homedir) {
		if ((homedir = getenv("HOME")) == NULL) {
    		homedir = getpwuid(getuid())->pw_dir;
		}
		strcat(homedir, "/.crowrc");
		printf("%s", homedir);
	}

	// default prefix
	cfg_set(config, "bot_prefix", "??");

	int loaded = cfg_load(config, homedir);

	if (loaded < 0) {
		no_config = 1;
	} else {
		if (!token) {
			cfg_set(config, "token", cfg_get(config, "token"));
		}
		cfg_set(config, "bot_prefix", cfg_get(config, "bot_prefix"));
	}

	if ( (no_config && !token) || (cfg_get(config, "token") == NULL) ) {
		fprintf(stderr, "no token provided\n");
		usage();
		exit(1);
	}

	if (save) {
		cfg_save(config, homedir);
	}

	token = cfg_get(config, "token");

	init_curl(token);

	log_info("starting with token %s", token);

	wsclient *client = libwsclient_new("wss://gateway.discord.gg");

	log_info("initializing Websocket client...");

	if(!client) {
		log_fatal("unable to initialize new WS client!");
		exit(1);
	}

	libwsclient_onopen(client, &onopen);
	libwsclient_onmessage(client, &onmessage);
	libwsclient_onerror(client, &onerror);
	libwsclient_onclose(client, &onclose);

	libwsclient_run(client);

	log_info("WS client thread is running...");

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