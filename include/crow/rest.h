#ifndef _REST_H_
#define _REST_H_

#include <curl/curl.h>
#include <json-c/json.h>

#include <crow/types.h>

#define DISCORD_API_GUILDS "https://discordapp.com/api/guilds/"
#define DISCORD_API_CHANNELS "https://discordapp.com/api/channels/"
#define DISCORD_API_USERS "https://discordapp.com/api/users/"
#define DISCORD_API_GATEWAY "https://discordapp.com/api/gateway"
#define DISCORD_API_BOT_GATEWAY "https://discordapp.com/api/gateway/bot"
#define DISCORD_API_WEBHOOKS "https://discordapp.com/api/webhooks/"

#define AUTH_HEADER "Authorization: Bot "

void send_message(char *channel_id, char *text);
void add_reaction(char* channel_id, char *message_id, char *emoji);
guild_channel_t get_channel(char *channel_id);
user_t get_user(char *user_id);

#endif