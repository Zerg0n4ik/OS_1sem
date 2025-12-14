#include <stdlib.h>
#include <string.h>
#include "contracts.h"

char* convert(int x) {
    if (x == 0) {
        char* result = malloc(2);
        if (!result) return NULL;
        result[0] = '0';
        result[1] = '\0';
        return result;
    }
    
    int negative = x < 0;
    unsigned int n = negative ? -x : x;

    unsigned int temp = n;
    int digits = 0;
    while (temp > 0) {
        temp /= 3;
        digits++;
    }

    char* result = malloc(negative ? digits + 2 : digits + 1);
    if (!result) return NULL;
    
    int pos = 0;
    if (negative) {
        result[pos++] = '-';
    }

    for (int i = digits - 1; i >= 0; i--) {
        result[pos + i] = (n % 3) + '0';
        n /= 3;
    }
    
    result[pos + digits] = '\0';
    return result;
}