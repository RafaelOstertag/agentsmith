#include <stdio.h>

int
main(int argc, char** argv) {
    if (argc != 2) {
	fprintf(stderr, "Please specify file to open\n");
	exit (1);
    }

    follow(argv[1]);
    exit (0);
}
