#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

#define PATH_MAX 4096
#define MAX_PROCESSES 10

int file_exists(const char *filename) {
    struct stat st;
    return stat(filename, &st) == 0;
}

int is_hex(const char *s) {
    if (s == NULL || *s == '\0') return 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    while (*s) {
        if (!isxdigit(*s)) return 0;
        s++;
    }
    return 1;
}

void xor_operation(const char* filename, int N) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Ошибка открытия файла %s\n", filename);
        return;
    }

    if (N == 2) {
        uint8_t xor_result = 0;
        uint8_t byte;
        int nibble_count = 0;

        while (fread(&byte, 1, 1, file) == 1) {
            uint8_t high_nibble = byte >> 4;
            uint8_t low_nibble = byte & 0x0F;
            
            xor_result ^= high_nibble;
            xor_result ^= low_nibble;
            nibble_count += 2;
        }

        if (nibble_count % 2 != 0) {
            xor_result ^= 0x00;
        }

        printf("Файл %s: XOR2 результат: %02X\n", filename, xor_result & 0x0F);
        fclose(file);
        return;
    }

    int block_size_bytes = (1 << N) / 8;
    uint8_t *buffer = (uint8_t*)malloc(block_size_bytes);
    if (!buffer) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        fclose(file);
        return;
    }

    uint8_t *xor_result = (uint8_t*)calloc(block_size_bytes, 1);
    if (!xor_result) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        free(buffer);
        fclose(file);
        return;
    }

    while (1) {
        size_t bytes_read = fread(buffer, 1, block_size_bytes, file);
        if (bytes_read == 0) break;

        if (bytes_read < block_size_bytes) {
            memset(buffer + bytes_read, 0, block_size_bytes - bytes_read);
        }

        for (int i = 0; i < block_size_bytes; i++) {
            xor_result[i] ^= buffer[i];
        }
    }

    printf("Файл %s: XOR%d результат: ", filename, N);
    for (int i = 0; i < block_size_bytes; i++) {
        printf("%02X", xor_result[i]);
    }
    printf("\n");

    free(buffer);
    free(xor_result);
    fclose(file);
}

void mask_operation(const char* filename, uint32_t mask) {
    FILE *file = fopen(filename, "rb");  
    if (!file) {
        fprintf(stderr, "Ошибка открытия файла %s\n", filename);
        return;
    }

    uint32_t value;
    int matches = 0;
    int total = 0;

    while (fread(&value, sizeof(uint32_t), 1, file) == 1) {
        total++;
        if ((value & mask) == mask) {
            matches++;
            printf("Совпадение: 0x%08x\n", value);
        }
    }

    printf("Файл %s: Найдено %d совпадений из %d (маска: 0x%08X)\n", filename, matches, total, mask);
    fclose(file);
}

void copy_file(const char* src, const char* dst) {
    int src_fd = open(src, O_RDONLY);
    if (src_fd == -1) {
        fprintf(stderr, "Ошибка открытия файла %s\n", src);
        return;
    }

    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        fprintf(stderr, "Ошибка создания файла %s\n", dst);
        close(src_fd);
        return;
    }

    char buffer[2048];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(src_fd, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written != bytes_read) {
            fprintf(stderr, "Ошибка записи в файл %s\n", dst);
            close(src_fd);
            close(dst_fd);
            return;
        }
    }

    close(src_fd);
    close(dst_fd);
    printf("Создана копия: %s\n", dst);
    return;
}

void *memmem(const void *haystack, size_t haystacklen,
    const void *needle, size_t needlelen) {
    if (needlelen == 0) return (void *)haystack;
    if (haystacklen < needlelen) return NULL;

    const char *h = haystack;
    const char *n = needle;
    const char *end = h + haystacklen - needlelen + 1;

    for (; h < end; h++) {
        if (*h == *n && memcmp(h, n, needlelen) == 0) {
            return (void *)h;
        }
    }
    return NULL;
}

