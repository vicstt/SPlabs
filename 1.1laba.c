#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_LOGIN 6
#define MAX_USERS 100

typedef struct {
    char login[MAX_LOGIN + 1];
    int pin;
    int request_limit;
    int request_count;
} User;

typedef struct {
    User users[MAX_USERS];
    int user_count;
    User* current_user;
    pthread_mutex_t mutex;
} Current;

char* strdup(const char* str) {
    if (str == NULL) {
        return NULL;
    }
    size_t len = strlen(str) + 1;
    char* new_str = (char*)malloc(len);
    if (new_str != NULL) {
        strcpy(new_str, str);
    }
    return new_str;
}

int valid_login(const char* login) {
    for (int i = 0; login[i] != '\0'; i++) {
        if (!isalnum(login[i])) {
            return 0;
        }
    }
    return 1;
}

void registration(Current* state) {
    pthread_mutex_lock(&state->mutex);
    if (state->user_count >= MAX_USERS) {
        printf("Достигнуто максимальное количество пользователей.\n");
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    char login[MAX_LOGIN + 1];
    int pin;

    printf("Регистрация нового пользователя\n");
    printf("Введите логин (не более 6 символов, только латинские буквы и цифры): ");
    
    if (scanf("%6s", login) != 1) {
        printf("Ошибка ввода логина.\n");
        while (getchar() != '\n'); 
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    if (!valid_login(login)) {
        printf("Логин должен содержать только латинские буквы и цифры.\n");
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    for (int i = 0; i < state->user_count; i++) {
        if (strcmp(state->users[i].login, login) == 0) {
            printf("Пользователь с таким логином уже существует.\n");
            pthread_mutex_unlock(&state->mutex);
            return;
        }
    }

    printf("Введите PIN-код (число от 0 до 100000): ");
    if (scanf("%d", &pin) != 1) {
        printf("Ошибка ввода PIN-кода.\n");
        while (getchar() != '\n');
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    if (pin < 0 || pin > 100000) {
        printf("Некорректный PIN-код. Допустимый диапазон: от 0 до 100000.\n");
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    strcpy(state->users[state->user_count].login, login);
    state->users[state->user_count].pin = pin;
    state->users[state->user_count].request_limit = -1;
    state->users[state->user_count].request_count = 0;
    state->user_count++;

    printf("Пользователь успешно зарегистрирован.\n");
    pthread_mutex_unlock(&state->mutex);
}

void authorization(Current* state) {
    pthread_mutex_lock(&state->mutex);
    char login[MAX_LOGIN + 1];
    int pin;

    printf("Авторизация\n");
    printf("Введите логин: ");
    if (scanf("%6s", login) != 1) {
        printf("Ошибка ввода логина.\n");
        while (getchar() != '\n'); 
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    for (int i = 0; i < state->user_count; i++) {
        if (strcmp(state->users[i].login, login) == 0) {
            printf("Введите PIN-код: ");
            if (scanf("%d", &pin) != 1) {
                printf("Ошибка ввода PIN-кода.\n");
                while (getchar() != '\n'); 
                pthread_mutex_unlock(&state->mutex);
                return;
            }

            while (getchar() != '\n');

            if (state->users[i].pin == pin) {
                state->current_user = &state->users[i];
                printf("Добро пожаловать, %s!\n", state->current_user->login);
                pthread_mutex_unlock(&state->mutex);
                return;
            } else {
                printf("Неверный PIN-код.\n");
                pthread_mutex_unlock(&state->mutex);
                return;
            }
        }
    }

    printf("Пользователь с таким логином не найден.\n");
    pthread_mutex_unlock(&state->mutex);
}

void logout(Current* state) {
    state->current_user = NULL;
    printf("Вы вышли из системы.\n");
}

void get_time() {
    time_t now = time(NULL);
    if (now == -1) {
        printf("Ошибка при получении времени.\n");
        return;
    }

    struct tm *tm = localtime(&now);
    if (tm == NULL) {
        printf("Ошибка при преобразовании времени.\n");
        return;
    }

    printf("Текущее время: %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void get_date() {
    time_t now = time(NULL);
    if (now == -1) {
        printf("Ошибка при получении времени.\n");
        return;
    }

    struct tm *tm = localtime(&now);
    if (tm == NULL) {
        printf("Ошибка при преобразовании времени.\n");
        return;
    }

    printf("Текущая дата: %02d:%02d:%04d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year + 1900);
}

void howmuch(const char *time_str, const char *flag) {
    struct tm time_in = {0}; 
    int day, month, year;

    if (sscanf(time_str, "%d:%d:%d", &day, &month, &year) != 3) {
        printf("Некорректный формат даты. Используйте формат дд:мм:гггг.\n");
        return;
    }

    if (day < 1 || day > 31 || month < 1 || month > 12 || year < 1900) {
        printf("Некорректная дата. Проверьте введённые значения.\n");
        return;
    }

    time_in.tm_mday = day;
    time_in.tm_mon = month - 1;  
    time_in.tm_year = year - 1900; 

    time_t first = mktime(&time_in);
    if (first == -1) {
        printf("Ошибка при разборе даты.\n");
        return;
    }

    time_t now = time(NULL);
    if (now == -1) {
        printf("Ошибка при получении текущего времени.\n");
        return;
    }

    double res = difftime(now, first);

    if (strcmp(flag, "-s") == 0) {
        printf("Прошло секунд: %.0f\n", res); 
    } else if (strcmp(flag, "-m") == 0) {
        printf("Прошло минут: %.0f\n", res / 60); 
    } else if (strcmp(flag, "-h") == 0) {
        printf("Прошло часов: %.0f\n", res / 3600); 
    } else if (strcmp(flag, "-y") == 0) {
        printf("Прошло лет: %.0f\n", res / (3600 * 24 * 365)); 
    } else {
        printf("Некорректный флаг. Допустимые флаги: -s, -m, -h, -y.\n");
    }
}

void set_sanctions(Current* state, const char *username, int number) {
    pthread_mutex_lock(&state->mutex);

    if (number < 0) {
        printf("Число должно быть неотрицательным.\n");
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    char confirmation[6];
    printf("Введите 12345 для подтверждения: ");
    if (scanf("%5s", confirmation) != 1) {
        printf("Ошибка ввода подтверждения.\n");
        while (getchar() != '\n'); 
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    while (getchar() != '\n');

    if (strcmp(confirmation, "12345") != 0) {
        printf("Подтверждение не удалось.\n");
        pthread_mutex_unlock(&state->mutex);
        return;
    }

    for (int i = 0; i < state->user_count; i++) {
        if (strcmp(state->users[i].login, username) == 0) {
            state->users[i].request_limit = number;
            printf("Для пользователя %s установлено ограничение в %d запросов.\n", username, number);
            pthread_mutex_unlock(&state->mutex);
            return;
        }
    }

    printf("Пользователь с логином %s не найден.\n", username);
    pthread_mutex_unlock(&state->mutex);
}

typedef struct {
    Current* state;
    char* command;
} Thread_args;

void* process_command_thread(void* arg) {
    Thread_args* args = (Thread_args*)arg;
    Current* state = args->state;
    char* command = args->command;

    pthread_mutex_lock(&state->mutex);
    if (state->current_user == NULL) {
        printf("Пользователь не авторизован.\n");
        pthread_mutex_unlock(&state->mutex);
        free(command);
        free(args);
        pthread_exit(NULL);
    }

    if (state->current_user->request_limit != -1 && 
        state->current_user->request_count >= state->current_user->request_limit) {
        printf("Превышено количество запросов.\n");
        logout(state);
        pthread_mutex_unlock(&state->mutex);
        free(command);
        free(args);
        pthread_exit(NULL);
    }

    state->current_user->request_count++;
    pthread_mutex_unlock(&state->mutex);

    if (strcmp(command, "Time") == 0) {
        get_time();
    } else if (strcmp(command, "Date") == 0) {
        get_date();
    } else if (strncmp(command, "Howmuch", 7) == 0) {
        char time_str[11];
        char flag[3];
        if (sscanf(command, "Howmuch %10s %2s", time_str, flag) == 2) {
            howmuch(time_str, flag);
        } else {
            printf("Некорректный формат команды. Используйте: <Howmuch> <дд:мм:гггг> <-флаг>.\n");
        }
    } else if (strcmp(command, "Logout") == 0) {
        logout(state);
    } else if (strncmp(command, "Sanctions", 9) == 0) {
        char username[MAX_LOGIN + 1];
        int number;

        if (sscanf(command, "Sanctions %6s %d", username, &number) == 2) {
            set_sanctions(state, username, number);
        } else {
            printf("Некорректный формат команды. Используйте: <Sanctions> <логин> <число>.\n");
        }
    } else {
        printf("Некорректная команда.\n");
    }

    free(command);
    free(args);
    pthread_exit(NULL);
}

int main() {
    Current state = {0};
    pthread_mutex_init(&state.mutex, NULL);

    while (1) {
        if (state.current_user == NULL) {
            printf("\n1. Авторизация\n");
            printf("2. Регистрация\n");
            printf("3. Выход\n");
            printf("Выберите действие: ");
    
            int choice;
            if (scanf("%d", &choice) != 1) {
                printf("Ошибка ввода. Введите число.\n");
                while (getchar() != '\n'); 
                continue;
            }
    
            while (getchar() != '\n');
    
            switch (choice) {
                case 1:
                    authorization(&state);
                    break;
                case 2:
                    registration(&state);
                    break;
                case 3:
                    pthread_mutex_destroy(&state.mutex);
                    return 0; 
                default:
                    printf("Некорректное действие. Введите число от 1 до 3.\n");
            }
        } else {
            while (state.current_user != NULL) {
                char command[128];

                printf("@%s: ", state.current_user->login);
                if (fgets(command, sizeof(command), stdin) == NULL) {
                    printf("Ошибка ввода команды.\n");
                    continue;
                }

                command[strcspn(command, "\n")] = '\0';

                if (strlen(command) == 0) {
                    continue;
                }

                pthread_mutex_lock(&state.mutex);
                if (state.current_user->request_limit != -1 && 
                    state.current_user->request_count >= state.current_user->request_limit) {
                    printf("Превышено количество запросов.\n");
                    logout(&state);
                    pthread_mutex_unlock(&state.mutex);
                    break;
                }
                pthread_mutex_unlock(&state.mutex);

                pthread_t thread;
                char* command_copy = strdup(command);
                if (command_copy == NULL) {
                    printf("Ошибка выделения памяти.\n");
                    continue;
                }
                
                Thread_args* args = malloc(sizeof(Thread_args));
                if (args == NULL) {
                    printf("Ошибка выделения памяти.\n");
                    free(command_copy);
                    continue;
                }
                args->state = &state;
                args->command = command_copy;

                if (pthread_create(&thread, NULL, process_command_thread, args) != 0) {
                    printf("Ошибка создания потока.\n");
                    free(command_copy);
                    free(args);
                    continue;
                }

                pthread_join(thread, NULL);

                if (state.current_user == NULL) {
                    break;
                }
            }
        }
    }
}