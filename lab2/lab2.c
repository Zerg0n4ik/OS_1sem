#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>

typedef struct {
    int start;
    int end;
    int thread_id;
} thread_data_t;

pthread_mutex_t barrier_mutex;
pthread_cond_t phase_complete;
int threads_ready;
int total_threads;
int current_phase;

int* global_array;
int global_size;

void barrier_wait(int phase) {
    pthread_mutex_lock(&barrier_mutex);
    
    threads_ready++;
    
    if (threads_ready == total_threads) {
        threads_ready = 0;
        current_phase = phase + 1; 
        pthread_cond_broadcast(&phase_complete);
    } else {
        while (current_phase == phase) {
            pthread_cond_wait(&phase_complete, &barrier_mutex);
        }
    }
    
    pthread_mutex_unlock(&barrier_mutex);
}

void* odd_even_sort_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int* arr = global_array;
    int n = global_size;
    
    for (int phase = 0; phase < n; phase++) {
        if (phase % 2 == 0) {
            for (int i = data->start; i < data->end; i++) {
                if (i % 2 == 0 && i + 1 < n) {
                    if (arr[i] > arr[i + 1]) {
                        int temp = arr[i];
                        arr[i] = arr[i + 1];
                        arr[i + 1] = temp;
                    }
                }
            }
        } else {
            for (int i = data->start; i < data->end; i++) {
                if (i % 2 == 1 && i + 1 < n) {
                    if (arr[i] > arr[i + 1]) {
                        int temp = arr[i];
                        arr[i] = arr[i + 1];
                        arr[i + 1] = temp;
                    }
                }
            }
        }
        
        barrier_wait(phase);
    }
    
    return NULL;
}

void sequential_odd_even_sort(int* arr, int n) {
    int sorted;
    int phase;
    
    for (phase = 0; phase < n; phase++) {
        sorted = 1;
        
        if (phase % 2 == 0) {
            for (int i = 0; i < n - 1; i += 2) {
                if (arr[i] > arr[i + 1]) {
                    int temp = arr[i];
                    arr[i] = arr[i + 1];
                    arr[i + 1] = temp;
                    sorted = 0;
                }
            }
        } else {
            for (int i = 1; i < n - 1; i += 2) {
                if (arr[i] > arr[i + 1]) {
                    int temp = arr[i];
                    arr[i] = arr[i + 1];
                    arr[i + 1] = temp;
                    sorted = 0;
                }
            }
        }
        
        if (sorted) {
            break;
        }
    }
}

void parallel_odd_even_sort(int* arr, int n, int num_threads) {
    global_array = arr;
    global_size = n;
    
    pthread_mutex_init(&barrier_mutex, NULL);
    pthread_cond_init(&phase_complete, NULL);
    
    threads_ready = 0;
    current_phase = 0;
    total_threads = num_threads;
    
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    thread_data_t* thread_data = (thread_data_t*)malloc(num_threads * sizeof(thread_data_t));
    
    int elements_per_thread = (n + num_threads - 1) / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].start = i * elements_per_thread;
        thread_data[i].end = (i + 1) * elements_per_thread;
        if (thread_data[i].end > n) {
            thread_data[i].end = n;
        }
        thread_data[i].thread_id = i;
        
        pthread_create(&threads[i], NULL, odd_even_sort_thread, &thread_data[i]);
    }
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&barrier_mutex);
    pthread_cond_destroy(&phase_complete);
    free(threads);
    free(thread_data);
}

long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

void output_string(int fd, const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    write(fd, str, len);
}

void output_number(int fd, long long num) {
    char buffer[32];
    int i = 0;
    long long n = num;
    
    if (n == 0) {
        buffer[i++] = '0';
    } else {
        while (n > 0) {
            buffer[i++] = '0' + (n % 10);
            n /= 10;
        }
        for (int j = 0; j < i / 2; j++) {
            char temp = buffer[j];
            buffer[j] = buffer[i - j - 1];
            buffer[i - j - 1] = temp;
        }
    }
    
    buffer[i] = '\0';
    write(fd, buffer, i);
}

void output_percent(int fd, int percent) {
    char buffer[32];
    int i = 0;
    int n = percent;
    
    if (n == 0) {
        buffer[i++] = '0';
    } else {
        while (n > 0) {
            buffer[i++] = '0' + (n % 10);
            n /= 10;
        }
        for (int j = 0; j < i / 2; j++) {
            char temp = buffer[j];
            buffer[j] = buffer[i - j - 1];
            buffer[i - j - 1] = temp;
        }
    }
    
    buffer[i++] = '%';
    buffer[i] = '\0';
    write(fd, buffer, i);
}

