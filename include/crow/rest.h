#ifndef _REST_H_
#define _REST_H_

#include <curl/curl.h>
#include <json.h>

#include "types.h"

void send_message(char *channel_id, char *text);
void add_reaction(char* channel_id, char *message_id, char *emoji);
guild_channel_t get_channel(char *channel_id);

#endif