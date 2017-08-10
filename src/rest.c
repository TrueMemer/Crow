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

#include "../include/rest.h"
#include "../deps/librequests/include/requests.h"

char header[1024];

void init_curl(char *token) {
    sprintf(header, "Authorization: Bot %s", token);
}

void
send_message(char* channel_id, char* text) {
    req_t req;
	CURL *curl = requests_init(&req);

	json_object *tosend;
	tosend = json_object_new_object();
	json_object_object_add(tosend, "content", json_object_new_string(text));

	char target[1024];
	sprintf(target, "https://discordapp.com/api/channels/%s/messages", channel_id);

    char *auth_header[] = {
        header
    };

	requests_post_headers(curl, &req, target, json_object_to_json_string(tosend), auth_header, sizeof(auth_header)/sizeof(char*));

	if (req.ok) {
        log_debug("Request URL: %s\n", req.url);
        log_debug("Response Code: %lu\n", req.code);
        log_debug("Response Body:\n%s", req.text);
	}

    requests_close(&req);
}

guild_channel_t
get_channel(char* channel_id) {
	req_t req;
	CURL *curl = requests_init(&req);

    char target[1024];

	sprintf(target, "https://discordapp.com/api/channels/%s", channel_id);

    char *auth_header[] = {
        header
    };

	requests_get_headers(curl, &req, target, auth_header, sizeof(auth_header)/sizeof(char*));

    if (!req.ok) {
        log_debug("get_channel: something went wrong!");
        return;
    }

    guild_channel_t channel;

    json_object *output;

    output = json_tokener_parse(req.text);

    channel.guild_id = json_object_get_string(json_object_object_get(output, "guild_id"));
    channel.name = json_object_get_string(json_object_object_get(output, "name"));
    channel.id = json_object_get_string(json_object_object_get(output, "id"));
    if (!strcmp(json_object_get_string(json_object_object_get(output, "type")), "text"))
        channel.type = 0;
    else
        channel.type = 1;

    channel.position = json_object_get_int(json_object_object_get(output, "position"));

    //channel.is_private = json_object_get_bool(json_object_object_get(output, "is_private"));

    channel.topic = json_object_get_string(json_object_object_get(output, "topic"));

    channel.last_message_id = json_object_get_string(json_object_object_get(output, "last_message_id"));

    if (channel.type = 1) {
        channel.bitrate = json_object_get_int(json_object_object_get(output, "bitrate"));
        channel.bitrate = json_object_get_int(json_object_object_get(output, "user_limit"));
    }

    requests_close(&req);

    return channel;
}

void
add_reaction(char* channel_id, char *message_id, char *emoji) {
	req_t req;
	CURL *curl = requests_init(&req);

	char target[1024];

    char *auth_header[] = {
        header
    };

	sprintf(target, "https://discordapp.com/api/channels/%s/messages/%s/reactions/%s/@me",channel_id, message_id, emoji);

    requests_put_headers(curl, &req, target, NULL, auth_header, sizeof(auth_header)/sizeof(char*));

    log_debug("Request URL: %s\n", req.url);
    log_debug("Response Code: %lu\n", req.code);
    log_debug("Response Body:\n%s", req.text);

    requests_close(&req);
}

// void
// delete_own_reaction(char* channel_id, char *message_id, char *emoji) {
// 	CURLcode res;

// 	char target[1024];

// 	snprintf(target, sizeof(target), "https://discordapp.com/api/channels/%s/messages/%s/reactions/%s/@me",channel_id, message_id, emoji);

//     curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

// 	res = curl_fetch_url(curl, target, cf);

//     long response_code;

//     curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

//     if (response_code != 204) {
//         if (res != CURLE_OK || cf->size < 1) {
//             fprintf(stderr, "ERROR: Failed to fetch url (%s) - curl said: %s",
//                 target, curl_easy_strerror(res));
//         }
//     }
    
// 	if (cf->payload != NULL) {
// 		log_debug("CURL returned %s", cf->payload);
// 	}
// }