#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <sqlite3.h>

//NOTE: POSIX EXTENSION. IMPLEMENT IT IN MISSING ENVIRONMENTS
unsigned char *strdup(const unsigned char *s);
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
static sqlite3 *db;

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
    assert(db != NULL);
    const char* task = shift_args(argc, argv);
    if(task == NULL)
    {
        EPRINT("Please provide a task to be added to the list of todos"); 
        HELP("td add \"TASK TO BE ADDED\"");
        return FAILURE;
    }
#define MAXIMUM_INSERT_LENGTH 1024
    char insert_task_into_table[MAXIMUM_INSERT_LENGTH];
    sprintf(insert_task_into_table, "INSERT INTO TASK(task, done) VALUES (\"%s\", false)", task);
    sqlite3_stmt *pp_stmt;
    int sql3_result = sqlite3_prepare_v2(db, 
            insert_task_into_table,
            strlen(insert_task_into_table),
            &pp_stmt, 
            NULL
            );
    if(sql3_result != SQLITE_OK)
    {
        EPRINTF("could not prepare sql statement because %s", sqlite3_errmsg(db));
        return FAILURE;
    }
    sql3_result = sqlite3_step(pp_stmt);
    if(sql3_result != SQLITE_DONE)
    {
        EPRINTF("could not run sql statement insert_into_table because %s", sqlite3_errmsg(db));
        return FAILURE;
    }
    sql3_result = sqlite3_finalize(pp_stmt);
    if(sql3_result != SQLITE_OK)
    {
        EPRINTF("could not finalize sql statement insert_into_table because %s", sqlite3_errmsg(db));
        return FAILURE;
    }
    SPRINTF("added task %s", task);
    return SUCCESS;
}

typedef struct {
    unsigned char* todo;
    int id;
} Task;

typedef struct {
    size_t capacity;
    size_t len;
    Task *tasks;
} Tasks;

NODISCARD
static int insert_task(Tasks *tasks, Task task) 
{
    if(tasks->len + 1 >= tasks->capacity)
    {
        size_t new_capacity = tasks->capacity > 8 ? tasks->capacity * 2 : 8;
        tasks->tasks = realloc(tasks->tasks, sizeof(Task) * new_capacity);
        if(tasks->tasks == NULL)
        {
            EPRINTF("could not allocate memory to add task because: %s", strerror(errno));
            return FAILURE;
        }
        tasks->capacity = new_capacity;
    }
    tasks->tasks[tasks->len++] = task;
    return SUCCESS;
}

NODISCARD
static int free_tasks(Tasks *tasks)
{
    for(size_t i = 0; i < tasks->len; ++i)
    {
        free(tasks->tasks[i].todo);
    }
    free(tasks->tasks);
    memset(tasks, sizeof(Task), tasks->capacity);
    return SUCCESS;
}

NODISCARD
static int get_max_id_length(Tasks *tasks)
{
    int max_length = 0;
    for(size_t i = 0; i < tasks->len; ++i) 
    {
        Task task = tasks->tasks[i];
        if(task.id > max_length)
        {
            max_length = task.id;
        }
    }
    return ((int)floor(log10(max_length))) + 1;
}

NODISCARD
static size_t get_max_str_length(Tasks *tasks)
{
    size_t max_length = 0;
    for(size_t i = 0; i < tasks->len; ++i) 
    {
        Task task = tasks->tasks[i];
        size_t length = strlen((char*)task.todo);
        if(length > max_length)
        {
            max_length = length;
        }
    }
    return max_length;
}

static size_t evenize(size_t n)
{
    if((n % 2) != 0) n = n + 1;
    return n;
}

inline void print_separator(size_t total_len)
{
    for(size_t i = 0; i < total_len; ++i)
    {
        printf("-");
    }
    printf("\n");
    return;
}

NODISCARD
static int list_tasks_command(int *argc, char*** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    assert(db != NULL);
    const char* retrieve_tasks_from_table = "SELECT * FROM TASK;";
    sqlite3_stmt *pp_stmt;
    int sql3_result = sqlite3_prepare_v2(db, 
            retrieve_tasks_from_table,
            strlen(retrieve_tasks_from_table),
            &pp_stmt, 
            NULL
            );
    if(sql3_result != SQLITE_OK)
    {
        EPRINTF("could not prepare sql statement because %s", sqlite3_errmsg(db));
        return FAILURE;
    }
    sql3_result = sqlite3_step(pp_stmt);
    const int id_row   = 0;
    const int task_row = 1;
    Tasks tasks = {0};
    while(sql3_result == SQLITE_ROW)
    {
        const int id  = sqlite3_column_int(pp_stmt, id_row);
        const unsigned char* task = sqlite3_column_text(pp_stmt, task_row);
        if(!insert_task(&tasks, (Task) {strdup(task), (int) id}))
        {
            sqlite3_finalize(pp_stmt);
            return FAILURE;
        }
        sql3_result = sqlite3_step(pp_stmt);
    }
    if(sql3_result != SQLITE_DONE)
    {
        EPRINTF("could not run sql statement retrieve_tasks because %s", sqlite3_errmsg(db));
        return FAILURE;
    }
    sql3_result = sqlite3_finalize(pp_stmt);
    if(sql3_result != SQLITE_OK)
    {
        EPRINTF("could not finalize sql statement retrieve_tasks because %s", sqlite3_errmsg(db));
        return FAILURE;
    }
    int id_len = evenize(get_max_id_length(&tasks));
#define MIN_ID 4
    id_len = id_len > MIN_ID ? id_len : MIN_ID;
    int todo_len = evenize(get_max_str_length(&tasks));
#define PADDING_BYTE_LEN 3
    size_t total_len = id_len + todo_len + PADDING_BYTE_LEN;
    print_separator(total_len);
    printf("%-*s| %-*s|\n", id_len,"ID", todo_len, "TASK");
    print_separator(total_len);
    for(size_t i = 0; i < tasks.len; ++i)
    {
        Task task = tasks.tasks[i];
        printf("%-*d|%-*s |\n", id_len, task.id, todo_len, task.todo);
    }
    print_separator(total_len);
    if(!free_tasks(&tasks)) return FAILURE;
    return SUCCESS;
}

NODISCARD 
static int init_command(int *argc, char*** argv)
{
    UNUSED(argc);
    UNUSED(argv);
    assert(db != NULL);
    const char* create_task_table = 
        "CREATE TABLE TASK ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "task TEXT,"
            "done bool"
        ");";

    sqlite3_stmt *pp_stmt;
    int sql3_result = sqlite3_prepare_v2(db, create_task_table, strlen(create_task_table), &pp_stmt, NULL);
    if(sql3_result != SQLITE_OK)
    {
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
        "-l",
        "list",
        "lists all todos",
        list_tasks_command,
    };
    Command init = {
        "-i",
        "init",
        "initializes the application",
        init_command,
    };
    if(!add_command(&commands, init)) return EXIT_FAILURE;
    if(!add_command(&commands, add)) return EXIT_FAILURE;
    if(!add_command(&commands, list)) return EXIT_FAILURE;
    if(!add_command(&commands, help)) return EXIT_FAILURE;
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
