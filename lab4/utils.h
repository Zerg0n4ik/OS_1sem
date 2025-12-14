#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <string.h>

void print_str(const char *s);
void print_int(int num);
void print_float(float num);
void read_line(char *buffer, int max_size);

float parse_float(const char* str);
int parse_int(const char* str);

int is_valid_int(const char *str);
int is_valid_float(const char *str);
int is_valid_pi_k(int k);

#endif