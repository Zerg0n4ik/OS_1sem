#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>


void print_str(const char *s) {
    write(1, s, strlen(s));
}

void print_int(int num) {
    char buffer[32];
    sprintf(buffer, "%d", num);
    write(1, buffer, strlen(buffer));
}

void print_float(float num) {
    char buffer[32];
    sprintf(buffer, "%.6f", num); 
    write(1, buffer, strlen(buffer));
}


void read_line(char *buffer, int max_size) {
    int bytes_read = read(0, buffer, max_size - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        char *newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
    } else {
        buffer[0] = '\0';
    }
}

int is_valid_int(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    
    int i = 0;
    int has_digits = 0;
    int negative = 0;
    long long result = 0;
    
 
    while (isspace(str[i])) i++;

    if (str[i] == '-') {
        negative = 1;
        i++;
    } else if (str[i] == '+') {
        i++;
    }

    while (str[i] != '\0') {
        if (isdigit(str[i])) {
            has_digits = 1;
            

            if (result > (INT_MAX - (str[i] - '0')) / 10) {
                return 0;
            }
            
            result = result * 10 + (str[i] - '0');
        } else if (isspace(str[i])) {

            while (isspace(str[i])) i++;
            if (str[i] != '\0') return 0;
            break;
        } else {
            return 0;
        }
        i++;
    }
    
    if (!has_digits) return 0;

    if (negative) {
        result = -result;
        if (result < INT_MIN) return 0;
    } else {
        if (result > INT_MAX) return 0;
    }
    
    return 1;
}

int parse_int(const char* str) {
    const char *num_str = str;
    while (*num_str == ' ' || *num_str == '\t') {
        num_str++;
    }
    
    if (*num_str == '\0') {
        return 0;
    }

    if (!is_valid_int(num_str)) {
        return 0;
    }
    
    return atoi(num_str);
}

int is_valid_float(const char *str) {
    if (str == NULL || *str == '\0') {
        return 0;
    }
    
    int i = 0;
    int has_digits = 0;
    int has_dot = 0;
    int has_exp = 0;
    
    while (isspace(str[i])) i++;
    
    if (str[i] == '-' || str[i] == '+') {
        i++;
    }
    while (str[i] != '\0') {
        if (str[i] == '.') {
            if (has_dot || has_exp) return 0;
            has_dot = 1;
        } else if (str[i] == 'e' || str[i] == 'E') {
            if (has_exp) return 0;
            has_exp = 1;
            has_dot = 0;
            if (str[i+1] == '-' || str[i+1] == '+') i++;
        } else if (isdigit(str[i])) {
            has_digits = 1;
        } else if (isspace(str[i])) {
            while (isspace(str[i])) i++;
            if (str[i] != '\0') return 0;
            break;
        } else {
            return 0;
        }
        i++;
    }
    
    return has_digits;
}

float parse_float(const char* str) {
    const char *num_str = str;
    while (*num_str == ' ' || *num_str == '\t') {
        num_str++;
    }

    if (*num_str == '\0') {
        return 0.0f;
    }
    
    if (!is_valid_float(num_str)) {
        return 0.0f;
    }

    return atof(num_str);
}

int is_valid_pi_k(int k) {
    if (k <= 0) {
        print_str("Ошибка: k должно быть положительным числом\n");
        return 0;
    }
    if (k > 1000000) {
        print_str("Ошибка: слишком большое значение k (максимум 1000000)\n");
        return 0;
    }
    return 1;
}
