#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>

typedef struct {
    int start;
    int end;
    int thread_id;
} thread_data_t;

pthread_mutex_t barrier_mutex;
pthread_cond_t phase_complete;
int threads_ready;
int total_threads;

int* global_array;
int global_size;

// Барьер
void barrier_wait() {
    pthread_mutex_lock(&barrier_mutex);
    threads_ready++;
    if (threads_ready == total_threads) {
        threads_ready = 0;
        pthread_cond_broadcast(&phase_complete);
    } else {
        pthread_cond_wait(&phase_complete, &barrier_mutex);
    }
    pthread_mutex_unlock(&barrier_mutex);
}

// Функция потока
void* odd_even_sort_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int* arr = global_array;
    int n = global_size;
    
    for (int phase = 0; phase < n; phase++) {
        barrier_wait();
        
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
        }
        else {
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
        
        barrier_wait();
    }
    
    return NULL;
}

// Последовательная версия
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
        }
        else {
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

// Параллельная версия
void parallel_odd_even_sort(int* arr, int n, int num_threads) {
    global_array = arr;
    global_size = n;
    
    pthread_mutex_init(&barrier_mutex, NULL);
    pthread_cond_init(&phase_complete, NULL);
    
    threads_ready = 0;
    total_threads = num_threads;
    
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    thread_data_t* thread_data = (thread_data_t*)malloc(num_threads * sizeof(thread_data_t));
    
    int elements_per_thread = (n + num_threads - 1) / num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].start = i * elements_per_thread;
        thread_data[i].end = (i + 1) * elements_per_thread;
        if (thread_data[i].end > n) thread_data[i].end = n;
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

void generate_array(int* arr, int n) {
    int seed = getpid();
    for (int i = 0; i < n; i++) {
        arr[i] = (seed * (i + 1)) % 10000;
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
    }
}

int check_sorted(int* arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;
        }
    }
    return 1;
}

void output_string(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    write(STDOUT_FILENO, str, len);
}

void output_number(long long num) {
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
    write(STDOUT_FILENO, buffer, i);
}

void output_percent(int percent) {
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
    write(STDOUT_FILENO, buffer, i);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        output_string("Usage: program array_size max_threads iterations\n");
        return 1;
    }
    
    int array_size = 0;
    int max_threads = 0;
    int iterations = 0;
    
    const char* ptr = argv[1];
    while (*ptr) {
        array_size = array_size * 10 + (*ptr - '0');
        ptr++;
    }
    
    ptr = argv[2];
    while (*ptr) {
        max_threads = max_threads * 10 + (*ptr - '0');
        ptr++;
    }
    
    ptr = argv[3];
    while (*ptr) {
        iterations = iterations * 10 + (*ptr - '0');
        ptr++;
    }
    
    int logical_cores = sysconf(_SC_NPROCESSORS_ONLN);
    
    int* array = (int*)malloc(array_size * sizeof(int));
    
    output_string("Логических ядер в системе: ");
    output_number(logical_cores);
    output_string("\n");
    output_string("Размер массива: ");
    output_number(array_size);
    output_string(" элементов\n");
    output_string("Количество итераций: ");
    output_number(iterations);
    output_string("\n\n");
    
    output_string("ПОСЛЕДОВАТЕЛЬНЫЙ АЛГОРИТМ:\n");
    output_string("Потоки Время(мс)\n");
    
    long long total_seq_time = 0;
    for (int iter = 0; iter < iterations; iter++) {
        generate_array(array, array_size);
        long long start_time = get_time_ms();
        sequential_odd_even_sort(array, array_size);
        long long end_time = get_time_ms();
        total_seq_time += (end_time - start_time);
        
        if (!check_sorted(array, array_size)) {
            output_string("ERROR: Sequential sort failed\n");
            return 1;
        }
    }
    long long seq_time = total_seq_time / iterations;
    
    output_string("1 ");
    output_number(seq_time);
    output_string("\n\n");
    
    output_string("ПАРАЛЛЕЛЬНЫЙ АЛГОРИТМ:\n");
    output_string("Потоки Время(мс) Ускорение Эффективность\n");
    
    int thread_counts[] = {1, 2, 4, 8, 12, 16, 32, 64, 128};
    int num_tests = sizeof(thread_counts) / sizeof(thread_counts[0]);
    
    for (int i = 0; i < num_tests; i++) {
        int threads = thread_counts[i];
        if (threads > max_threads) continue;
        
        long long total_par_time = 0;
        int success = 1;
        
        for (int iter = 0; iter < iterations; iter++) {
            generate_array(array, array_size);
            long long start_time = get_time_ms();
            parallel_odd_even_sort(array, array_size, threads);
            long long end_time = get_time_ms();
            total_par_time += (end_time - start_time);
            
            if (!check_sorted(array, array_size)) {
                output_string("ERROR: Parallel sort failed for ");
                output_number(threads);
                output_string(" threads\n");
                success = 0;
                break;
            }
        }
        
        if (!success) continue;
        
        long long par_time = total_par_time / iterations;
        
        int speedup_percent = (int)(((double)seq_time / par_time) * 100);
        int efficiency_percent = speedup_percent / threads;
        
        output_number(threads);
        output_string(" ");
        output_number(par_time);
        output_string(" ");
        output_percent(speedup_percent);
        output_string(" ");
        output_percent(efficiency_percent);
        output_string("\n");
    }
    
    free(array);
    
    output_string("\nТестирование завершено успешно!\n");
    return 0;
}