#include <syscall.h>
#include <string.h>
#include <iolib.h>
#include <mem.h>


#define BFR_SIZE 90
#define SC_MAX 57

#define SHELL_PROMPT "profanOS [$9%s$7] -> "

static char current_dir[256] = "/";

typedef struct {
    char name[256];
} string_20_t;

void assemble_path(char old[], char new[], char result[]);
void parse_path(char path[], string_20_t liste_path[]);
void go(char file[], char prefix[], char suffix[]);
int shell_command(char command[]);
void gpd();

int main(int argc, char **argv) {
    char char_buffer[BFR_SIZE];
    int history_size = 0x1000 / BFR_SIZE - 1;
    char **history = malloc(history_size * sizeof(char*));
    int addr = (int) malloc(0x1000);
    for (int i = 0; i < history_size; i++) {
        history[i] = (char*)addr;
        addr += BFR_SIZE;
    }
    int current_history_size = 0;

    while (1) {
        fsprint(SHELL_PROMPT, current_dir);
        input_wh(char_buffer, BFR_SIZE, c_blue, history, current_history_size);
        fsprint("\n");
        if (str_cmp(char_buffer, history[0]) && char_buffer[0] != '\0') {
            for (int i = history_size - 1; i > 0; i--) str_cpy(history[i], history[i - 1]);
            if (current_history_size < history_size) current_history_size++;
            str_cpy(history[0], char_buffer);
        }
        if (shell_command(char_buffer)) break;
    }
    free(history[0]);
    free(history);
    return 0;
}

int shell_command(char *buffer) {
    char *prefix = malloc(str_len(buffer) + 5); // size of char is 1 octet
    char *suffix = malloc(str_len(buffer) + 5);
    str_cpy(prefix, buffer);
    str_cpy(suffix, buffer);
    str_start_split(prefix, ' ');
    str_end_split(suffix, ' ');

    int return_value = 0;


    // internal commands

    if (!str_cmp(prefix, "exit")) {
        return_value = 1;
    } else if (!str_cmp(prefix, "cd")) {
        char old_path[256];
        str_cpy(old_path, current_dir);
        string_20_t * liste_path = calloc(1024);
        parse_path(suffix, liste_path);
        for (int i=0; i<str_count(suffix, '/')+1; i++) {
            if (!str_cmp(liste_path[i].name, "..")) {
                gpd();
            } else {
                char *new_path = calloc(256);
                assemble_path(current_dir, liste_path[i].name, new_path);
                if (c_fs_does_path_exists(new_path) && c_fs_get_sector_type(c_fs_path_to_id(new_path)) == 3)
                    str_cpy(current_dir, new_path);
                else {
                    fsprint("$3%s$B path not found\n", new_path);
                    str_cpy(current_dir, old_path);
                    free(new_path);
                    break;
                }
                free(new_path);
            }
        }
        free(liste_path);
    } else if (!str_cmp(prefix, "go")) {
        if (!(str_count(suffix, '.'))) str_cat(suffix, ".bin");
        char *file = malloc(str_len(suffix) + str_len(current_dir) + 3);
        assemble_path(current_dir, suffix, file);
        go(file, prefix, suffix);
        free(file);
    } else {  // shell command
        char *old_prefix = malloc(str_len(prefix) + 1);
        str_cpy(old_prefix, prefix);
        if(!(str_count(prefix, '.'))) str_cat(prefix, ".bin");
        char *file = malloc(str_len(prefix) + str_len(current_dir) + 17);
        assemble_path("/bin/commands", prefix, file);
        if (c_fs_does_path_exists(file) && c_fs_get_sector_type(c_fs_path_to_id(file)) == 2) {
            go(file, old_prefix, suffix);
        } else if (str_cmp(old_prefix, "")) {
            fsprint("$3%s$B is not a valid command.\n", old_prefix);
        }
        free(file);
        free(old_prefix);
    }

    free(prefix);
    free(suffix);
    return return_value;
}

void go(char file[], char prefix[], char suffix[]) {
    if (c_fs_does_path_exists(file) && c_fs_get_sector_type(c_fs_path_to_id(file)) == 2) {
        int argc = str_count(suffix, ' ') + 3;
        char **argv = malloc(argc * sizeof(char *));
        // set argv[0] to the command name
        argv[0] = malloc(str_len(prefix) + 1);
        str_cpy(argv[0], file);
        argv[1] = malloc(str_len(current_dir) + 1);
        str_cpy(argv[1], current_dir);
        for (int i = 2; i < argc; i++) {
            argv[i] = malloc(str_len(suffix) + 1);
            str_cpy(argv[i], suffix);
            str_start_split(argv[i], ' ');
            str_end_split(suffix, ' ');
        }
        c_run_ifexist(file, argc, argv);
        // free
        for (int i = 0; i < argc; i++) free(argv[i]);
        free(argv);
    } else fsprint("$3%s$B file not found\n", file);
}

void assemble_path(char old[], char new[], char result[]) {
    result[0] = '\0';
    str_cpy(result, old);
    if (result[str_len(result) - 1] != '/') str_append(result, '/');
    str_cat(result, new);
}

void parse_path(char path[], string_20_t liste_path[]) {
    int index = 0;
    int index_in_str = 0;
    for (int i = 0; i < str_len(path); i++) {
        if (path[i] != '/') {
            liste_path[index].name[index_in_str] = path[i];
            index_in_str++;
        } else {
            liste_path[index].name[index_in_str] = '\0';
            index++;
            index_in_str = 0;
        }
    }
}

void gpd() {
    for (int i = str_len(current_dir); i > 0; i--) {
        if (current_dir[i] == '/' || i == 1) {
            current_dir[i] = '\0';
            break;
        }
    }
}
