#include <semaphore.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>    

#define BUFFER_SIZE 10
#define SHM_BASE_NAME "/lab3_shm"
#define SEM_EMPTY_BASE_NAME "/lab3_empty"
#define SEM_FULL_BASE_NAME "/lab3_full"
#define SEM_MUTEX_BASE_NAME "/lab3_mutex"

struct shared_data {
    int data[BUFFER_SIZE];
    int client_is_over;
    int server_is_over;
    int write_index;
    int read_index;
};

int StringToInt(const char* str, int* success) {
    long long result = 0;
    int sign = 1;
    size_t index = 0;
    *success = 1;
    
    if (str[0] == '-') {
        sign = -1;
        index = 1;
    }
    
    while (str[index] != '\0') {
        if (str[index] >= '0' && str[index] <= '9') {
            if (result > (2147483647LL - (str[index] - '0')) / 10) {
                *success = 0;
                return 0;
            }
            result = result * 10 + (str[index] - '0');
        }
        index = index + 1;
    }
    
    result = result * sign;
    
    const int MAX_INT = 2147483647;
    const int MIN_INT = -2147483648;
    if (result > MAX_INT || result < MIN_INT) {
        *success = 0;
        return 0;
    }
    
    return (int)result;
}

void IntToString(int number, char* buffer) {
    int position = 0;
    
    if (number < 0) {
        buffer[position] = '-';
        position++;
        number = -number;
    }
    
    int tempNumber = number;
    int digitCount = 0;
    do { 
        digitCount = digitCount + 1; 
        tempNumber = tempNumber / 10; 
    } while (tempNumber != 0);
    

    for (int digitIndex = digitCount - 1; digitIndex >= 0; digitIndex = digitIndex - 1) {
        buffer[position + digitIndex] = '0' + (number % 10);
        number = number / 10;
    }
    position = position + digitCount;
    buffer[position] = '\0';
}

int IsPrime(int number) {
    if (number <= 1) {
        return 0;
    }
    if (number == 2) {
        return 1;
    }
    if (number % 2 == 0) {
        return 0;
    }
    for (int i = 3; i * i <= number; i += 2) {
        if (number % i == 0) {
            return 0;
        }
    }
    return 1;
}

