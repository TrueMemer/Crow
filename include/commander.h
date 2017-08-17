#ifndef _COMMANDER_H_
#define _COMMANDER_H_

#include "types.h"
#include "rest.h"
#include "client.h"

void on_discord_message(client_t *bot, message_t msg);
void on_presence_update(presence_update_t upd);

#endif