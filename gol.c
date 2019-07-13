#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#define LIVING_CELL 1
#define DEAD_CELL 0
#define LIVING_CELL_CHAR '#'
#define DEAD_CELL_CHAR ' '

#define STUCK_LIMIT 10000
#define HISTORY_SIZE 32


struct BoardHistory {
	size_t data[HISTORY_SIZE];
	int index;
};

struct Board {
	char** data;
	char** prev_data;
	int width;
	int height;
	long tick;
	long id;
	struct BoardHistory data_history;
};


// Bernstein hash function
static size_t djb_hash(struct Board *b)
{
	int row, col;
    size_t hash = 0;
	for (row = 0; row < b->height; row++)
		for (col = 0; col < b->width; col++) {
		hash = 33 * hash ^ (unsigned char) b->data[row][col];
	}
    return hash;
}

long long get_millis(void) {
	struct timeval te;
	gettimeofday(&te, NULL);
	long long m = (te.tv_sec * 1000LL) + (te.tv_usec/1000);
	return m;
}

char **alloc_board_data(int width, int height) {
	int i;
	char **data = malloc(height * sizeof(char *));
	for (i = 0; i < height; i++)
		data[i] = malloc(width * sizeof(char));
	return data;	
}

void clear_board(char** data, int width, int height) {
	int i;
	for (i = 0; i < height; i++) {
		memset(data[i], 0, width);
	}
}

void init_board_random(struct Board *board, int chance) {
	int row, col;
	for (row = 0; row < board->height; row++) {
		for (col = 0; col < board->width; col++) {
			if (rand() % chance == 0) {
				board->data[row][col] = LIVING_CELL;
			}
		}
	}
}

struct Board* new_board(int width, int height, int chance, long id) {
	struct Board *b = malloc(sizeof(struct Board));
	b->tick = 0;
	b->id = id;
	b->width = width;
	b->height = height;
	b->data = alloc_board_data(width, height);
	b->prev_data = alloc_board_data(width, height);
	b->data_history.index = 0;
	clear_board(b->data, width, height);
	init_board_random(b, chance);
	return b;
}


void delete_board(struct Board *b) {
	int i;
	for (i = 0; i < b->height; i++)
		free(b->data[i]);
	free(b->data);
	free(b);
}

void display_board(struct Board *b) {
	int row, col;
	char *str = malloc(b->width + 1);
	for (row = 0; row < b->height; row++) {
		memset(str, DEAD_CELL_CHAR, b->width + 1);
		for (col = 0; col < b->width; col++) {
			str[col] = b->data[row][col] == LIVING_CELL ? LIVING_CELL_CHAR : DEAD_CELL_CHAR;
		}
		str[col] = '\0';
		printf("%s\n", str);
	}
	printf("stats: %ld/%ld\n", b->tick, b->id);
}

int is_alive(struct Board *b, int i, int j) {
	if (i < 0) i = b->height - 1;
	else if (i >= b->height) i = 0;
	
	if (j < 0) j = b->width - 1;
	else if (j >= b->width) j = 0;

//	if (i < 0 || j < 0 || i >= b->height || j >= b->width)
//		return 0;

	return b->data[i][j] == LIVING_CELL;
}

int count_living_neighbours(struct Board *b, int row, int col) {
	int n = 0;
	if (is_alive(b, row - 1, col - 1)) 
		n++;	
	if (is_alive(b, row - 1, col)) 
		n++;	
	if (is_alive(b, row - 1, col + 1)) 
		n++;	
	if (is_alive(b, row, col - 1))
		n++;	
	if (is_alive(b, row, col + 1))
		n++;	
	if (is_alive(b, row + 1, col - 1))
		n++;	
	if (is_alive(b, row + 1, col))
		n++;	
	if (is_alive(b, row + 1, col + 1))
		n++;	

	return n;
}

void transform(struct Board *b) {
	int row, col;
	int n;
	int new_state;
	char **swp;
	clear_board(b->prev_data, b->width, b->height);
	for (row = 0; row < b->height; row++) {
		for (col = 0; col < b->width; col++) {
			n = count_living_neighbours(b, row, col);
			//printf("Cell (%d,%d) has %d neighbours\n", row, col, n);
			new_state = b->data[row][col];
			if (is_alive(b, row, col)) {
				if (n < 2 || n > 3) { //underpopulation or overpopulation
					new_state = DEAD_CELL;
				} else { // next generation
				}
			} else if (n == 3) { //reproduction
				new_state = LIVING_CELL;	
			}
			b->prev_data[row][col] = new_state;
		}
	}
	b->data_history.data[b->data_history.index] = djb_hash(b);
	b->data_history.index = (b->data_history.index + 1) % HISTORY_SIZE;

	swp = b->data;
	b->data = b->prev_data;
	b->prev_data = swp;

	b->tick++;
}

int is_stuck(struct Board *b) {
	int i;
	int eq = 1;
	for (i = 0; eq && i < HISTORY_SIZE-2; i+=2) {
		eq = eq && b->data_history.data[i] == b->data_history.data[i+2];
	}
	for (i = 1; eq && i < HISTORY_SIZE-2; i+=1) {
		eq = eq && b->data_history.data[i] == b->data_history.data[i+2];
	}	
	return eq;
}


int main(int argc, char **argv) {

	long tick;
	int board_width;
	int board_height;
	int chance;
	long id = 0;
	struct timespec sleep_arg;
	struct Board *board;
	sleep_arg.tv_sec = 0;
	sleep_arg.tv_nsec = 500;

	srand(time(NULL));

	if (argc == 5) {
		board_width = atoi(argv[1]);
		board_height = atoi(argv[2]);
		tick = atol(argv[3]); 
		sleep_arg.tv_sec = tick / 1000;
		sleep_arg.tv_nsec = (tick % 1000) * 1000000L; 	
		printf("%ld %ld\n", sleep_arg.tv_sec, sleep_arg.tv_nsec);
		chance = atoi(argv[4]);
		board = new_board(board_width, board_height, chance, id++);
	} else {
		printf("Run %s <width> <height> <tick ms> <init chance>\n", argv[0]);
		return 1;
	}

	for (;;) {
		printf("\e[1;1H\e[2J");
		display_board(board);
		transform(board);
		if (is_stuck(board)) {
			delete_board(board);
			board = new_board(board_width, board_height, chance, id++);
		}
		nanosleep(&sleep_arg, NULL);
	}

}
