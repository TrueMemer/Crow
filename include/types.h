#ifndef _TYPES_H_
#define _TYPES_H_

#include "../deps/libwsclient/wsclient.h"

#include <unistd.h>
#include <json.h>

enum game_types {
    GAME = 0,
    STREAMING = 1
};

enum channel_types {
    TEXT = 0,
    VOICE = 1
};

enum {
    DISPATCH,
    HEARTBEAT,
    IDENTIFY,
    PRESENCE,
    VOICE_STATE,
    VOICE_PING,
    RESUME,
    RECONNECT,
    REQUEST_MEMBERS,
    INVALIDATE_SESSION,
    HELLO,
    HEARTBEAT_ACK
};

typedef struct curl_fetch_st {
    char *payload;
    size_t size;
} curl_fetch_t;

typedef struct guild_channel {
    char *id;
    char *guild_id;
    char *name;
    int type;
    int position;
    int is_private;
    /* TODO permission_overwrites */
    char *topic;
    char *last_message_id;
    int bitrate;
    int user_limit;
} guild_channel_t;

typedef struct thumbnail {
    char* url;
    char* proxy_url;
    int height;
    int width;
} thumbnail_t;

typedef struct video {
    char* url;
    int height;
    int width;
} video_t;

typedef struct image {
    char* url;
    char* proxy_url;
    int height;
    int width;
} image_t;

typedef struct provider {
    char* name;
    char* url;
} provider_t; 

typedef struct author {
    char* name;
    char* url;
    char* icon_url;
    char* proxy_icon_url;
} author_t;

typedef struct footer {
    char* text;
    char* icon_url;
    char* proxy_icon_url;
} footer_t;

typedef struct field {
    char* name;
    char* value;
    int _inline;
} field_t;

typedef struct embed {
    char* title;
    char* type;
    char* description;
    char* url;
    char* timestamp;
    int color;
    footer_t footer;
    image_t image;
    thumbnail_t thumbnail;
    video_t video;
    provider_t provider;
    author_t author;
    field_t fields;
} embed_t;

typedef struct user {
    char* id;
    char* username;
    char* discriminator;
    char* avatar;
    int bot;
    int mfa_enabled;
    int verified;
    char* email;
} user_t;

typedef struct dm_channel {
    char *id;
    int is_private;
    user_t recipient;
    char *last_message_id;
} dm_channel_t;

typedef struct game {
    char *name;
    int type;
    char *url; // Only if type is STREAMING (1)
} game_t;

typedef struct presence_update {
    user_t user; // Can be partial
    game_t game; // May be null
    char *guild_id;
    char *status;
    char *roles[200];
} presence_update_t;

typedef struct emoji {
    char* id;
    char* name;
} emoji_t;

typedef struct reaction {
    int counter;
    int me;
    emoji_t _emoji;
} reaction_t;

typedef struct message {
    char* id;
    char* channel_id;
    user_t author;
    char* content;
    char* timestamp;
    char* edited_timestamp;
    int tts;
    int mention_everyone;
    user_t mentions[1024];
    /*
    TODO:
    mention_roles
    attachments
    embeds
    nonce
    */
    embed_t embeds[1024];
    reaction_t reactions[1024];
    int pinned;
    char* webhook_id;
} message_t;

user_t user(json_object *raw);
message_t message(json_object *raw);

#endif // TYPES_H_