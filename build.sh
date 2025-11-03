set -uex

gcc -O3 todoist.c -ggdb -Wall -Wextra -Wunused-result -Wpedantic -Werror -std=c11 -lm -Wl,-Bstatic -lsqlite3 -Wl,-Bdynamic -o td
cp ./td ~/.local/bin
