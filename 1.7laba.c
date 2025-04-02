#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#define PATH_MAX 4096

const char* get_file_type(mode_t mode) {
    if (S_ISREG(mode)) return "файл";
    if (S_ISDIR(mode)) return "каталог";
    if (S_ISCHR(mode)) return "символьное устройство";
    if (S_ISBLK(mode)) return "блочное устройство";
    if (S_ISFIFO(mode)) return "FIFO/канал";
    if (S_ISLNK(mode)) return "символическая ссылка";
    return "неизвестный тип";
}

void my_ls(const char *dirpath) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    char fullpath[PATH_MAX];

    dir = opendir(dirpath);
    if (dir == NULL) {
        fprintf(stderr, "Ошибка открытия каталога '%s'\n", dirpath);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", dirpath, entry->d_name);

        if (stat(fullpath, &file_stat) == -1) {
            fprintf(stderr, "Ошибка получения информации о '%s'\n", fullpath);
            continue;
        }

        printf("%-10lu %-30s %s\n", (unsigned long)entry->d_ino, entry->d_name, get_file_type(file_stat.st_mode));
    }

    if (closedir(dir) == -1) {
        fprintf(stderr, "Ошибка закрытия каталога '%s'\n", dirpath);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Использование: %s <путь к каталогу>\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat path_stat;
        
        if (stat(argv[i], &path_stat) == -1) {
            fprintf(stderr, "Ошибка доступа к '%s'\n", argv[i]);
            continue;
        }

        if (!S_ISDIR(path_stat.st_mode)) {
            fprintf(stderr, "'%s' не является каталогом\n", argv[i]);
            continue;
        }
        printf("Содержимое каталога '%s':\n", argv[i]);
        my_ls(argv[i]);
        printf("\n");
    }

    return 0;
}