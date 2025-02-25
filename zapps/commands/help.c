#include <syscall.h>
#include <iolib.h>
#include <mem.h>

int main(int argc, char **argv) {
    char file[] = "/zada/shell_help.txt";
    if (c_fs_does_path_exists(file) && c_fs_get_sector_type(c_fs_path_to_id(file)) == 2) {
        char *char_content = c_fs_declare_read_array(file);
        c_fs_read_file(file, (uint8_t *) char_content);
        c_ckprint(char_content, c_magenta);
        free(char_content);
    } else fsprint("$Bshell_help.txt not found\n", file);
    return 0;
}
