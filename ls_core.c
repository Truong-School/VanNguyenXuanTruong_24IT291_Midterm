#include "ls_options.h"

static int g_reverse = 0;
static int g_sort_time = 0;
static int g_sort_size = 0;
static int g_time_type = 0;

// Chuyển đổi mode sang chuỗi quyền hạn (rwxrwxrwx) bao gồm cả SUID, SGID, Sticky bit
static void get_permission_string(mode_t mode, char *str) {
    strcpy(str, "----------");
    
    if (S_ISDIR(mode))  str[0] = 'd';
    if (S_ISCHR(mode))  str[0] = 'c';
    if (S_ISBLK(mode))  str[0] = 'b';
    if (S_ISLNK(mode))  str[0] = 'l';
    if (S_ISFIFO(mode)) str[0] = 'p';
    if (S_ISSOCK(mode)) str[0] = 's';

    if (mode & S_IRUSR) str[1] = 'r';
    if (mode & S_IWUSR) str[2] = 'w';
    if (mode & S_IXUSR) str[3] = 'x';
    if (mode & S_ISUID) str[3] = (mode & S_IXUSR) ? 's' : 'S';

    if (mode & S_IRGRP) str[4] = 'r';
    if (mode & S_IWGRP) str[5] = 'w';
    if (mode & S_IXGRP) str[6] = 'x';
    if (mode & S_ISGID) str[6] = (mode & S_IXGRP) ? 's' : 'S';

    if (mode & S_IROTH) str[7] = 'r';
    if (mode & S_IWOTH) str[8] = 'w';
    if (mode & S_IXOTH) str[9] = 'x';
    if (mode & S_ISVTX) str[9] = (mode & S_IXOTH) ? 't' : 'T';
}

// In tên an toàn cho ký tự không in được (-q / -w)
static void print_formatted_name(const char *name, int mode) {
    if (mode == 1) {
        for (int i = 0; name[i] != '\0'; i++) {
            unsigned char c = (unsigned char)name[i];
            putchar((c >= 32 && c < 127) ? c : '?');
        }
    } else {
        printf("%s", name);
    }
}

// Lấy ký tự chỉ thị cho tùy chọn -F
static char get_file_indicator(mode_t mode) {
    if (S_ISDIR(mode)) return '/';
    if (S_ISLNK(mode)) return '@';
    if (S_ISSOCK(mode)) return '=';
    if (S_ISFIFO(mode)) return '|';
    if (mode & (S_IXUSR | S_IXGRP | S_IXOTH)) return '*';
    return '\0';
}

// Định dạng dung lượng kiểu dễ đọc (-h)
static void format_human_readable(off_t size, char *out_buf) {
    const char *units[] = {"B", "K", "M", "G", "T"};
    int idx = 0;
    double dsize = (double)size;

    while (dsize >= 1024.0 && idx < 4) {
        dsize /= 1024.0;
        idx++;
    }

    if (idx == 0) {
        sprintf(out_buf, "%4ldB", (long)size);
    } else {
        sprintf(out_buf, "%4.1f%s", dsize, units[idx]);
    }
}

// Hiển thị chi tiết tệp khi dùng -l / -n (Bổ sung xử lý Symlink)
static void display_long_format(const char *name, const char *full_path, const struct stat *st, LsFlags flags) {
    char perm[11];
    char time_str[64];
    char size_buf[32];
    struct passwd *pw;
    struct group *gr;
    time_t t_val;

    get_permission_string(st->st_mode, perm);
    printf("%s %2lu ", perm, (unsigned long)st->st_nlink);

    if (flags.numeric_uid_gid) {
        printf("%-8u %-8u ", st->st_uid, st->st_gid);
    } else {
        pw = getpwuid(st->st_uid);
        gr = getgrgid(st->st_gid);
        if (pw) printf("%-8s ", pw->pw_name);
        else printf("%-8u ", st->st_uid);

        if (gr) printf("%-8s ", gr->gr_name);
        else printf("%-8u ", st->st_gid);
    }

    if (flags.human_readable) {
        format_human_readable(st->st_size, size_buf);
        printf("%8s ", size_buf);
    } else {
        printf("%8ld ", (long)st->st_size);
    }

    // Xác định mốc thời gian dựa vào -u hoặc -c
    if (flags.time_type == 1) t_val = st->st_atime;
    else if (flags.time_type == 2) t_val = st->st_ctime;
    else t_val = st->st_mtime;

    struct tm *tm_info = localtime(&t_val);
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);
    printf("%s ", time_str);

    // In tên tệp
    print_formatted_name(name, flags.non_printable_mode);

    if (flags.show_type_indicator) {
        char ind = get_file_indicator(st->st_mode);
        if (ind) putchar(ind);
    }

    // Xử lý Symlink theo chuẩn (filename -> target)
    if (S_ISLNK(st->st_mode)) {
        char target_path[1024];
        ssize_t len = readlink(full_path, target_path, sizeof(target_path) - 1);
        if (len != -1) {
            target_path[len] = '\0';
            printf(" -> %s", target_path);
        }
    }
    putchar('\n');
}

