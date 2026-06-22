#include <stdlib.h>

#include "bot.hh"

int main(int argc, char *argv[]) {
	chess::bot bot = chess::bot();

	if (!bot.register_listen()) return EXIT_FAILURE;
	bot.run();

	return EXIT_SUCCESS;
}