int read_array_from_file(const char* filename, int** arr) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return -1;
    }
    
    int capacity = 1000;
    int size = 0;
    *arr = (int*)malloc(capacity * sizeof(int));
    
    if (!*arr) {
        close(fd);
        return -1;
    }
    
    char buffer[1024];
    int bytes_read;
    int num = 0;
    int negative = 0;
    int in_number = 0;
    
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (int i = 0; i < bytes_read; i++) {
            char ch = buffer[i];
            
            if (ch >= '0' && ch <= '9') {
                num = num * 10 + (ch - '0');
                in_number = 1;
            } else if (ch == '-') {
                negative = 1;
            } else if (in_number) {
                if (negative) {
                    num = -num;
                }
                
                if (size >= capacity) {
                    capacity *= 2;
                    int* new_arr = (int*)realloc(*arr, capacity * sizeof(int));
                    if (!new_arr) {
                        close(fd);
                        free(*arr);
                        return -1;
                    }
                    *arr = new_arr;
                }
                (*arr)[size++] = num;
                
                num = 0;
                negative = 0;
                in_number = 0;
            }
        }
    }
    
    if (in_number) {
        if (negative) {
            num = -num;
        }
        if (size >= capacity) {
            capacity *= 2;
            int* new_arr = (int*)realloc(*arr, capacity * sizeof(int));
            if (!new_arr) {
                close(fd);
                free(*arr);
                return -1;
            }
            *arr = new_arr;
        }
        (*arr)[size++] = num;
    }
    
    close(fd);
    return size;
}

void write_array_to_file(const char* filename, int* arr, int n) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        return;
    }
    
    for (int i = 0; i < n; i++) {
        output_number(fd, arr[i]);
        if (i < n - 1) {
            output_string(fd, " ");
        }
    }
    output_string(fd, "\n");
    
    close(fd);
}

int check_sorted(int* arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        output_string(STDERR_FILENO, "Неверное использование\n");
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    int max_threads = 0;
    
    const char* ptr = argv[3];
    while (*ptr) {
        max_threads = max_threads * 10 + (*ptr - '0');
        ptr++;
    }
    
    int* array = NULL;
    int array_size = read_array_from_file(input_file, &array);
    
    if (array_size <= 0) {
        output_string(STDERR_FILENO, "Ошибка файла\n");
        return 1;
    }
    
    int* array_copy = (int*)malloc(array_size * sizeof(int));
    int* final_sorted = (int*)malloc(array_size * sizeof(int));
    
    output_string(STDOUT_FILENO, "Размер массива: ");
    output_number(STDOUT_FILENO, array_size);
    output_string(STDOUT_FILENO, " элементов\n");
    output_string(STDOUT_FILENO, "Максимальное количество потоков: ");
    output_number(STDOUT_FILENO, max_threads);
    output_string(STDOUT_FILENO, "\n\n");
    
    output_string(STDOUT_FILENO, "ПОСЛЕДОВАТЕЛЬНЫЙ АЛГОРИТМ:\n");
    output_string(STDOUT_FILENO, "Потоки Время(мс)\n");
    
    for (int i = 0; i < array_size; i++) {
        array_copy[i] = array[i];
    }
    
    long long start_time = get_time_ms();
    sequential_odd_even_sort(array_copy, array_size);
    long long end_time = get_time_ms();
    long long seq_time = end_time - start_time;
    
    if (!check_sorted(array_copy, array_size)) {
        output_string(STDERR_FILENO, "Ошибка сортировки\n");
        free(array);
        free(array_copy);
        free(final_sorted);
        return 1;
    }
    
    for (int i = 0; i < array_size; i++) {
        final_sorted[i] = array_copy[i];
    }
    
    output_string(STDOUT_FILENO, "1 ");
    output_number(STDOUT_FILENO, seq_time);
    output_string(STDOUT_FILENO, "\n\n");
    
    output_string(STDOUT_FILENO, "ПАРАЛЛЕЛЬНЫЙ АЛГОРИТМ:\n");
    output_string(STDOUT_FILENO, "Потоки Время(мс) Ускорение Эффективность\n");
    
    int thread_counts[] = {1, 2, 4, 8, 12, 16, 32, 64, 128};
    int num_tests = sizeof(thread_counts) / sizeof(thread_counts[0]);
    
    for (int i = 0; i < num_tests; i++) {
        int threads = thread_counts[i];
        if (threads > max_threads) continue;

        for (int j = 0; j < array_size; j++) {
            array_copy[j] = array[j];
        }
        
        start_time = get_time_ms();
        parallel_odd_even_sort(array_copy, array_size, threads);
        end_time = get_time_ms();
        long long par_time = end_time - start_time;
        
        if (!check_sorted(array_copy, array_size)) {
            output_string(STDOUT_FILENO, "Ошибка для ");
            output_number(STDOUT_FILENO, threads);
            output_string(STDOUT_FILENO, " потоков\n");
            continue;
        }
        
        for (int j = 0; j < array_size; j++) {
            final_sorted[j] = array_copy[j];
        }
        
        int speedup_percent = (int)(((double)seq_time / par_time) * 100);
        int efficiency_percent = speedup_percent / threads;
        
        output_number(STDOUT_FILENO, threads);
        output_string(STDOUT_FILENO, " ");
        output_number(STDOUT_FILENO, par_time);
        output_string(STDOUT_FILENO, " ");
        output_percent(STDOUT_FILENO, speedup_percent);
        output_string(STDOUT_FILENO, " ");
        output_percent(STDOUT_FILENO, efficiency_percent);
        output_string(STDOUT_FILENO, "\n");
    }

    write_array_to_file(output_file, final_sorted, array_size);
    
    output_string(STDOUT_FILENO, "\nОтсортированный массив записан в файл: ");
    output_string(STDOUT_FILENO, output_file);
    output_string(STDOUT_FILENO, "\n");
    
    free(array);
    free(array_copy);
    free(final_sorted);
    
    output_string(STDOUT_FILENO, "Тестирование завершено успешно!\n");
    return 0;
}