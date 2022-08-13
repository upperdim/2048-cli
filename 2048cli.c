/* 
	2048 game

	_____________________________
	| 2048 | 2048 | 2048 | 2048 |
	|______|______|______|______|
	| 2048 | 2048 | 2048 | 2048 |
	|______|______|______|______|
	| 2048 | 2048 | 2048 | 2048 |
	|______|______|______|______|
	| 2048 | 2048 | 2048 | 2048 |
	|______|______|______|______|
*/

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#endif

#define	VERSION	"0.1"

#define BOARD_ROW_CNT   4
#define BOARD_COL_CNT   4
#define	POS_CNT	(BOARD_ROW_CNT * BOARD_ROW_CNT)

typedef struct {
	int val;
	bool isAvailable;
} cell;

cell board[BOARD_ROW_CNT][BOARD_COL_CNT];
cell backup[BOARD_ROW_CNT][BOARD_COL_CNT];

// Lock for allowing random number generation.
// Used for generating numbers only if an actual game move was made by the user,
// not operations reset and decline confirmation, or exit and decline confirmation
bool gLockNumGen;

#ifdef _WIN32
int get_input() {
	return getch();
}
#else
static struct termios old, current;
/* Initialize new terminal i/o settings */
void initTermios(int echo) { 
	tcgetattr(0, &old);					/* grab old terminal i/o settings */
	current = old;						/* make new settings same as old settings */
	current.c_lflag &= ~ICANON;			/* disable buffered i/o */
	if (echo) {
		current.c_lflag |= ECHO;		/* set echo mode */
	} else {
		current.c_lflag &= ~ECHO;		/* set no echo mode */
	}
	tcsetattr(0, TCSANOW, &current);	/* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) {
	tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) {
	char ch;
	initTermios(echo);
	ch = getchar();
	resetTermios();
	return ch;
}

/* Read 1 character without echo */
int get_input() {
	return getch_(0);
}
#endif

void clear_screen() {
#ifdef _WIN32
	system("cls");
#else
	system("clear");
#endif
}

void print_version() {
	printf("2048 v%s - Author: github.com/upperdim\n"
	       "Compiled on %s, %s\n", VERSION, __DATE__, __TIME__);
}

void print_help() {
	print_version();

	printf("\nControls:\n"
	       "  W, Swipe up\n"
	       "  A, Swipe left\n"
	       "  S, Swipe down\n"
	       "  D, Swipe right\n"
	       "  R, Revert move\n"
	       "  X, Restart game\n"
	       "  E, Exit\n");
	
	printf("\nArguments:\n"
	       "  -h, -H, --help       Prints this help.\n"
	       "  -v, -V, --version    Prints the version of the binary.\n\n");
}

void handle_args(int *argc, char **argv) {
	for (int i = 1; i < *argc; ++i) {
		if      (strcmp("--help",    argv[i]) == 0) {print_help(); exit(EXIT_SUCCESS);}
		else if (strcmp("--version", argv[i]) == 0) {print_version(); exit(EXIT_SUCCESS);}
		else if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'h':
			case 'H':
				print_help();
				exit(EXIT_SUCCESS);
			case 'v':
			case 'V':
				print_version();
				exit(EXIT_SUCCESS);
			default:
				goto undefinedarg;
			}
		} else {
			undefinedarg:
			printf("%s: Unrecognized option '%s'\nTry '%s --help' for more information.\n", argv[0], argv[i], argv[0]);
			exit(EXIT_SUCCESS);
		}
	}
}

void print_menu() {
	printf("\n\n    Swipe with  : W, A, S, D\n"
	           "    Repeat move : R\n"
	           "    Restart game: X\n"
	           "    Exit        : E\n");
}

void print_board() {
	printf("    _____________________________\n");

	for (int i = 0; i < BOARD_ROW_CNT + BOARD_ROW_CNT - 1; ++i)
		if (i % 2)
			printf("    |______|______|______|______|\n");
		else
			printf("    | %4.0d | %4.0d | %4.0d | %4.0d |\n", board[i/2][0].val, board[i/2][1].val, board[i/2][2].val, board[i/2][3].val);
	
	printf("    |______|______|______|______|\n");
}

int get_valid_pos_count() {
	int count = 0;

	for (int i = 0; i < BOARD_ROW_CNT; ++i)
		for (int j = 0; j < BOARD_COL_CNT; ++j)
			if (board[i][j].isAvailable)
				++count;

	return count;
}

// Generates a random number on the board, returns 1 if successful, 0 otherwise
int spawn_random_number() {
	int numOfValids = get_valid_pos_count();

	if (!numOfValids)
		return 0;

	if (gLockNumGen)
		return 1;

	int counter = 0, r = rand() % numOfValids;

	for (int i = 0; i < BOARD_ROW_CNT; ++i) {
		for (int j = 0; j < BOARD_COL_CNT; ++j) {
			if (board[i][j].isAvailable)
				++counter;
			if ((counter - 1) == r) {
				board[i][j].val = rand() % 10 == 1 ? 4 : 2;
				board[i][j].isAvailable = false;
				goto done;
			}
		}
	}
	done:
	gLockNumGen = true;
	return 1;
}

// Save the state of the board so that player can revert back a move
void backup_board() {
	for (int r = 0; r < BOARD_ROW_CNT; ++r)
		for (int c = 0; c < BOARD_COL_CNT; ++c)
			backup[r][c] = board[r][c];
}

void init() {
	srand(time(NULL));

	for (int i = 0; i < BOARD_ROW_CNT; ++i) {
		for (int j = 0; j < BOARD_COL_CNT; ++j) {
			board[i][j].val = 0;
			board[i][j].isAvailable = true;
		}
	}

	spawn_random_number();
	gLockNumGen = false;

	backup_board();
}