void WriteToFile(int fileDescriptor, const char* str) {
    size_t length = 0;
    while (str[length] != '\0') {
        length++;
    }
    write(fileDescriptor, str, length);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        WriteToFile(STDERR_FILENO, "Использование: ./server <output_file>\n");
        return 1;
    }

    pid_t pid = getpid();
    char pid_str[20];
    IntToString(pid, pid_str);
    
    char shm_name[500];
    char sem_empty_name[500];
    char sem_full_name[500];
    char sem_mutex_name[500];

    snprintf(shm_name, sizeof(shm_name), "%s_%s", SHM_BASE_NAME, pid_str);
    snprintf(sem_empty_name, sizeof(sem_empty_name), "%s_%s", SEM_EMPTY_BASE_NAME, pid_str);
    snprintf(sem_full_name, sizeof(sem_full_name), "%s_%s", SEM_FULL_BASE_NAME, pid_str);
    snprintf(sem_mutex_name, sizeof(sem_mutex_name), "%s_%s", SEM_MUTEX_BASE_NAME, pid_str);

    shm_unlink(shm_name);
    sem_unlink(sem_empty_name);
    sem_unlink(sem_full_name);
    sem_unlink(sem_mutex_name);

    int outputFile = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (outputFile == -1) {
        WriteToFile(STDERR_FILENO, "Ошибка открытия файла\n");
        return 1;
    }

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        char error_msg[1024];
        snprintf(error_msg, sizeof(error_msg), "Ошибка shm_open: %s\n", shm_name);
        WriteToFile(STDERR_FILENO, error_msg);
        return 1;
    }

    if(ftruncate(shm_fd, sizeof(struct shared_data)) == -1) {
        WriteToFile(STDERR_FILENO, "Ошибка ftruncate\n");
        close(shm_fd);
        return 1;
    }

    struct shared_data* shm_ptr = mmap(NULL, sizeof(struct shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    close(shm_fd);
    
    if (shm_ptr == MAP_FAILED) {
        WriteToFile(STDERR_FILENO, "Ошибка mmap\n");
        return 1;
    }

    shm_ptr->client_is_over = 0;
    shm_ptr->server_is_over = 0;
    shm_ptr->write_index = 0;
    shm_ptr->read_index = 0;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        shm_ptr->data[i] = 0;
    }

    sem_t* sem_empty = sem_open(sem_empty_name, O_CREAT, 0666, BUFFER_SIZE);
    if (sem_empty == SEM_FAILED) {
        char error_msg[1024];
        snprintf(error_msg, sizeof(error_msg), "Ошибка sem_open (empty): %s\n", sem_empty_name);
        WriteToFile(STDERR_FILENO, error_msg);
        munmap(shm_ptr, sizeof(struct shared_data));
        return 1;
    }

    sem_t* sem_full = sem_open(sem_full_name, O_CREAT, 0666, 0);
    if (sem_full == SEM_FAILED) {
        char error_msg[1024];
        snprintf(error_msg, sizeof(error_msg), "Ошибка sem_open (full): %s\n", sem_full_name);
        WriteToFile(STDERR_FILENO, error_msg);
        sem_close(sem_empty);
        munmap(shm_ptr, sizeof(struct shared_data));
        return 1;
    }

    sem_t* sem_mutex = sem_open(sem_mutex_name, O_CREAT, 0666, 1);
    if (sem_mutex == SEM_FAILED) {
        char error_msg[1024];
        snprintf(error_msg, sizeof(error_msg), "Ошибка sem_open (mutex): %s\n", sem_mutex_name);
        WriteToFile(STDERR_FILENO, error_msg);
        sem_close(sem_empty);
        sem_close(sem_full);
        munmap(shm_ptr, sizeof(struct shared_data));
        return 1;
    }

    pid_t client_pid = fork();
    if (client_pid == 0) {
        execl("./client", "client", shm_name, sem_empty_name, sem_full_name, sem_mutex_name, NULL);
        
        WriteToFile(STDERR_FILENO, "Ошибка запуска клиента\n");
        _exit(1);
    } else if (client_pid < 0) {
        WriteToFile(STDERR_FILENO, "Ошибка fork\n");
        sem_close(sem_empty);
        sem_close(sem_full);
        sem_close(sem_mutex);
        munmap(shm_ptr, sizeof(struct shared_data));
        return 1;
    }

    int should_exit = 0;
    int processed_count = 0;
    
    while (!should_exit) {
        sem_wait(sem_full);
        sem_wait(sem_mutex);

        if (shm_ptr->client_is_over == 1 && shm_ptr->read_index == shm_ptr->write_index) {
            sem_post(sem_mutex);
            break;
        }

        int number = shm_ptr->data[shm_ptr->read_index];
        shm_ptr->read_index = (shm_ptr->read_index + 1) % BUFFER_SIZE;
        
        sem_post(sem_mutex);
        sem_post(sem_empty);

        if (number < 0) {
            char msg[500];
            snprintf(msg, sizeof(msg), "Сервер: отрицательное число %d - завершение\n", number);
            WriteToFile(STDOUT_FILENO, msg);
            should_exit = 1;
        } else if (IsPrime(number)) {
            char msg[500];
            snprintf(msg, sizeof(msg), "Сервер: простое число %d - завершение\n", number);
            WriteToFile(STDOUT_FILENO, msg);
            should_exit = 1;
        } else {
            char num_str[100];
            IntToString(number, num_str);
            WriteToFile(outputFile, num_str);
            WriteToFile(outputFile, "\n");
            processed_count++;
        }
        
        if (processed_count > 1000) {
            WriteToFile(STDOUT_FILENO, "Сервер: обработано максимальное количество чисел\n");
            break;
        }

        if (should_exit) {
            sem_wait(sem_mutex);
            shm_ptr->server_is_over = 1;
            sem_post(sem_mutex);
            
            for (int i = 0; i < BUFFER_SIZE; i++) {
                sem_post(sem_full);
            }
            break;
        }
    }

    sem_wait(sem_mutex);
    shm_ptr->server_is_over = 1;
    sem_post(sem_mutex);
    sem_post(sem_full);

    waitpid(client_pid, NULL, 0);

    sem_close(sem_full);
    sem_close(sem_empty);
    sem_close(sem_mutex);
    
    munmap(shm_ptr, sizeof(struct shared_data));
    close(outputFile);
    
    shm_unlink(shm_name);
    sem_unlink(sem_empty_name);
    sem_unlink(sem_full_name);
    sem_unlink(sem_mutex_name);
    
    return 0;
}