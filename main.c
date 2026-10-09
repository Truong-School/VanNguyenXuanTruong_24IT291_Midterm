#include "ls_options.h"

int main(int argc, char *argv[]) {
    LsFlags flags = {0};
    int opt;

    flags.non_printable_mode = isatty(STDOUT_FILENO) ? 1 : 2;

    while ((opt = getopt(argc, argv, "AacdFfhiklnqRrSstuw")) != -1) {
        switch (opt) {
            case 'a': flags.show_all = 1; break;
            case 'A': flags.show_almost_all = 1; break;
            case 'c': flags.time_type = 2; break;
            case 'u': flags.time_type = 1; break;
            case 'd': 
                flags.dir_as_file = 1; 
                flags.recursive = 0; 
                break;
            case 'R': 
                flags.recursive = 1; 
                flags.dir_as_file = 0; 
                break;
            case 'F': flags.show_type_indicator = 1; break;
            case 'f': flags.disable_sort = 1; break;
            case 'h': 
                flags.human_readable = 1; 
                flags.block_in_kb = 0; 
                break;
            case 'k': 
                flags.block_in_kb = 1; 
                flags.human_readable = 0; 
                break;
            case 'i': flags.show_inode = 1; break;
            case 'l': 
                flags.show_long = 1; 
                flags.numeric_uid_gid = 0; 
                break;
            case 'n': 
                flags.show_long = 1; 
                flags.numeric_uid_gid = 1; 
                break;
            case 'q': flags.non_printable_mode = 1; break;
            case 'w': flags.non_printable_mode = 2; break;
            case 'r': flags.reverse_sort = 1; break;
            case 'S': flags.sort_by_size = 1; break;
            case 's': flags.show_blocks = 1; break;
            case 't': flags.sort_by_time = 1; break;
            default:
                fprintf(stderr, "Usage: %s [-AacdFfhiklnqRrSstuw] [file...]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind == argc) {
        list_directory_contents(".", flags);
    } else {
        int multiple = (argc - optind > 1);
        while (optind < argc) {
            if (multiple && !flags.dir_as_file) {
                printf("%s:\n", argv[optind]);
            }
            list_directory_contents(argv[optind], flags);
            if (multiple && optind + 1 < argc) putchar('\n');
            optind++;
        }
    }

    return 0;
}