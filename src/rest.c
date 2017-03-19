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

#include <curl/curl.h>
#include <json.h>

#include "log.h"
#include "crow.h"

CURL *curl;
struct curl_slist *headers;
struct curl_fetch_st curl_fetch; 
struct curl_fetch_st *cf = &curl_fetch;

void
init_curl() {
	if ((curl = curl_easy_init()) == NULL) {
        log_crit("curl", "Failed to create curl init object!");
        exit(1);
	}

    char authorization_header[1024];

	snprintf(authorization_header, sizeof(authorization_header), "Authorization: Bot %s", token);

	headers = curl_slist_append(headers, authorization_header);

	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
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