int quit_game() {
	printf("\nThanks for playing!\n\n"
	       "~ github.com/upperdim\n");
	exit(EXIT_SUCCESS);
}

int isValidInput(int input) {
	char *validInputs = "wasderx";
	char *curr = validInputs;

	while (*curr != '\0')
		if (tolower(input) == *curr++)
			return 1;

	return 0;
}

int isValidConfirm(int input) {
	return input == 'Y' || input == 'y' || input == 'N' || input == 'n';
}

int confirm(const char *action) {
	printf("\nAre you sure you want to %s? You progress will be lost [Y/N]: ", action);

	int input = get_input();
	int invalidInputCount = 0;
	putchar('\n');

	while (!isValidConfirm(input)) {
		if (!invalidInputCount++)
			printf("Invalid input.\n");
		input = get_input();
	}

	switch (input) {
		case 'Y':
		case 'y':
			return 1;
		case 'N':
		case 'n':
			return 0;
	}

	printf("Error at confirm(): Unreachable code.\n");
	return -1;
}

void make_move() {
	int input = get_input();
	int invalidInputCount = 0;

	while (!isValidInput(input)) {
		if (!invalidInputCount++)
			printf("Invalid input.\n");
		input = get_input();
	}

	if (input != 'r' && input != 'R')
		backup_board();

	bool didMove = false;

	switch (input) {
	case 'W':
	case 'w':
		for (int i = 1; i < BOARD_ROW_CNT; ++i) {
			for (int j = 0; j < BOARD_COL_CNT; ++j) {
				cell *curr = &board[i][j];
				if (!curr->isAvailable) {
					bool done = false;
					int aboveRow = i - 1;
					cell *above = &board[aboveRow][j];
					while (!done && aboveRow >= 0) {
						if (above->isAvailable) {
							// Above cell is empty
							above->val = curr->val;
							curr->val = 0;
							above->isAvailable = false;
							curr->isAvailable = true;
							curr = above;
							above = &board[--aboveRow][j];
							didMove = true;
						} else if (above->val == curr->val) {
							// Above cell is occupied
							above->val *= 2;
							curr->val = 0;
							above->isAvailable = false;
							curr->isAvailable = true;
							didMove = true;
						} else {
							done = true;
						}
					}
				}
			}
		}
		break;
	case 'S':
	case 's':
		for (int i = 2; i >= 0; --i) {
			for (int j = 0; j < BOARD_COL_CNT; ++j) {
				cell *curr = &board[i][j];
				if (!curr->isAvailable) {
					bool done = false;
					int belowRow = i + 1;
					cell *below = &board[belowRow][j];
					while (!done && belowRow < BOARD_ROW_CNT) {
						if (below->isAvailable) {
							below->val = curr->val;
							curr->val = 0;
							below->isAvailable = false;
							curr->isAvailable = true;
							curr = below;
							below = &board[++belowRow][j];
							didMove = true;
						} else if (below->val == curr->val) {
							below->val *= 2;
							curr->val = 0;
							below->isAvailable = false;
							curr->isAvailable = true;
							didMove = true;
						} else {
							done = true;
						}
					}
				}
			}
		}
		break;
	case 'D':
	case 'd':
		for (int c = BOARD_COL_CNT - 2; c >= 0; --c) {
			for (int r = 0; r < BOARD_ROW_CNT; ++r) {
				cell *curr = &board[r][c];
				if (!curr->isAvailable) {
					bool done = false;
					int rightCol = c + 1;
					cell *right = &board[r][rightCol];
					while (!done && rightCol < BOARD_COL_CNT) {
						if (right->isAvailable) {
							right->val = curr->val;
							curr->val = 0;
							right->isAvailable = false;
							curr->isAvailable = true;
							curr = right;
							right = &board[r][++rightCol];
							didMove = true;
						} else if (right->val == curr->val) {
							right->val *= 2;
							curr->val = 0;
							right->isAvailable = false;
							curr->isAvailable = true;
							didMove = true;
						} else {
							done = true;
						}
					}
				}
			}
		}
		break;
	case 'A':
	case 'a':
		for (int c = 1; c < BOARD_COL_CNT; ++c) {
			for (int r = 0; r < BOARD_ROW_CNT; ++r) {
				cell *curr = &board[r][c];
				if (!curr->isAvailable) {
					bool done = false;
					int leftCol = c - 1;
					cell *left = &board[r][leftCol];
					while (!done && leftCol >= 0) {
						if (left->isAvailable) {
							left->val = curr->val;
							curr->val = 0;
							left->isAvailable = false;
							curr->isAvailable = true;
							curr = left;
							left = &board[r][--leftCol];
							didMove = true;
						} else if (left->val == curr->val) {
							left->val *= 2;
							curr->val = 0;
							left->isAvailable = false;
							curr->isAvailable = true;
							didMove = true;
						} else {
							done = true;
						}
					}
				}
			}
		}
		break;
	case 'R':
	case 'r':
		// Revert move
		for (int r = 0; r < BOARD_ROW_CNT; ++r)
			for (int c = 0; c < BOARD_COL_CNT; ++c)
				board[r][c] = backup[r][c];
		break;
	case 'X':
	case 'x':
		// Restart game
		if (confirm("restart")) {
			gLockNumGen = false;
			init();
		}
		break;
	case 'E':
	case 'e':
		if (confirm("exit"))
			quit_game();
		break;
	default:
		break;
	}

	if (didMove)
		gLockNumGen = false;
}

int main(int argc, char *argv[]) {
	handle_args(&argc, argv);
	init();

	for (;;) {
		if (!spawn_random_number()) {
			printf("\nGame Over!\n");
			break;
		}

		clear_screen();
		print_menu();
		print_board();
		make_move();
	}

	quit_game();
}
