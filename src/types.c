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