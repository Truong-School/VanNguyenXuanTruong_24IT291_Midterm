#ifndef LS_OPTIONS_H
#define LS_OPTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>

// Cấu trúc lưu trữ trạng thái các cờ được bật từ dòng lệnh
typedef struct {
    int show_all;            // -a: Bao gồm file ẩn (. và ..)
    int show_almost_all;     // -A: Bao gồm file ẩn ngoại trừ . và ..
    int show_inode;          // -i: In số inode
    int show_long;           // -l: Định dạng danh sách chi tiết
    int recursive;           // -R: Duyệt đệ quy thư mục con
    int reverse_sort;        // -r: Đảo ngược thứ tự sắp xếp
    int sort_by_time;        // -t: Sắp xếp theo thời gian
    int sort_by_size;        // -S: Sắp xếp theo kích thước
    int show_blocks;         // -s: In số khối hệ thống tệp
    int show_type_indicator; // -F: Phân loại tệp (/ * @ = |)
    int human_readable;      // -h: Định dạng kích thước dễ đọc (K, M, G)
    int numeric_uid_gid;     // -n: In UID và GID dạng số
    int time_type;           // 0: mtime, 1: atime (-u), 2: ctime (-c)
    int disable_sort;        // -f: Tắt sắp xếp
    int dir_as_file;         // -d: Xem thư mục như file thường
    int block_in_kb;         // -k: Hiển thị block dạng KB
    int non_printable_mode;  // 1: Force '?' (-q), 2: Raw (-w)
} LsFlags;

// Cấu trúc lưu thông tin từng tệp để phục vụ sắp xếp
typedef struct {
    char *name;
    char *full_path;
    struct stat statbuf;
} FileItem;

// Khai báo hàm
void list_directory_contents(const char *target_path, LsFlags flags);

#endif // LS_OPTIONS_H