/*
 * mind.c - A game that might be similiar to Mastermind.
 *
 * Copyright (C) 2011-2013  Jonathan Neuschäfer <j.neuschaefer@gmx.net>
 *     Please read LICENSE.
 *
 * Rules:
 * you get a number of chances to solve this riddle.
 * you will first be promted for four numbers, then the game
 * will show you whether they where correct (green), wrong, but
 * somewhere else (yellow), or totally wrong (red). The game
 * will choose numbers from zero to nine.
 * example for the numbers 2 3 4 0:
 *  1. 0 3 5 3  Y G R Y
 *  2. 1 2 4 6  R Y G R
 *  3. 2 3 4 0  G G G G
 */

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#define NCHARS 4

struct colors {
	const char *red;
	const char *yellow;
	const char *green;
};

static struct colors colors_mono = {
	.red	= "R",
	.yellow	= "Y",
	.green	= "G"
};

static struct colors colors_dark = {
	.red	= "\033[0;31m" "R" "\033[m",
	.yellow	= "\033[0;33m" "Y" "\033[m",
	.green	= "\033[0;32m" "G" "\033[m"
};

static struct colors colors_light = {
	.red	= "\033[1;31m" "R" "\033[m",
	.yellow	= "\033[1;33m" "Y" "\033[m",
	.green	= "\033[1;32m" "G" "\033[m"
};

static struct colors colors_grey = {
	.red	= "\033[1;30m" "R" "\033[m",
	.yellow	= "\033[0;37m" "Y" "\033[m",
	.green	= "\033[1;37m" "G" "\033[m"
};

static struct colors *colors;

static char colormode = 'a';

static struct colors *get_colors(void)
{
	switch (colormode) {
	case 'm': return &colors_mono;
	case 'd': return &colors_dark;
	case 'l': return &colors_light;
	case 'g': return &colors_grey;
	}
	fprintf(stderr, "invalid color mode, falling back to mono\n");
	return &colors_mono;
}

static struct termios saved_tios;
static int isterm;

static void init_term(void)
{
	struct termios tios;

	if (tcgetattr(0, &tios) == -1) {
		if (errno != ENOTTY)
			perror("tcgetaddr returned");
	} else {
		isterm = 1;
		saved_tios = tios;
	}

	if (isterm) {
		tios.c_lflag &= ~(ICANON|ECHO);

		tcsetattr(0, TCSANOW, &tios);
	}

	/* check chosen mode */
	if (colormode == 'a')
		colormode = isterm? 'l':'m';

	/* set colors */
	colors = get_colors();
}

static void exit_term(void)
{
	/* undo all the stuff */
	if (isterm) {
		tcsetattr(0, TCSANOW, &saved_tios);
	}
}

static int chances;

static void parse_args(int argc, char **argv)
{
	int opt;

	opterr = 0;
	while (1) {
		opt = getopt(argc, argv, "C:c:");
		if (opt == -1)
			break;
		switch (opt) {
		case 'C':
			colormode = tolower(optarg[0]);
			break;
		case 'c':
			chances = atoi(optarg);
			break;
		default:
			fprintf(stderr, "usage: %s [-c <chances>] "
					"[-C <color>]\n", argv[0]);
			exit((opt == 'h')? EXIT_SUCCESS : EXIT_FAILURE);
		}
	}
}

static int is_valid(char c) {
	return (c >= '0' && c <= '9');
}

static char answer[NCHARS];

/* needs to produce the same characters that is_valid considers valid */
static void mkanswer(void)
{
	int i;

	/* since this is the only function using rand(), it
	 * should initialize the generator */
	srand(time(NULL));

	for (i = 0; i < NCHARS; i++) {
		answer[i] = '0' + (rand() % ('9'-'0'));
	}
}

static void choice_back(int n)
{
	int i;

	/* from bash(1) */
	for (i = 0; i < n; i++) {
		putchar('\010');
		putchar('\010');
	}
	printf("\033[K");
}

static void get_choice(char *choice)
{
	/* stuff */
	int pos = 0, ch;

	while (1) {
		ch = getchar();
		if (is_valid(ch) && pos < NCHARS) {
			choice[pos] = ch;
			putchar(ch);
			putchar(' ');
			pos++;
		} else if (ch == '\n' || ch == '\r') {
			if (pos == NCHARS)
				break;
		} else if (iscntrl(ch)) {
			if (ch == 127 && pos) { /* backspace */
				choice_back(1);
				--pos;
			} else if (ch == 21 && pos) { /* ^u */
				choice_back(pos);
				pos = 0;
			}	
		}
	}
}

/* returns 1 if everything is correct, else 0 */
static int print_test(char *choice)
{
	int i, j, right = 0;
	const char *match;

	putchar(' ');
	for (i = 0; i < NCHARS; i++) {
		match = colors->red;
		if (choice[i] == answer[i]) {
			match = colors->green;
			right++;
		} else for (j = 0; j < NCHARS; j++) {
			if (j == i)
				continue;
			if (choice[i] == answer[j])
				match = colors->yellow;
		}
		printf("%s ", match);
	}
	putchar('\n');
	return right == NCHARS;
}

int main(int argc, char **argv)
{
	int i, done = 0;
	char choice[NCHARS];

	parse_args(argc, argv);

	if (!chances)
		chances = 10;

	atexit(exit_term);

	init_term();

	mkanswer();
	
	for (i = 1; i <= chances && !done; i++) {
		printf(" %2d. ", i);
		get_choice(choice);
		done = print_test(choice);
	}

	if (!done) {
		printf(" answer:");
		for (i = 0; i < NCHARS; i++)
			printf(" %c", answer[i]);
		putchar('\n');
	}

	return EXIT_SUCCESS;
}
