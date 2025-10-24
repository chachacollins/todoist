#include <stdio.h>
#include <assert.h>

#define NODISCARD __attribute__((warn_unused_result))
#define UNUSED(var) (void)(var);
#define EPRINT(fmt, ...) fprintf(stderr, "ERROR:"fmt"\n", __VA_ARGS__)

typedef NODISCARD int (*comm_fn)(int *argc, char** argv);

typedef struct {
    const char* s_flag;
    const char* l_flag;
    const char* help;
    comm_fn fn;
} Command;

#define COMMAND_CAPACITY 16
typedef struct {
    Command commands[COMMAND_CAPACITY];
    short size;
} Commands;

static Commands commands;

NODISCARD 
static int add_command(Commands* comms, Command command)
{
    assert(comms != NULL);
    if(comms->size >= COMMAND_CAPACITY)
    {
        EPRINT("Commands added exceed maximum capacity: %d\n", COMMAND_CAPACITY);
        return 0;
    }
    comms->commands[comms->size++] = command;
    return 1;
}

NODISCARD
static const char* shift_args(int *argc, char** argv)
{
    if(*argc < 1) return NULL;
    *argc -= 1;
    return *argv++;
}

NODISCARD  
static int usage(int *argc, char** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    printf("Usage: td [options]\n\n");
    printf("Yet another TODO App\n\n");
    printf("Options:\n");
    for(int i = 0; i < commands.size; ++i)
    {
        Command command = commands.commands[i];
        assert(command.s_flag != NULL);
        assert(command.l_flag != NULL);
        assert(command.help != NULL);
        printf("    %s, %-10s  %s\n", command.s_flag, command.l_flag, command.help);
    }
    return 1;
}

int main(int argc, char** argv)
{
    (void)argv;
    Command help =  {
        "-h", 
        "help", 
        "prints this help message and quits",
        usage,
    };
    Command add = {
        "-a",
        "add",
        "adds a todo",
        NULL,
    };
    if(!add_command(&commands, add)) return 1;
    if(!add_command(&commands, help)) return 1;
    if(argc < 2)
    {
        if(!usage(&argc, argv)) return 1;
        return 1;
    }
    return 0;
}
