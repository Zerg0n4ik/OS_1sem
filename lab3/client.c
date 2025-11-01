#include <stdio.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <semaphore.h>

#define BUFFER_SIZE 10

struct shared_data {
    int data[BUFFER_SIZE];
    int client_is_over;
    int server_is_over;
    int write_index;
    int read_index;
};

int IsValidInteger(const char* str) {
    if (str == NULL || str[0] == '\0') {
        return 0;
    }
    
    size_t startIndex = 0;
    if (str[0] == '-') {
        if (str[1] == '\0') {
            return 0;
        }
        startIndex = 1;
    }
    
    size_t index = startIndex;
    while (str[index] != '\0') {
        if (str[index] < '0' || str[index] > '9') {
            return 0;
        }
        index++;
    }
    return 1;
}

int ReadInput(char* buffer, size_t size) {
    size_t position = 0;
    
    while (position < size - 1) {
        ssize_t bytesRead = read(STDIN_FILENO, buffer + position, 1);
        if (bytesRead <= 0) {
            return 0;
        }
        
        if (buffer[position] == '\n') {
            buffer[position] = '\0';
            return 1;
        }
        position++;
    }
    
    buffer[size - 1] = '\0';
    return 1;
}

void WriteString(int fileDescriptor, const char* str) {
    size_t length = 0;
    while (str[length] != '\0') {
        length++;
    }
    write(fileDescriptor, str, length);
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        WriteString(STDERR_FILENO, "Использование: ./client <shm_name> <sem_empty> <sem_full> <sem_mutex>\n");
        return 1;
    }

    const char* shm_name = argv[1];
    const char* sem_empty_name = argv[2];
    const char* sem_full_name = argv[3];
    const char* sem_mutex_name = argv[4];

    int shm_fd = shm_open(shm_name, O_RDWR, 0666);
    if (shm_fd == -1) {
        char error_msg[500];
        snprintf(error_msg, sizeof(error_msg), "Ошибка shm_open: %s\n", shm_name);
        WriteString(STDERR_FILENO, error_msg);
        return 1;
    }

    struct shared_data* shm_ptr = mmap(NULL, sizeof(struct shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    
    if (shm_ptr == MAP_FAILED) {
        WriteString(STDERR_FILENO, "Ошибка mmap\n");
        return 1;
    }

    sem_t* sem_empty = sem_open(sem_empty_name, 0);
    if (sem_empty == SEM_FAILED) {
        char error_msg[500];
        snprintf(error_msg, sizeof(error_msg), "Ошибка sem_open (empty): %s\n", sem_empty_name);
        WriteString(STDERR_FILENO, error_msg);
        munmap(shm_ptr, sizeof(struct shared_data));
        return 1;
    }

    sem_t* sem_full = sem_open(sem_full_name, 0);
    if (sem_full == SEM_FAILED) {
        char error_msg[500];
        snprintf(error_msg, sizeof(error_msg), "Ошибка sem_open (full): %s\n", sem_full_name);
        WriteString(STDERR_FILENO, error_msg);
        sem_close(sem_empty);
        munmap(shm_ptr, sizeof(struct shared_data));
        return 1;
    }

    sem_t* sem_mutex = sem_open(sem_mutex_name, 0);
    if (sem_mutex == SEM_FAILED) {
        char error_msg[500];
        snprintf(error_msg, sizeof(error_msg), "Ошибка sem_open (mutex): %s\n", sem_mutex_name);
        WriteString(STDERR_FILENO, error_msg);
        sem_close(sem_empty);
        sem_close(sem_full);
        munmap(shm_ptr, sizeof(struct shared_data));
        return 1;
    }

    int should_exit = 0;
    char input_buff[BUFSIZ];
    
    while (!should_exit) {
        WriteString(STDOUT_FILENO, "> ");
        
        if (!ReadInput(input_buff, sizeof(input_buff))) {
            WriteString(STDOUT_FILENO, "Ошибка чтения ввода\n");
            break;
        }
        
        if (input_buff[0] == '\0') {
            WriteString(STDOUT_FILENO, "Пустой ввод - завершение\n");
            break;
        }
        
        if (!IsValidInteger(input_buff)) {
            WriteString(STDOUT_FILENO, "Неподходящий ввод - завершение\n");
            break;
        }
        
        int number = atoi(input_buff);

        sem_wait(sem_empty);
        sem_wait(sem_mutex);

        if (shm_ptr->server_is_over) {
            sem_post(sem_mutex);
            sem_post(sem_empty);
            WriteString(STDOUT_FILENO, "Сервер завершил работу - клиент также завершает работу\n");
            break;
        }

        shm_ptr->data[shm_ptr->write_index] = number;
        shm_ptr->write_index = (shm_ptr->write_index + 1) % BUFFER_SIZE;

        sem_post(sem_mutex);
        sem_post(sem_full);

        struct timespec ts = {0, 50000000};
        nanosleep(&ts, NULL);
        
        sem_wait(sem_mutex);
        if (shm_ptr->server_is_over) {
            sem_post(sem_mutex);
            WriteString(STDOUT_FILENO, "Сервер завершил работу - клиент также завершает работу\n");
            break;
        }
        sem_post(sem_mutex);
    }

    sem_wait(sem_mutex);
    shm_ptr->client_is_over = 1;
    sem_post(sem_mutex);
    sem_post(sem_full);

    sem_close(sem_empty);
    sem_close(sem_full);
    sem_close(sem_mutex);
    munmap(shm_ptr, sizeof(struct shared_data));
    
    return 0;
}