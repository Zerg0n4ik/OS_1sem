#include "utils.h"
#include <ctype.h>
#include <dlfcn.h>

typedef float (*pi_func_t)(int);
typedef char* (*convert_func_t)(int);

int main() {
    void* pi_handle = NULL;
    void* convert_handle = NULL;
    pi_func_t pi_func = NULL;
    convert_func_t convert_func = NULL;
    
    int pi_lib = 1;
    int convert_lib = 1;
    
    char buffer[1000];
    
    print_str("Программа с динамической загрузкой\n");
    print_str("Команды:\n");
    print_str("  0 - переключить библиотеки\n");
    print_str("  1 k - вычисление pi (текущая: ");
    print_str(pi_lib == 1 ? "Лейбниц" : "Валлис");
    print_str(")\n");
    print_str("  2 x - перевод системы (текущая: ");
    print_str(convert_lib == 1 ? "двоичная" : "троичная");
    print_str(")\n");
    print_str("  9 - выход\n\n");
    
    pi_handle = dlopen("./libpi_leibniz.so", RTLD_LAZY);
    if (!pi_handle) {
        print_str("Ошибка загрузки pi библиотеки\n");
        return 1;
    }
    
    convert_handle = dlopen("./libconvert_binary.so", RTLD_LAZY);
    if (!convert_handle) {
        print_str("Ошибка загрузки convert библиотеки\n");
        dlclose(pi_handle);
        return 1;
    }
    
    pi_func = (pi_func_t)dlsym(pi_handle, "pi");
    convert_func = (convert_func_t)dlsym(convert_handle, "convert");
    
    if (!pi_func || !convert_func) {
        print_str("Ошибка получения функций\n");
        dlclose(pi_handle);
        dlclose(convert_handle);
        return 1;
    }
    
    while (1) {
        print_str("> ");
        read_line(buffer, sizeof(buffer));
        
        if (buffer[0] == '\0') continue;
        
        if (buffer[0] == '9') {
            break;
        }
        else if (buffer[0] == '0') {
            dlclose(pi_handle);
            dlclose(convert_handle);
            
            if (pi_lib == 1) {
                pi_handle = dlopen("./libpi_wallis.so", RTLD_LAZY);
                pi_lib = 2;
            } else {
                pi_handle = dlopen("./libpi_leibniz.so", RTLD_LAZY);
                pi_lib = 1;
            }
            
            if (convert_lib == 1) {
                convert_handle = dlopen("./libconvert_ternary.so", RTLD_LAZY);
                convert_lib = 2;
            } else {
                convert_handle = dlopen("./libconvert_binary.so", RTLD_LAZY);
                convert_lib = 1;
            }
            
            if (!pi_handle || !convert_handle) {
                print_str("Ошибка загрузки библиотек\n");
                return 1;
            }
            
            pi_func = (pi_func_t)dlsym(pi_handle, "pi");
            convert_func = (convert_func_t)dlsym(convert_handle, "convert");
            
            print_str("Переключено: pi - ");
            print_str(pi_lib == 1 ? "Лейбниц" : "Валлис");
            print_str(", convert - ");
            print_str(convert_lib == 1 ? "двоичная" : "троичная");
            print_str("\n");
        }
        else if (buffer[0] == '1') {
            char *arg = buffer + 1;
            while (*arg == ' ') arg++;
            
            if (*arg == '\0') {
                print_str("Ошибка: введите значение k\n");
                continue;
            }
            
            if (*arg == '\0' || (*arg != '-' && !isdigit(*arg))) {
                print_str("Ошибка: некорректный ввод. Ожидается целое число\n");
                continue;
            }
            
            if (!is_valid_int(arg)) {
                print_str("Ошибка: некорректное значение k\n");
                continue;
            }
            
            int k = parse_int(arg);
            
            if (!is_valid_pi_k(k)) {
                continue;
            }
            
            float result = pi_func(k);
            
            print_str("pi(");
            print_int(k);
            print_str(") = ");
            print_float(result);
            print_str(" (алгоритм: ");
            print_str(pi_lib == 1 ? "Лейбниц" : "Валлис");
            print_str(")\n");
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
            
            char* result = convert_func(x);
            
            if (result != NULL) {
                print_str("convert(");
                print_int(x);
                print_str(") = ");
                print_str(result);
                print_str(" (система: ");
                print_str(convert_lib == 1 ? "двоичная" : "троичная");
                print_str(")\n");
                free(result);
            } else {
                print_str("Ошибка: не удалось выполнить перевод\n");
            }
        }
        else {
            print_str("Неизвестная команда. Используйте 0, 1, 2 или 9\n");
        }
    }
    
    if (pi_handle) dlclose(pi_handle);
    if (convert_handle) dlclose(convert_handle);

    return 0;
}