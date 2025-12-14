#include "contracts.h"


float pi(int k) {
    if (k <= 0) return 0.0f;
    
    float result = 1.0f;
    
    for (int i = 1; i <= k; i++) {
        float numerator = 4.0f * i * i;
        float denominator = 4.0f * i * i - 1.0f;
        result *= numerator / denominator;
    }
    
    return 2.0f * result;
}