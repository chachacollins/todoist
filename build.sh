set -uex

gcc -O3 todoist.c -Wall -Wextra -Wunused-result -Wpedantic -Werror -std=c11 -o td
