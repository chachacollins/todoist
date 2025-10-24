#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#define NODISCARD __attribute__((warn_unused_result))
#define UNUSED(var) (void)(var);
//TODO: add somekind of logging system
#define EPRINT(fmt) fprintf(stderr, "\033[31mERROR: " fmt "\033[0m\n")
#define EPRINTF(fmt, ...) fprintf(stderr, "\033[31mERROR:" fmt "\033[0m\n", __VA_ARGS__)
#define SPRINTF(fmt, ...) printf("\033[32mSUCCESS: " fmt "\033[0m\n", __VA_ARGS__)
#define HELP(fmt) fprintf(stderr, "\033[33mUSAGE: " fmt "\033[0m\n")
#define TODOS_FILE "todos.txt"
#define SUCCESS 0
#define FAILURE 1

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
    return SUCCESS;
}

NODISCARD
static int add_task_command(int *argc, char*** argv)
{
    const char* task = shift_args(argc, argv);
    if(task == NULL)
    {
        EPRINT("Please provide a task to be added to the list of todos"); 
        HELP("td add \"TASK TO BE ADDED\"");
        return FAILURE;
    }
    FILE *todos_file = fopen(TODOS_FILE, "a");
    if(todos_file == NULL)
    {
        EPRINTF("Could not open file %s because %s", TODOS_FILE, strerror(errno));
        return FAILURE;
    }
    const char* prefix = "- [ ] ";
    size_t total_write = strlen(prefix) + strlen(task) + 1;
    size_t written_bytes = fprintf(todos_file, "%s%s\n", prefix, task);
    if(total_write != written_bytes)
    {
        EPRINTF("Failed to write all bytes of %s, expected: %zu but wrote %zu", task, total_write, written_bytes);
        fclose(todos_file);
        return FAILURE;
    }
    fclose(todos_file);
    SPRINTF("added task %s", task);
    return SUCCESS;
}

NODISCARD
static const char* read_file(const char* file_path)
{
    FILE *file = fopen(file_path, "rb");
    if(file == NULL)
    {
        EPRINTF("Could not open file %s because %s", file_path, strerror(errno));
        return NULL;
    }
    if(fseek(file, 0L, SEEK_END) < 0)
    {
        EPRINTF("Could not seek file %s because %s", file_path, strerror(errno));
        fclose(file);
        return NULL;
    }
    size_t file_size = ftell(file);
    if(fseek(file, 0L, SEEK_SET) < 0)
    {
        EPRINTF("Could not seek file %s because %s", file_path, strerror(errno));
        fclose(file);
        return NULL;
    }
    char* buffer = (char*) malloc(file_size + 1);
    if(buffer == NULL)
    {
        EPRINTF("Could not allocate buffer to hold contents for file %s because %s", file_path, strerror(errno));
        fclose(file);
        return NULL;
    }
    size_t read_bytes = fread(buffer, sizeof(char), file_size, file);
    if(read_bytes != file_size)
    {
        free(buffer);
        fclose(file);
        EPRINTF("Failed to read all bytes of %s, expected: %zu but wrote %zu", file_path, file_size, read_bytes);
        return NULL;
    }
    *(buffer+file_size) = '\0';
    fclose(file);
    return buffer;
}

NODISCARD
static int list_tasks_command(int *argc, char*** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    const char* todo_file = read_file(TODOS_FILE);
    if(todo_file == NULL) return FAILURE;
    printf("-----------------TODO------------------\n");
    printf("%s", todo_file);
    return SUCCESS;
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
    Command list = {
        "-ls",
        "list",
        "lists all todos",
        list_tasks_command,
    };
    if(!add_command(&commands, add)) return FAILURE;
    if(!add_command(&commands, help)) return FAILURE;
    if(!add_command(&commands, list)) return FAILURE;
    if(argc < 1)
    {
        if(help_command(&argc, &argv)) return FAILURE;
        return FAILURE;
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
    return FAILURE;
}
