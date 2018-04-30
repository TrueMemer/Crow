#ifndef _EVENTS_H_
#define _EVENTS_H_

#include <json-c/json.h>

#include <crow/client.h>

void dispatch(client_t *bot, json_object *data);

#endif // _EVENTS_H_