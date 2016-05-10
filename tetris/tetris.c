// Copyright (c) 1992 The Regents of the University of California.
// This file is free software, distributed under the BSD license.
//
// Tetris (or however it is spelled).

#include "input.h"
#include "scores.h"
#include "screen.h"
#include "tetris.h"
#include <sys/time.h>
#include <err.h>
#include <signal.h>

cell board[B_SIZE];		// 1 => occupied, 0 => empty

int Rows, Cols;			// current screen size

const struct shape *curshape;
const struct shape *nextshape;

long fallrate;			// less than 1 million; smaller => faster

int score;			// the obvious thing
gid_t gid, egid;

char key_msg[100];
int showpreview;

static void elide(void);
static void setup_board(void);
int main(int, char **);
void onintr(int);
void usage(void);

// Set up the initial board.  The bottom display row is completely set,
// along with another (hidden) row underneath that.  Also, the left and
// right edges are set.
static void setup_board(void)
{
    cell* p = board;
    for (int i = B_SIZE; i; i--)
	*p++ = i <= (2 * B_COLS) || (i % B_COLS) < 2;
}

// Elide any full active rows.
static void elide(void)
{
    for (int i = A_FIRST; i < A_LAST; ++i) {
	int base = i * B_COLS + 1;
	cell* p = &board[base];
	for (int j = B_COLS - 2; *p++ != 0;) {
	    if (--j <= 0) {
		// this row is to be elided
		memset(&board[base], 0, B_COLS - 2);
		scr_update();
		tsleep();
		while (--base != 0)
		    board[base + B_COLS] = board[base];
		scr_update();
		tsleep();
		break;
	    }
	}
    }
}

int main(int argc, char *argv[])
{
    int pos, c;
    const char *keys;
    int level = 2;
    char key_write[6][10];
    int ch, i, j;
    int fd;

    gid = getgid();
    egid = getegid();
    setegid(gid);

    fd = open("/dev/null", O_RDONLY);
    if (fd < 3)
	return EXIT_FAILURE;
    close(fd);

    keys = "jkl pq";

    while ((ch = getopt(argc, argv, "k:l:ps")) != -1)
	switch (ch) {
	    case 'k':
		if (strlen(keys = optarg) != 6)
		    usage();
		break;
	    case 'l':
		level = atoi(optarg);
		if (level < MINLEVEL || level > MAXLEVEL) {
		    errx(1, "level must be from %d to %d", MINLEVEL, MAXLEVEL);
		}
		break;
	    case 'p':
		showpreview = 1;
		break;
	    case 's':
		showscores(0);
		return EXIT_SUCCESS;
	    case '?':
	    default:
		usage();
	}

    argc -= optind;
    argv += optind;

    if (argc)
	usage();

    fallrate = 1000000 / level;

    for (i = 0; i <= 5; i++) {
	for (j = i + 1; j <= 5; j++) {
	    if (keys[i] == keys[j]) {
		errx(1, "duplicate command keys specified.");
	    }
	}
	if (keys[i] == ' ')
	    strcpy(key_write[i], "<space>");
	else {
	    key_write[i][0] = keys[i];
	    key_write[i][1] = '\0';
	}
    }

    sprintf(key_msg, "%s - left   %s - rotate   %s - right   %s - drop   %s - pause   %s - quit", key_write[0], key_write[1], key_write[2], key_write[3], key_write[4], key_write[5]);

    signal(SIGINT, onintr);
    scr_init();
    setup_board();

    srand(getpid());
    scr_set();

    pos = A_FIRST * B_COLS + (B_COLS / 2) - 1;
    nextshape = randshape();
    curshape = randshape();

    scr_msg(key_msg, 1);

    for (;;) {
	place(curshape, pos, 1);
	scr_update();
	place(curshape, pos, 0);
	c = tgetchar();
	if (c < 0) {
	    // Timeout.  Move down if possible.
	    if (fits_in(curshape, pos + B_COLS)) {
		pos += B_COLS;
		continue;
	    }

	    // Put up the current shape `permanently',
	    // bump score, and elide any full rows.
	    place(curshape, pos, 1);
	    score++;
	    elide();

	    // Choose a new shape.  If it does not fit,
	    // the game is over.
	    curshape = nextshape;
	    nextshape = randshape();
	    pos = A_FIRST * B_COLS + (B_COLS / 2) - 1;
	    if (!fits_in(curshape, pos))
		break;
	    continue;
	}

	// Handle command keys.
	if (c == keys[5]) {
	    // quit
	    break;
	}
	if (c == keys[4]) {
	    static char msg[] = "paused - press RETURN to continue";

	    place(curshape, pos, 1);
	    do {
		scr_update();
		scr_msg(key_msg, 0);
		scr_msg(msg, 1);
		(void) fflush(stdout);
	    } while (rwait((struct timeval *) NULL) == -1);
	    scr_msg(msg, 0);
	    scr_msg(key_msg, 1);
	    place(curshape, pos, 0);
	    continue;
	}
	if (c == keys[0]) {
	    // move left
	    if (fits_in(curshape, pos - 1))
		pos--;
	    continue;
	}
	if (c == keys[1]) {
	    // turn
	    const struct shape *new = &shapes[curshape->rot];

	    if (fits_in(new, pos))
		curshape = new;
	    continue;
	}
	if (c == keys[2]) {
	    // move right
	    if (fits_in(curshape, pos + 1))
		pos++;
	    continue;
	}
	if (c == keys[3]) {
	    // move to bottom
	    while (fits_in(curshape, pos + B_COLS)) {
		pos += B_COLS;
		score++;
	    }
	    continue;
	}
	if (c == '\f') {
	    scr_clear();
	    scr_msg(key_msg, 1);
	}
    }

    scr_clear();
    scr_end();

    printf("Your score:  %d point%s  x  level %d  =  %d\n", score, score == 1 ? "" : "s", level, score * level);
    savescore(level);

    printf("\nHit RETURN to see high scores, ^C to skip.\n");

    while ((i = getchar()) != '\n')
	if (i == EOF)
	    break;

    showscores(level);
    return EXIT_SUCCESS;
}

void onintr (int signo UNUSED)
{
    scr_clear();
    scr_end();
    exit (EXIT_SUCCESS);
}

void usage(void)
{
    fprintf(stderr, "usage: tetris-bsd [-ps] [-k keys] [-l level]\n");
    exit (EXIT_SUCCESS);
}
