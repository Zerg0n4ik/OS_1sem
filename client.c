#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>

// Проверка что строка - корректное целое число
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
        index = index + 1;
    }
    return 1;
}

// Чтение строки из стандартного ввода
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
        position = position + 1;
    }
    
    buffer[size - 1] = '\0';
    return 1;
}

// Функция для безопасной записи строки
void WriteString(int fileDescriptor, const char* str) {
    size_t length = 0;
    while (str[length] != '\0') {
        length = length + 1;
    }
    write(fileDescriptor, str, length);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        WriteString(STDERR_FILENO, "Usage: client <filename>\n");
        return 1;
    }

    // Создаем два pipe
    int pipe1[2]; // родитель -> дочерний (числа)
    int pipe2[2]; // дочерний -> родитель (сигналы)
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        WriteString(STDERR_FILENO, "Error: failed to create pipes\n");
        return 1;
    }

    pid_t childPid = fork();
    
    if (childPid == -1) {
        WriteString(STDERR_FILENO, "Error: failed to fork\n");
        return 1;
    }

    if (childPid == 0) {
        // Дочерний процесс
        close(pipe1[1]); // закрываем запись в pipe1
        close(pipe2[0]); // закрываем чтение из pipe2
        
        // Перенаправляем stdin на pipe1, stdout на pipe2
        dup2(pipe1[0], STDIN_FILENO);
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe1[0]);
        close(pipe2[1]);
        
        // Запускаем сервер
        char* args[] = {"server", argv[1], NULL};
        execv("./server", args);
        
        // Если execv не удался
        WriteString(STDERR_FILENO, "Error: failed to execute server\n");
        exit(1);
        
    } else {
        // Родительский процесс
        close(pipe1[0]); // закрываем чтение из pipe1
        close(pipe2[1]); // закрываем запись в pipe2
        
        WriteString(STDOUT_FILENO, "Enter numbers (one per line, empty line to exit):\n");
        
        char inputBuffer[256];
        
        while (1) {
            WriteString(STDOUT_FILENO, "> ");
            
            if (!ReadInput(inputBuffer, sizeof(inputBuffer))) {
                break;
            }
            
            // Пустая строка - завершаем работу
            if (inputBuffer[0] == '\0') {
                WriteString(STDOUT_FILENO, "Empty line - exiting...\n");
                break;
            }
            
            // Проверяем корректность числа
            if (!IsValidInteger(inputBuffer)) {
                WriteString(STDOUT_FILENO, "Error: invalid integer. Exiting...\n");
                break;
            }
            
            // Отправляем число дочернему процессу
            size_t inputLength = 0;
            while (inputBuffer[inputLength] != '\0') {
                inputLength = inputLength + 1;
            }
            
            char numberMessage[256];
            for (size_t index = 0; index < inputLength; index = index + 1) {
                numberMessage[index] = inputBuffer[index];
            }
            numberMessage[inputLength] = '\n';
            
            write(pipe1[1], numberMessage, inputLength + 1);
            
            // Ждем немного чтобы дочерний процесс успел обработать число
            usleep(10000); // 10ms
            
            // Проверяем, не завершился ли дочерний процесс
            int childStatus;
            pid_t waitResult = waitpid(childPid, &childStatus, WNOHANG);
            if (waitResult == childPid) {
                WriteString(STDOUT_FILENO, "Prime or negative number detected. Processes terminated.\n");
                break;
            }
            
            // Читаем ответ от дочернего процесса
            char responseBuffer[16];
            int bytesRead = read(pipe2[0], responseBuffer, sizeof(responseBuffer) - 1);
            
            if (bytesRead > 0) {
                responseBuffer[bytesRead] = '\0';
                
                // Проверяем, не отправил ли дочерний процесс сигнал EXIT
                if (bytesRead >= 4 && 
                    responseBuffer[0] == 'E' && responseBuffer[1] == 'X' && 
                    responseBuffer[2] == 'I' && responseBuffer[3] == 'T') {
                    WriteString(STDOUT_FILENO, "Prime or negative number detected. Processes terminated.\n");
                    break;
                }
            }
        }
        
        // Завершаем дочерний процесс если он еще работает
        close(pipe1[1]);
        close(pipe2[0]);
        kill(childPid, SIGTERM);
        wait(NULL);
    }
    
    return 0;
}