// Hàm so sánh dùng cho qsort
static int compare_items(const void *a, const void *b) {
    const FileItem *item_a = (const FileItem *)a;
    const FileItem *item_b = (const FileItem *)b;
    int res = 0;
    time_t ta, tb;

    if (g_time_type == 1) { ta = item_a->statbuf.st_atime; tb = item_b->statbuf.st_atime; }
    else if (g_time_type == 2) { ta = item_a->statbuf.st_ctime; tb = item_b->statbuf.st_ctime; }
    else { ta = item_a->statbuf.st_mtime; tb = item_b->statbuf.st_mtime; }

    if (g_sort_size) {
        if (item_a->statbuf.st_size < item_b->statbuf.st_size) res = 1;
        else if (item_a->statbuf.st_size > item_b->statbuf.st_size) res = -1;
        else res = strcmp(item_a->name, item_b->name);
    } else if (g_sort_time) {
        if (ta < tb) res = 1;
        else if (ta > tb) res = -1;
        else res = strcmp(item_a->name, item_b->name);
    } else {
        res = strcmp(item_a->name, item_b->name);
    }

    return g_reverse ? -res : res;
}

// Hàm xử lý chính (Sử dụng cấp phát động để chống Segfault)
void list_directory_contents(const char *target_path, LsFlags flags) {
    struct stat target_st;

    g_reverse = flags.reverse_sort;
    g_sort_time = flags.sort_by_time;
    g_sort_size = flags.sort_by_size;
    g_time_type = flags.time_type;

    if (lstat(target_path, &target_st) == -1) {
        perror(target_path);
        return;
    }

    if (flags.dir_as_file || !S_ISDIR(target_st.st_mode)) {
        if (flags.show_inode) printf("%llu ", (unsigned long long)target_st.st_ino);
        if (flags.show_blocks) {
            long long blk = target_st.st_blocks;
            if (flags.block_in_kb) blk = (blk + 1) / 2;
            printf("%llu ", (unsigned long long)blk);
        }
        if (flags.show_long) {
            display_long_format(target_path, target_path, &target_st, flags);
        } else {
            print_formatted_name(target_path, flags.non_printable_mode);
            if (flags.show_type_indicator) {
                char ind = get_file_indicator(target_st.st_mode);
                if (ind) putchar(ind);
            }
            putchar('\n');
        }
        return;
    }

    DIR *dir = opendir(target_path);
    if (!dir) {
        perror(target_path);
        return;
    }

    size_t capacity = 128;
    size_t count = 0;
    FileItem *items = malloc(capacity * sizeof(FileItem));
    if (!items) {
        closedir(dir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            if (!flags.show_all && !flags.show_almost_all) continue;
            if (flags.show_almost_all && !flags.show_all) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
            }
        }

        if (count >= capacity) {
            capacity *= 2;
            FileItem *new_items = realloc(items, capacity * sizeof(FileItem));
            if (!new_items) break;
            items = new_items;
        }

        items[count].name = strdup(entry->d_name);
        size_t path_len = strlen(target_path) + strlen(entry->d_name) + 2;
        items[count].full_path = malloc(path_len);
        snprintf(items[count].full_path, path_len, "%s/%s", target_path, entry->d_name);

        if (lstat(items[count].full_path, &items[count].statbuf) == -1) {
            free(items[count].name);
            free(items[count].full_path);
            continue;
        }
        count++;
    }
    closedir(dir);

    if (!flags.disable_sort) {
        qsort(items, count, sizeof(FileItem), compare_items);
    }

    if (flags.show_long || flags.show_blocks) {
        long long total_blks = 0;
        for (size_t i = 0; i < count; i++) {
            long long blk = items[i].statbuf.st_blocks;
            if (flags.block_in_kb) blk = (blk + 1) / 2;
            total_blks += blk;
        }
        if (flags.show_long && count > 0) {
            printf("total %llu\n", (unsigned long long)total_blks);
        }
    }

    size_t sub_cap = 16;
    size_t sub_count = 0;
    char **sub_dirs = malloc(sub_cap * sizeof(char *));

    for (size_t i = 0; i < count; i++) {
        if (flags.show_inode) printf("%llu ", (unsigned long long)items[i].statbuf.st_ino);
        if (flags.show_blocks) {
            long long blk = items[i].statbuf.st_blocks;
            if (flags.block_in_kb) blk = (blk + 1) / 2;
            printf("%llu ", (unsigned long long)blk);
        }

        if (flags.show_long) {
            display_long_format(items[i].name, items[i].full_path, &items[i].statbuf, flags);
        } else {
            print_formatted_name(items[i].name, flags.non_printable_mode);
            if (flags.show_type_indicator) {
                char ind = get_file_indicator(items[i].statbuf.st_mode);
                if (ind) putchar(ind);
            }
            printf("  ");
        }

        if (flags.recursive && S_ISDIR(items[i].statbuf.st_mode)) {
            if (strcmp(items[i].name, ".") != 0 && strcmp(items[i].name, "..") != 0) {
                if (sub_count >= sub_cap) {
                    sub_cap *= 2;
                    char **new_sub = realloc(sub_dirs, sub_cap * sizeof(char *));
                    if (new_sub) sub_dirs = new_sub;
                }
                if (sub_dirs) sub_dirs[sub_count++] = strdup(items[i].full_path);
            }
        }

        free(items[i].name);
        free(items[i].full_path);
    }
    free(items);

    if (!flags.show_long) putchar('\n');

    if (flags.recursive && sub_dirs) {
        for (size_t i = 0; i < sub_count; i++) {
            printf("\n%s:\n", sub_dirs[i]);
            list_directory_contents(sub_dirs[i], flags);
            free(sub_dirs[i]);
        }
    }
    free(sub_dirs);
}