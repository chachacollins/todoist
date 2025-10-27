#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sqlite3.h>

#define NODISCARD __attribute__((warn_unused_result))
#define UNUSED(var) (void)(var);
//TODO: add somekind of logging system
#define EPRINT(fmt) fprintf(stderr, "\033[31mERROR: " fmt "\033[0m\n")
#define EPRINTF(fmt, ...) fprintf(stderr, "\033[31mERROR:" fmt "\033[0m\n", __VA_ARGS__)
#define SPRINTF(fmt, ...) printf("\033[32mSUCCESS: " fmt "\033[0m\n", __VA_ARGS__)
#define HELP(fmt) fprintf(stderr, "\033[33mUSAGE: " fmt "\033[0m\n")
#define TODOS_FILE "todos.txt"
#define SUCCESS 1
#define FAILURE 0

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
        return FAILURE;
    }
    comms->commands[comms->size++] = command;
    return SUCCESS;
}

NODISCARD
static const char* shift_args(int *argc, char*** argv)
{
    assert(argc != NULL);
    assert(argv != NULL);
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
static char* read_file(const char* file_path)
{
    assert(file_path != NULL);
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
    long file_size = ftell(file);
    if(file_size < 0)
    {
        EPRINTF("Could not ftell file %s because %s", file_path, strerror(errno));
        fclose(file);
        return NULL;
    }
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
    if(read_bytes != (size_t) file_size)
    {
        free(buffer);
        fclose(file);
        EPRINTF("Failed to read all bytes of %s, expected: %zu but read %zu", file_path, file_size, read_bytes);
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
    char* todo_file = read_file(TODOS_FILE);
    if(todo_file == NULL) return FAILURE;
    printf("-----------------TODO------------------\n");
    printf("%s", todo_file);
    free(todo_file);
    return SUCCESS;
}

static sqlite3 *db;

NODISCARD 
static int init_command(int *argc, char*** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    assert(db != NULL);
    const char* create_task_table = 
        "CREATE TABLE TASK ("
            "id   INTEGER PRIMARY_KEY,"
            "task TEXT,"
            "done bool"
        ");";

    sqlite3_stmt *pp_stmt;
    int sql3_result = sqlite3_prepare_v2(db, create_task_table, strlen(create_task_table), &pp_stmt, NULL);
    if(sql3_result != SQLITE_OK)
    {
        printf("%d\n", sql3_result);
        EPRINTF("could not prepare sql statement because %s", sqlite3_errmsg(db));
        return FAILURE;
    }
    sql3_result = sqlite3_step(pp_stmt);
    if(sql3_result != SQLITE_DONE)
    {
        EPRINTF("could not run sql statement create_table because %s", sqlite3_errmsg(db));
        return FAILURE;
    }
    sql3_result = sqlite3_finalize(pp_stmt);
    if(sql3_result != SQLITE_OK)
    {
        EPRINTF("could not finalize sql statement create_table because %s", sqlite3_errmsg(db));
        return FAILURE;
    }
    SPRINTF("%s","initialized the app successfully");
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
    Command init = {
        "-i",
        "init",
        "Initializes the application",
        init_command,
    };
    if(!add_command(&commands, add)) return EXIT_FAILURE;
    if(!add_command(&commands, help)) return EXIT_FAILURE;
    if(!add_command(&commands, list)) return EXIT_FAILURE;
    if(!add_command(&commands, init)) return EXIT_FAILURE;
    if(argc < 1)
    {
        if(!help_command(&argc, &argv)) return EXIT_FAILURE;
        return EXIT_FAILURE;
    }
    int result = EXIT_SUCCESS;
    int sql3_result = sqlite3_open("task.db", &db);
    if(sql3_result != SQLITE_OK)
    {
        EPRINTF("could not open sqlite database connection because of %s", sqlite3_errmsg(db));
        result = EXIT_FAILURE;
        goto done;
    }
    const char* flag = shift_args(&argc, &argv);
    for(int i = 0; i < commands.size; ++i)
    {
        Command command = commands.commands[i];
        if(strcmp(command.s_flag, flag) == 0 || strcmp(command.l_flag, flag) == 0)
        {
            assert(command.fn != NULL);
            if(!command.fn(&argc, &argv)) result = EXIT_FAILURE;
            goto done;
        }
    }
    EPRINTF("Unrecognized option passed: %s", flag);
    result = EXIT_FAILURE;
done:
    sqlite3_close(db);
    return result;
}
