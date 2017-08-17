#include "../include/client.h"

int main(void) {
	client_t bot;

	client_init(&bot);

	client_run(&bot);

	return 0;
}