void find_in_file(const char* filename, const char* search_str) {
    char *processed_str = malloc(strlen(search_str) + 1);
    int j = 0;
    for (int i = 0; search_str[i]; i++) {
        if (search_str[i] == '\\' && search_str[i+1] == 'n') {
            processed_str[j++] = '\n';
            i++;
        } else {
            processed_str[j++] = search_str[i];
        }
    }
    processed_str[j] = '\0';

    FILE *file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Ошибка открытия файла %s\n", filename);
        free(processed_str);
        return;
    }

    size_t search_len = j;
    char buffer[1024];
    size_t bytes_read;
    size_t overlap = search_len > 0 ? search_len - 1 : 0;
    int found = 0;

    bytes_read = fread(buffer, 1, sizeof(buffer) - overlap, file);
    
    while (!found && bytes_read > 0) {
        if (memmem(buffer, bytes_read, processed_str, search_len) != NULL) {
            found = 1;
            break;
        }

        if (feof(file)) break;

        if (bytes_read > overlap) {
            memmove(buffer, buffer + bytes_read - overlap, overlap);
            bytes_read = overlap;
        }

        size_t new_read = fread(buffer + bytes_read, 1, sizeof(buffer) - bytes_read, file);
        if (new_read == 0) break;
        bytes_read += new_read;
    }

    char full_path[PATH_MAX];
    if (getcwd(full_path, sizeof(full_path))) {
        strncat(full_path, "/", sizeof(full_path) - strlen(full_path) - 1);
        strncat(full_path, filename, sizeof(full_path) - strlen(full_path) - 1);
    } else {
        strncpy(full_path, filename, sizeof(full_path));
    }

    if (found) {
        printf("Найдено в: %s\n", full_path);
    } else {
        printf("Не найдено '");
        for (const char *p = search_str; *p; p++) {
            if (*p == '\n') printf("\\n");
            else if (*p == '\\') printf("\\\\");
            else putchar(*p);
        }
        printf("' в файле: %s\n", filename);
    }

    free(processed_str);
    fclose(file);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Использование:\n"
                "  %s файл1 [файл2...] xorN\n"
                "  %s файл1 [файл2...] mask <hex-маска>\n"
                "  %s файл1 [файл2...] copyN\n"
                "  %s файл1 [файл2...] find \"строка\"\n", 
                argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    char *operation = NULL;
    char *operation_arg = NULL;
    int first_file_index = 1;
    int last_file_index;

    if (strncmp(argv[argc-1], "xor", 3) == 0 || 
        strncmp(argv[argc-1], "copy", 4) == 0) {
        operation = argv[argc-1];
        last_file_index = argc - 2;
    } 
    else if (strcmp(argv[argc-2], "mask") == 0 || 
             strcmp(argv[argc-2], "find") == 0) {
        operation = argv[argc-2];
        operation_arg = argv[argc-1];
        last_file_index = argc - 3;
    }
    else {
        operation = argv[argc-1];
    }

    if (first_file_index > last_file_index) {
        fprintf(stderr, "Не указаны файлы для обработки\n");
        return 1;
    }

    if (strncmp(operation, "xor", 3) == 0) {
        int N = atoi(operation + 3);
        if (N < 2 || N > 6) {
            fprintf(stderr, "N должно быть от 2 до 6 (получено %d)\n", N);
            return 1;
        }

        for (int i = first_file_index; i <= last_file_index; i++) {
            if (!file_exists(argv[i])) {
                fprintf(stderr, "Файл %s не существует\n", argv[i]);
                continue;
            }
            xor_operation(argv[i], N);
        }
    }
    else if (strcmp(operation, "mask") == 0) {
        if (!operation_arg) {
            fprintf(stderr, "Для mask необходимо указать hex-маску\n");
            return 1;
        }

        char *endptr;
        uint32_t mask_value = strtoul(operation_arg, &endptr, 16);
        if (*endptr != '\0') {
            fprintf(stderr, "Неверный формат маски. Используйте hex, например ABCD\n");
            return 1;
        }

        for (int i = first_file_index; i <= last_file_index; i++) {
            if (!file_exists(argv[i])) {
                fprintf(stderr, "Файл %s не существует\n", argv[i]);
                continue;
            }
            mask_operation(argv[i], mask_value);
        }
    }

    else if (strncmp(operation, "copy", 4) == 0) {
        int N = atoi(operation + 4);
        if (N <= 0) {
            fprintf(stderr, "N должно быть положительным числом\n");
            return 1;
        }
    
        pid_t pids[MAX_PROCESSES];
        int active_processes = 0;
    
        for (int i = first_file_index; i <= last_file_index; i++) {
            if (!file_exists(argv[i])) {
                fprintf(stderr, "Файл %s не существует\n", argv[i]);
                continue;
            }
    
            for (int copy_num = 1; copy_num <= N; copy_num++) {
                if (active_processes >= MAX_PROCESSES) {
                    wait(NULL);
                    active_processes--;
                }
    
                pid_t pid = fork();
                if (pid == 0) {
                    char new_name[PATH_MAX];
                    snprintf(new_name, sizeof(new_name), "%s_%d", argv[i], copy_num);
                    copy_file(argv[i], new_name);
                    exit(0); 
                } 
                else if (pid > 0) {
                    pids[active_processes++] = pid;
                } 
                else {
                    fprintf(stderr, "Ошибка создания процесса для копирования\n");
                }
            }
        }
    
        while (active_processes > 0) {
            wait(NULL);
            active_processes--;
        }
    }

    else if (strcmp(operation, "find") == 0) {
        if (!operation_arg) {
            fprintf(stderr, "Для find необходимо указать строку поиска\n");
            return 1;
        }
        
        int active_processes = 0;
        int status;
        
        for (int i = first_file_index; i <= last_file_index; i++) {
            if (!file_exists(argv[i])) {
                fprintf(stderr, "Файл %s не существует\n", argv[i]);
                continue;
            }
        
            while (active_processes >= MAX_PROCESSES) {
                if (waitpid(-1, &status, 0) > 0) {
                    active_processes--;
                }
            }
        
            pid_t pid = fork();
            if (pid == 0) {
                find_in_file(argv[i], operation_arg);
                exit(0); 
            } 
            else if (pid > 0) {
                active_processes++;
            } 
            else {
                fprintf(stderr, "Ошибка создания процесса для поиска\n");
                continue;
            }
        }
        
        while (active_processes > 0) {
            if (waitpid(-1, &status, 0) > 0) {
                active_processes--;
            }
        }
    }
    return 0;
}