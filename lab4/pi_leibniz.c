#include "contracts.h"


float pi(int k) {
    if (k <= 0) return 0.0f;
    
    float result = 0.0f;
    int sign = 1;
    
    for (int i = 0; i < k; i++) {
        result += (float)sign / (2.0f * i + 1.0f);
        sign = -sign;
    }
    
    return 4.0f * result;
}