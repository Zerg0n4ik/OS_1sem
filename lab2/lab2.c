#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/mman.h>

// Структура для передачи данных в поток
typedef struct {
    int start;
    int end;
    int thread_id;
} thread_data_t;

// Глобальные переменные для синхронизации
pthread_mutex_t barrier_mutex;
pthread_cond_t phase_complete;
int threads_ready;
int total_threads;

int* global_array;
int global_size;

// Барьер для синхронизации
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

// Функция потока для четно-нечетной сортировки
void* odd_even_sort_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    int* arr = global_array;
    int n = global_size;
    
    for (int phase = 0; phase < n; phase++) {
        barrier_wait();
        
        // Четная фаза - сравниваем элементы с четными индексами
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
        // Нечетная фаза - сравниваем элементы с нечетными индексами
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
        
        // Четная фаза
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
        // Нечетная фаза
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
        
        if (sorted) break;
    }
}

// Параллельная версия
void parallel_odd_even_sort(int* arr, int n, int num_threads) {
    global_array = arr;
    global_size = n;
    
    // Инициализация синхронизации
    pthread_mutex_init(&barrier_mutex, NULL);
    pthread_cond_init(&phase_complete, NULL);
    
    threads_ready = 0;
    total_threads = num_threads;
    
    // Выделение памяти для потоков и данных
    pthread_t* threads = (pthread_t*)mmap(NULL, num_threads * sizeof(pthread_t),
                                        PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    thread_data_t* thread_data = (thread_data_t*)mmap(NULL, num_threads * sizeof(thread_data_t),
                                                    PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    
    int elements_per_thread = (n + num_threads - 1) / num_threads;
    
    // Создание потоков
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].start = i * elements_per_thread;
        thread_data[i].end = (i + 1) * elements_per_thread;
        if (thread_data[i].end > n) thread_data[i].end = n;
        thread_data[i].thread_id = i;
        
        pthread_create(&threads[i], NULL, odd_even_sort_thread, &thread_data[i]);
    }
    
    // Ожидание завершения
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Освобождение ресурсов
    pthread_mutex_destroy(&barrier_mutex);
    pthread_cond_destroy(&phase_complete);
    munmap(threads, num_threads * sizeof(pthread_t));
    munmap(thread_data, num_threads * sizeof(thread_data_t));
}

// Получение времени в миллисекундах
long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

// Генерация массива
void generate_array(int* arr, int n) {
    int seed = getpid();
    for (int i = 0; i < n; i++) {
        arr[i] = (seed * (i + 1)) % 10000;
        seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
    }
}

// Проверка сортировки
int check_sorted(int* arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;
        }
    }
    return 1;
}

// Вывод строки
void output_string(const char* str) {
    int len = 0;
    while (str[len] != '\0') len++;
    write(STDOUT_FILENO, str, len);
}

// Вывод числа с выравниванием
void output_number_aligned(long long num, int width) {
    char buffer[32];
    int i = 0;
    long long n = num;
    
    if (n == 0) {
        buffer[i++] = '0';
    } else {
        // Записываем цифры в обратном порядке
        while (n > 0) {
            buffer[i++] = '0' + (n % 10);
            n /= 10;
        }
        // Разворачиваем цифры
        for (int j = 0; j < i / 2; j++) {
            char temp = buffer[j];
            buffer[j] = buffer[i - j - 1];
            buffer[i - j - 1] = temp;
        }
    }
    
    // Добавляем пробелы для выравнивания
    while (i < width) {
        write(STDOUT_FILENO, " ", 1);
        width--;
    }
    
    buffer[i] = '\0';
    write(STDOUT_FILENO, buffer, i);
}

// Вывод числа с плавающей точкой
void output_float_aligned(double num, int width, int decimals) {
    char buffer[32];
    long long int_part = (long long)num;
    double frac_part = num - int_part;
    
    // Целая часть
    int i = 0;
    long long n = int_part;
    
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
    
    // Дробная часть
    if (decimals > 0) {
        buffer[i++] = '.';
        for (int j = 0; j < decimals; j++) {
            frac_part *= 10;
            int digit = (int)frac_part;
            buffer[i++] = '0' + digit;
            frac_part -= digit;
        }
    }
    
    // Выравнивание
    while (i < width) {
        write(STDOUT_FILENO, " ", 1);
        width--;
    }
    
    buffer[i] = '\0';
    write(STDOUT_FILENO, buffer, i);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        output_string("Usage: program array_size max_threads iterations\n");
        return 1;
    }
    
    // Парсинг аргументов
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
    
    // Получение количества логических ядер
    int logical_cores = sysconf(_SC_NPROCESSORS_ONLN);
    
    // Выделение памяти для массива
    int* array = (int*)mmap(NULL, array_size * sizeof(int),
                          PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    
    // Красивый вывод заголовка
    output_string("Логических ядер в системе: ");
    output_number_aligned(logical_cores, 0);
    output_string("\n");
    output_string("Размер массива: ");
    output_number_aligned(array_size, 0);
    output_string(" элементов\n");
    output_string("Количество итераций: ");
    output_number_aligned(iterations, 0);
    output_string("\n\n");
    
    // Тестирование последовательной версии
    output_string("ПОСЛЕДОВАТЕЛЬНЫЙ АЛГОРИТМ:\n");
    output_string("Число потоков | Время исполнения (мс)\n");
    output_string("--------------|-----------------------\n");
    
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
    
    output_string("1             | ");
    output_number_aligned(seq_time, 0);
    output_string("\n\n");
    
    // Тестирование параллельных версий
    output_string("ПАРАЛЛЕЛЬНЫЙ АЛГОРИТМ:\n");
    output_string("Число потоков | Время исполнения (мс) | Ускорение  | Эффективность\n");
    output_string("--------------|-----------------------|------------|--------------\n");
    
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
                output_number_aligned(threads, 0);
                output_string(" threads\n");
                success = 0;
                break;
            }
        }
        
        if (!success) continue;
        
        long long par_time = total_par_time / iterations;
        
        // Расчет метрик
        double speedup = (double)seq_time / par_time;
        double efficiency = speedup / threads;
        
        // Вывод строки результатов
        output_number_aligned(threads, 14);
        output_string(" | ");
        output_number_aligned(par_time, 23);
        output_string(" | ");
        output_float_aligned(speedup, 10, 2);
        output_string(" | ");
        output_float_aligned(efficiency, 12, 2);
        output_string("\n");
    }
    
    // Освобождение памяти
    munmap(array, array_size * sizeof(int));
    
    output_string("\nТестирование завершено успешно!\n");
    return 0;
}