#include <stdio.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <json.h>
#include <wsclient/wsclient.h>
#include "iniparser/src/iniparser.h"

#include "crow.h"

struct curl_slist *header = NULL;
char *bot_token;
json_object *d;
CURL *curl;
int done = 0;
int can_send_message = 0;
int is_ready = 0;
struct user bot;

void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failedn");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = "";
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failedn");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = "";
    s->len = new_len;
    return size*nmemb;
}

void send_message (char* message, char* channel_id) {
    CURLcode res;
    struct string s;
    init_string(&s);
    char target[SIZE];
	char to_send[SIZE];
	snprintf(target, sizeof(target), "https://discordapp.com/api/channels/%s/messages", channel_id);
    snprintf(to_send, sizeof(to_send), "{\"content\": \"%s\"}", message);
	curl_easy_setopt(curl, CURLOPT_URL, target);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, to_send);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %sn",
    curl_easy_strerror(res));
    json_object *result, *a;
    result = json_tokener_parse(s.ptr);
    int error;
    error = json_object_object_get_ex(result, "message", &a);
		if (error) {
        printf("Error: \"%s\"\n", json_object_get_string(a));
    }
}

char* get_gateway_url (CURL *curl) {
    CURLcode res;
    struct string s;
    init_string(&s);
    curl_easy_setopt(curl, CURLOPT_URL, "https://discordapp.com/api/gateway");
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    fprintf(stderr, "curl_easy_perform() failed: %sn",
    curl_easy_strerror(res));
    json_object *result, *a;
    result = json_tokener_parse(s.ptr);
    int error;
    error = json_object_object_get_ex(result, "message", &a);
    if (error == 0) {
        a = json_object_object_get(result, "url");
        return json_object_get_string(a);
    }
    else {
        printf("Error: \"%s\"\n", json_object_get_string(a));
    }
}

int onclose(wsclient *c) {
    printf("WS server closed the connection.");
    // TODO error parsing
    done = 1;
    return 0;
}

int set_token(char* token) {
    char to_header[SIZE];
    snprintf(to_header, sizeof(to_header), "Authorization: Bot %s", token);
    header = curl_slist_append(header, to_header);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
    return 0;
}

int onerror(wsclient *c, wsclient_error *err) {
    fprintf(stderr, "onerror: (%d): %s\n", err->code, err->str);
    if(err->extra_code) {
        errno = err->extra_code;
        perror("recv");
    }
    done = 1;
}

int onmessage(wsclient *c, wsclient_message *msg, CURL *curl) {
    json_object *message;
    message = json_tokener_parse(msg->payload);
    json_object *t;
    t = json_object_object_get(message, "t");
    // TODO: sequence = json_object_object_get(message, "s");
    printf("Got new event \"%s\"\n", json_object_get_string(t));
    //printf("%s", json_object_to_json_string(message));
    if (strcmp(json_object_get_string(t), "READY") == 0) {
        json_object *output = json_object_object_get(message, "d");
        json_object *output_user = json_object_object_get(output, "user");
        struct user _bot = {
            .id = json_object_get_string(json_object_object_get(output_user, "id")),
            .username = json_object_get_string(json_object_object_get(output_user, "username")),
            .discriminator = json_object_get_string(json_object_object_get(output_user, "discriminator")),
            .avatar = json_object_get_string(json_object_object_get(output_user, "avatar")),
            .bot = json_object_get_int(json_object_object_get(output_user, "bot")),
            .mfa_enabled = json_object_get_int(json_object_object_get(output_user, "mfa_enabled")),
            .verified = json_object_get_int(json_object_object_get(output_user, "verified")),
            .email = json_object_get_string(json_object_object_get(output_user, "email"))
        };
        bot = _bot;
    }
    if (strcmp(json_object_get_string(t), "MESSAGE_CREATE") == 0) {
        json_object *output = json_object_object_get(message, "d");
        json_object *_author = json_object_object_get(output, "author");
        struct user author = {
            .id = json_object_get_string(json_object_object_get(_author, "id")),
            .username = json_object_get_string(json_object_object_get(_author, "username")),
            .discriminator = json_object_get_string(json_object_object_get(_author, "discriminator")),
            .avatar = json_object_get_string(json_object_object_get(_author, "avatar")),
            .bot = json_object_get_int(json_object_object_get(_author, "bot")),
            .mfa_enabled = json_object_get_int(json_object_object_get(_author, "mfa_enabled")),
            .verified = json_object_get_int(json_object_object_get(_author, "verified")),
            .email = json_object_get_string(json_object_object_get(_author, "email"))
        };
        struct message msg = {
            .id = json_object_get_string(json_object_object_get(output, "id")),
            .author = author,
            .channel_id = json_object_get_string(json_object_object_get(output, "channel_id")),
            .timestamp = json_object_get_string(json_object_object_get(output, "timestamp")),
            .edited_timestamp = json_object_get_string(json_object_object_get(output, "edited_timestamp")),
            .tts = json_object_get_int(json_object_object_get(output, "tts")),
            .mention_everyone = json_object_get_int(json_object_object_get(output, "mention_everyone")),
            .pinned = json_object_get_int(json_object_object_get(output, "pinned")),
            .webhook_id = json_object_get_string(json_object_object_get(output, "webhook_id")),
            .content = json_object_get_string(json_object_object_get(output, "content"))
        };
        on_discord_message(msg);
    }
    return 0;
}

int on_discord_message(struct message msg) {
    if (strcmp(msg.author.id, bot.id)) {
        if (!strcmp(msg.content, "!hi")) {
            char to_send[SIZE]; 
            snprintf(to_send, sizeof(to_send), "Hi, %s", msg.author.username);
            send_message(to_send, msg.channel_id);
        };
        if (!strcmp(msg.content, "ping")) {
            send_message("ping", msg.channel_id);
        };
    }
}

void on_discord_message_update (struct message msg) {

}

int onopen(wsclient *c) {
    fprintf(stderr, "onopen called: %d\n", c->sockfd);
    char _token[SIZE];
    snprintf(_token, sizeof(_token), "{\"op\":2,\"d\":{\"token\":\"%s\",\"v\":4,\"encoding\":\"etf\",\"properties\":{\"$os\":\"linux\",\"browser\":\"crow\",\"device\":\"crow\",\"referrer\":\"\",\"referring_domain\":\"\"},\"compress\":false,\"large_threshold\":250,\"shard\":[0,1]}}", bot_token);
    json_object *response = json_tokener_parse(_token);
    libwsclient_send(c, json_object_to_json_string(response));
    return 0;
}

int parse_config () {
    dictionary *ini;

    ini = iniparser_load("config.ini");

    if (ini == NULL) {
        printf("Cant find config.ini file!");
        exit(1);
    }

    bot_token = iniparser_getstring(ini, "settings:token", NULL);
    return 0;
}

int main (int argc, char* argv[]) {
    parse_config();
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        set_token(bot_token);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        char* gateway_url = get_gateway_url(curl);
        wsclient *client = libwsclient_new(gateway_url);
        if(!client) {
            fprintf(stderr, "Unable to initialize new WS client\n");
            exit(1);
        }
        libwsclient_onopen(client, &onopen);
        libwsclient_onmessage(client, &onmessage);
        libwsclient_onerror(client, &onerror);
        libwsclient_onclose(client, &onclose);
        libwsclient_run(client);
        fflush(stdout);
        sleep(2);
        while(!done) {
            libwsclient_send(client, "{\"op\": 1, \"d\": null}");
            fflush(stdout);
            sleep(5);
        }
        libwsclient_finish(client);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}