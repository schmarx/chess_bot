#include "../../chess_engine/src/globals.hh"

#include <algorithm>
#include <climits>
#include <stdlib.h>

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

namespace chess {

class move_score {
  public:
	int score;
	Move move;
};

bool sorter(move_score a, move_score b) {
	return a.score > b.score;
}

class bot {
  public:
	int client;
	bool running = true;

	Board board;

	bool register_listen() {
		client = socket(AF_INET, SOCK_STREAM, 0);
		sockaddr_in addr;

		addr.sin_port = htons(PORT);
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_family = AF_INET;

		int conn = connect(client, (sockaddr *)&addr, sizeof(addr));

		if (conn < 0) {
			printf("err\n");
			return false;
		}

		fcntl(client, F_SETFL, O_NONBLOCK);

		return true;

		// printf("continue?");
		// char c;
		// scanf("%c", &c);

		// const char *msg = "sending message beep boop";
		// send(client, msg, strlen(msg), 0);
		// printf("sending \"%s\" of length %li\n", msg, strlen(msg));
	}

	void listen() {
		int buf[TCP_BUF_LEN];
		int res = recv(client, buf, sizeof(packet), 0);

		if (res == 0) {
			// inactive connection
			printf("connection lost\n");
			running = false;

		} else if (res < 0) {
			// error occured

			if (no_msg()) {
				// the error is only signifying that there are no packets to poll for
			} else {
				printf("err\n");
			}
		} else {
			// a response consisting of res bytes is received
			printf("received board\n");
			packet_to_board(board, *((packet *)buf));
			play();
		}
	}

	int stage(Move move) {
		Board staging_board = board;
		staging_board.stage = false; // the staging board should not recursively stage more boards
		staging_board.commit(move);
		staging_board.next_turn();
		return staging_board.get_score();

		// std::vector<Move> moves = staging_board.get_valid_moves();

		// // check if the move allows the opponent to take the king
		// for (size_t i = 0; i < moves.size(); i++) {
		// 	if (staging_board[moves[i].end].type == PIECE_KING) return false;
		// }

		// return true;
	}

	int max(int v1, int v2) {
		if (v1 > v2) return v1;
		return v2;
	}

	int min(int v1, int v2) {
		if (v1 < v2) return v1;
		return v2;
	}

	int heuristic(Board &board) {
		return board.get_score() * 1000 + board.protected_pieces();
	}

	int ab(Board &board, int a, int b, int depth) {
		// beta minimizes, alpha maximizes

		if (depth == 0) {
			return heuristic(board);
		}

		int score;
		std::vector<Move> moves = board.get_valid_moves();

		if (board.player_turn == WHITE) {
			score = INT_MIN;

			for (size_t i = 0; i < moves.size(); i++) {
				Board result = board;
				result.stage = false;
				result.commit(moves[i]);
				result.next_turn();

				score = max(score, ab(result, a, b, depth - 1));
				if (score >= b) break;
				a = max(a, score);
			}
			return score;
		} else {
			score = INT_MAX;

			for (size_t i = 0; i < moves.size(); i++) {
				Board result = board;
				result.stage = false;
				result.commit(moves[i]);
				result.next_turn();

				score = min(score, ab(result, a, b, depth - 1));
				if (score <= a) break;
				b = min(b, score);
			}
		}

		return score;
	}

	void play() {
		if (board.player_turn == board.me) {
			std::vector<Move> moves = board.get_valid_moves();

			if (moves.size() == 0) {
				running = false;
				printf("lost\n");
				return;
			}

			// std::vector<move_score> move_scores;
			// for (size_t i = 0; i < moves.size(); i++) {
			// 	move_score ms;
			// 	ms.move = moves[i];
			// 	ms.score = stage(moves[i]);
			// 	move_scores.push_back(ms);
			// }

			// std::sort(move_scores.begin(), move_scores.end(), sorter);

			int high = 0;
			int high_score = INT_MIN;
			int low = 0;
			int low_score = INT_MAX;
			for (size_t i = 0; i < moves.size(); i++) {
				Board result = board;
				result.stage = false;
				result.commit(moves[i]);
				result.next_turn();
				int score = ab(result, INT_MIN, INT_MAX, 4);

				if (score > high_score) {
					high = i;
					high_score = score;
				}
				if (score < low_score) {
					low = i;
					low_score = score;
				}
			}

			int best = high;
			if (board.me == BLACK) best = low;

			char buf[20];
			moves[best].to_string(buf);
			send(client, buf, 20, 0);

			// printf("best:  %s (%i)\n", buf, best_score);
			// moves[worst].to_string(buf);
			// printf("worst: %s (%i)\n", buf, worst_score);
		}
	}

	void run() {
		while (running) {
			listen();
		}
	}
};

} // namespace chess