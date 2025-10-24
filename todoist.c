#include <stdio.h>
#include <string.h>
#include <assert.h>

#define NODISCARD __attribute__((warn_unused_result))
#define UNUSED(var) (void)(var);
#define EPRINT(fmt) fprintf(stderr, "ERROR: " fmt "\n")
#define EPRINTF(fmt, ...) fprintf(stderr, "ERROR:" fmt "\n", __VA_ARGS__)
#define HELP(fmt) fprintf(stderr, "USAGE: "fmt"\n")
#define TODOS_FILE "todos.txt"

typedef NODISCARD int (*comm_fn)(int *argc, char*** argv);

typedef struct {
    const char* s_flag;
    const char* l_flag;
    const char* desc;
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
        EPRINTF("Commands added exceed maximum capacity: %d\n", COMMAND_CAPACITY);
        return 0;
    }
    comms->commands[comms->size++] = command;
    return 1;
}

NODISCARD
static const char* shift_args(int *argc, char*** argv)
{
    if(*argc < 1) return NULL;
    *argc -= 1;
    return *(*argv)++;
}

NODISCARD  
static int help_command(int *argc, char*** argv)
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
        assert(command.desc != NULL);
        printf("    %s, %-10s  %s\n", command.s_flag, command.l_flag, command.desc);
    }
    return 0;
}

NODISCARD
static int add_task_command(int *argc, char*** argv)
{
    const char* task = shift_args(argc, argv);
    if(task == NULL)
    {
        EPRINT("Please provide a task to be added to the list of todos"); 
        HELP("td add \"TASK TO BE ADDED\"");
        return 1;
    }
    FILE *todos_file = fopen(TODOS_FILE, "a");
    if(todos_file == NULL)
    {
        perror("ERROR:");
        return 1;
    }
    size_t task_len = strlen(task) + 1;
    size_t written_bytes = fprintf(todos_file, "%s\n", task);
    if(task_len != written_bytes)
    {
        EPRINTF("Failed to write all bytes of %s, expected: %zu but wrote %zu", task, task_len, written_bytes);
        fclose(todos_file);
        return 1;
    }
    fclose(todos_file);
    return 0;
}

int main(int argc, char** argv)
{
    const char* program_name = shift_args(&argc, &argv);
    UNUSED(program_name);
    Command help =  {
        "-h", 
        "help", 
        "prints this help message and quits",
        help_command,
    };
    Command add = {
        "-a",
        "add",
        "adds a todo",
        add_task_command,
    };
    if(!add_command(&commands, add)) return 1;
    if(!add_command(&commands, help)) return 1;
    if(argc < 1)
    {
        if(help_command(&argc, &argv)) return 1;
        return 1;
    }
    const char* flag = shift_args(&argc, &argv);
    for(int i = 0; i < commands.size; ++i)
    {
        Command command = commands.commands[i];
        if(strcmp(command.s_flag, flag) == 0 || strcmp(command.l_flag, flag) == 0)
        {
            assert(command.fn != NULL);
            return command.fn(&argc, &argv);
        }
    }
    EPRINTF("Unrecognized option passed: %s", flag);
    return 1;
}
