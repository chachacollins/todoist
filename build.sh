set -uex

gcc -O3 todoist.c -Wall -Wextra -Wunused-result -Wpedantic -Werror -std=c11 -lm -lsqlite3 -o td
