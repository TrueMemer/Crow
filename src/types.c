#include "../include/types.h"

#include <json.h>

user_t user(json_object *raw) {
    user_t u;
    u.id = json_object_get_string(json_object_object_get(raw, "id"));
    u.username = json_object_get_string(json_object_object_get(raw, "username"));
    u.discriminator = json_object_get_string(json_object_object_get(raw, "discriminator"));
    u.avatar = json_object_get_string(json_object_object_get(raw, "avatar"));
    u.bot = json_object_get_int(json_object_object_get(raw, "bot"));
    u.mfa_enabled = json_object_get_int(json_object_object_get(raw, "mfa_enabled"));
    u.verified = json_object_get_int(json_object_object_get(raw, "verified"));
    u.email = json_object_get_string(json_object_object_get(raw, "email"));
    return u;
}

message_t message(json_object *raw) {
    message_t msg;

    msg.id = json_object_get_string(json_object_object_get(raw, "id"));
    msg.author = user(json_object_object_get(raw, "author"));
    msg.channel_id = json_object_get_string(json_object_object_get(raw, "channel_id"));
    msg.timestamp = json_object_get_string(json_object_object_get(raw, "timestamp"));
    msg.edited_timestamp = json_object_get_string(json_object_object_get(raw, "edited_timestamp"));
    msg.tts = json_object_get_int(json_object_object_get(raw, "tts"));
    msg.mention_everyone = json_object_get_int(json_object_object_get(raw, "mention_everyone"));
    msg.pinned = json_object_get_int(json_object_object_get(raw, "pinned"));
    msg.webhook_id = json_object_get_string(json_object_object_get(raw, "webhook_id"));
    msg.content = json_object_get_string(json_object_object_get(raw, "content"));

    return msg;
}