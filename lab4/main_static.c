#include "utils.h"
#include "contracts.h"
#include <ctype.h>

int main() {
    char buffer[1000];
    
    print_str("Программа со статической линковкой\n");
    print_str("Команды:\n");
    print_str("  1 k - вычисление pi (ряд Лейбница)\n");
    print_str("  2 x - перевод в двоичную систему\n");
    print_str("  9 - выход\n\n");
    
    while (1) {
        print_str("> ");
        
        read_line(buffer, sizeof(buffer));
        
        if (buffer[0] == '\0') continue;
        
        if (buffer[0] == '9') {
            break;
        }
        else if (buffer[0] == '1') {
            char *arg = buffer + 1;
            while (*arg == ' ') arg++;
            
            if (*arg == '\0') {
                print_str("Ошибка: введите значение k\n");
                continue;
            }
            
            if (!is_valid_int(arg)) {
                print_str("Ошибка: некорректное значение k (допустимы только цифры, возможен знак в начале)\n");
                continue;
            }
            
            int k = parse_int(arg);
            
            if (!is_valid_pi_k(k)) {
                continue;
            }
            
            float result = pi(k);
            
            print_str("pi(");
            print_int(k);
            print_str(") = ");
            print_float(result);
            print_str("\n");
        }
        else if (buffer[0] == '2') {
            char *arg = buffer + 1;
            while (*arg == ' ') arg++;
            
            if (*arg == '\0') {
                print_str("Ошибка: введите число для перевода\n");
                continue;
            }

            if (*arg == '\0' || (*arg != '-' && !isdigit(*arg))) {
                print_str("Ошибка: некорректный ввод. Ожидается целое число\n");
                continue;
            }
            
            if (!is_valid_int(arg)) {
                print_str("Ошибка: некорректное число (переполнение или недопустимые символы)\n");
                continue;
            }
            
            int x = parse_int(arg);
            
            char* result = convert(x);
            
            if (result != NULL) {
                print_str("convert(");
                print_int(x);
                print_str(") = ");
                print_str(result);
                print_str("\n");
                free(result);
            } else {
                print_str("Ошибка: не удалось выполнить перевод\n");
            }
        }
        else {
            print_str("Неизвестная команда. Используйте 1, 2 или 9\n");
        }
    }
    
    return 0;
}