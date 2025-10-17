#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

// Конвертация строки в число с проверкой переполнения
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
            // Проверка на переполнение
            if (result > (2147483647LL - (str[index] - '0')) / 10) {
                *success = 0;
                return 0;
            }
            result = result * 10 + (str[index] - '0');
        }
        index = index + 1;
    }
    
    result = result * sign;
    
    // Проверка на выход за границы int
    const int MAX_INT = 2147483647;
    const int MIN_INT = -2147483648;
    if (result > MAX_INT || result < MIN_INT) {
        *success = 0;
        return 0;
    }
    
    return (int)result;
}

// Конвертация числа в строку
void IntToString(int number, char* buffer) {
    int position = 0;
    
    if (number < 0) {
        buffer[position] = '-';
        position = position + 1;
        number = -number;
    }
    
    // Определяем количество цифр
    int tempNumber = number;
    int digitCount = 0;
    do { 
        digitCount = digitCount + 1; 
        tempNumber = tempNumber / 10; 
    } while (tempNumber != 0);
    
    // Записываем цифры
    for (int digitIndex = digitCount - 1; digitIndex >= 0; digitIndex = digitIndex - 1) {
        buffer[position + digitIndex] = '0' + (number % 10);
        number = number / 10;
    }
    position = position + digitCount;
    buffer[position] = '\0';
}

// Проверка числа на простоту
int IsPrime(int number) {
    if (number <= 1) {
        return 0;
    }
    if (number <= 3) {
        return 1;
    }
    if (number % 2 == 0 || number % 3 == 0) {
        return 0;
    }
    
    for (int divisor = 5; divisor * divisor <= number; divisor = divisor + 6) {
        if (number % divisor == 0 || number % (divisor + 2) == 0) {
            return 0;
        }
    }
    return 1;
}

// Запись строки в файл
void WriteToFile(int fileDescriptor, const char* str) {
    size_t length = 0;
    while (str[length] != '\0') {
        length = length + 1;
    }
    write(fileDescriptor, str, length);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        WriteToFile(STDERR_FILENO, "Usage: server <filename>\n");
        return 1;
    }

    // Открываем файл для записи
    int outputFile = open(argv[1], O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (outputFile == -1) {
        WriteToFile(STDERR_FILENO, "Error: failed to open file\n");
        return 1;
    }

    char inputBuffer[256];
    ssize_t bytesRead;
    
    while ((bytesRead = read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer) - 1)) > 0) {
        inputBuffer[bytesRead] = '\0';
        
        // Убираем символ новой строки
        if (bytesRead > 0 && inputBuffer[bytesRead - 1] == '\n') {
            inputBuffer[bytesRead - 1] = '\0';
        }
        
        // Пропускаем пустые строки
        if (inputBuffer[0] == '\0') {
            continue;
        }
        
        // Преобразуем в число с проверкой переполнения
        int conversionSuccess;
        int number = StringToInt(inputBuffer, &conversionSuccess);
        
        if (!conversionSuccess) {
            WriteToFile(STDOUT_FILENO, "EXIT\n");
            close(outputFile);
            exit(0);
        }
        
        // Проверяем условия по заданию:
        if (number < 0 || IsPrime(number)) {
            // НЕ записываем в файл! Просто завершаем процессы
            close(outputFile);
            
            // Отправляем сигнал родителю перед выходом
            WriteToFile(STDOUT_FILENO, "EXIT\n");
            exit(0);
        } 
        // Если число составное - записываем в файл
        else {
            WriteToFile(outputFile, "Composite: ");
            char numberString[32];
            IntToString(number, numberString);
            WriteToFile(outputFile, numberString);
            WriteToFile(outputFile, "\n");
            
            // Подтверждаем обработку
            WriteToFile(STDOUT_FILENO, "OK\n");
        }
    }

    close(outputFile);
    return 0;
}