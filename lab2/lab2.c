#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/mman.h>
#endif

// Структура для передачи данных в поток
typedef struct {
    int start;
    int end;
    int thread_id;
    int* local_sorted; // Флаг локальной отсортированности
} thread_data_t;

// Глобальные переменные
#ifdef _WIN32
    HANDLE barrier_mutex;
    HANDLE phase_complete;
    int threads_ready;
    int total_threads;
#else
    pthread_mutex_t barrier_mutex;
    pthread_cond_t phase_complete;
    int threads_ready;
    int total_threads;
#endif

int* global_array;
int global_size;
int global_any_swap; // Глобальный флаг обмена

// Барьер для синхронизации с проверкой обменов
void barrier_wait(int* local_swap) {
    #ifdef _WIN32
        WaitForSingleObject(barrier_mutex, INFINITE);
        if (*local_swap) global_any_swap = 1;
        threads_ready++;
        if (threads_ready == total_threads) {
            threads_ready = 0;
            ReleaseSemaphore(phase_complete, total_threads - 1, NULL);
        } else {
            ReleaseMutex(barrier_mutex);
            WaitForSingleObject(phase_complete, INFINITE);
        }
    #else
        pthread_mutex_lock(&barrier_mutex);
        if (*local_swap) global_any_swap = 1;
        threads_ready++;
        if (threads_ready == total_threads) {
            threads_ready = 0;
            pthread_cond_broadcast(&phase_complete);
        } else {
            pthread_cond_wait(&phase_complete, &barrier_mutex);
        }
        pthread_mutex_unlock(&barrier_mutex);
    #endif
}

// Функция потока для четно-нечетной сортировки
#ifdef _WIN32
DWORD WINAPI odd_even_sort_thread(LPVOID arg)
#else
void* odd_even_sort_thread(void* arg)
#endif
{
    thread_data_t* data = (thread_data_t*)arg;
    int* arr = global_array;
    int n = global_size;
    
    for (int phase = 0; phase < n; phase++) {
        int local_swap = 0;
        
        // Оптимизация: проверяем глобальный флаг после первых нескольких фаз
        if (phase > n/2) {
            #ifdef _WIN32
                WaitForSingleObject(barrier_mutex, INFINITE);
                int should_break = !global_any_swap;
                ReleaseMutex(barrier_mutex);
                if (should_break) break;
            #else
                pthread_mutex_lock(&barrier_mutex);
                int should_break = !global_any_swap;
                pthread_mutex_unlock(&barrier_mutex);
                if (should_break) break;
            #endif
        }
        
        // Четная фаза - работаем со всеми четными индексами в своем диапазоне
        if (phase % 2 == 0) {
            // Начинаем с первого четного индекса в диапазоне
            int start_i = (data->start % 2 == 0) ? data->start : data->start + 1;
            for (int i = start_i; i < data->end && i + 1 < n; i += 2) {
                if (arr[i] > arr[i + 1]) {
                    int temp = arr[i];
                    arr[i] = arr[i + 1];
                    arr[i + 1] = temp;
                    local_swap = 1;
                }
            }
        }
        // Нечетная фаза - работаем со всеми нечетными индексами в своем диапазоне
        else {
            // Начинаем с первого нечетного индекса в диапазоне
            int start_i = (data->start % 2 == 1) ? data->start : data->start + 1;
            for (int i = start_i; i < data->end && i + 1 < n; i += 2) {
                if (arr[i] > arr[i + 1]) {
                    int temp = arr[i];
                    arr[i] = arr[i + 1];
                    arr[i + 1] = temp;
                    local_swap = 1;
                }
            }
        }
        
        // Синхронизация в конце фазы
        barrier_wait(&local_swap);
        
        // Сбрасываем глобальный флаг для следующей фазы (делает последний поток)
        #ifdef _WIN32
            WaitForSingleObject(barrier_mutex, INFINITE);
            if (threads_ready == 0) {
                global_any_swap = 0;
            }
            ReleaseMutex(barrier_mutex);
        #else
            pthread_mutex_lock(&barrier_mutex);
            if (threads_ready == 0) {
                global_any_swap = 0;
            }
            pthread_mutex_unlock(&barrier_mutex);
        #endif
        
        barrier_wait(&local_swap);
    }
    
    #ifdef _WIN32
        return 0;
    #else
        return NULL;
    #endif
}

// Последовательная версия (оптимизированная)
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
    global_any_swap = 1; // Начинаем с предположения, что есть обмены
    
    // Инициализация синхронизации
    #ifdef _WIN32
        barrier_mutex = CreateMutex(NULL, FALSE, NULL);
        phase_complete = CreateSemaphore(NULL, 0, num_threads, NULL);
    #else
        pthread_mutex_init(&barrier_mutex, NULL);
        pthread_cond_init(&phase_complete, NULL);
    #endif
    
    threads_ready = 0;
    total_threads = num_threads;
    
    // Выделение памяти для потоков и данных
    #ifdef _WIN32
        HANDLE* threads = (HANDLE*)VirtualAlloc(NULL, num_threads * sizeof(HANDLE), 
                                              MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        thread_data_t* thread_data = (thread_data_t*)VirtualAlloc(NULL, num_threads * sizeof(thread_data_t), 
                                                                MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    #else
        pthread_t* threads = (pthread_t*)mmap(NULL, num_threads * sizeof(pthread_t),
                                            PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
        thread_data_t* thread_data = (thread_data_t*)mmap(NULL, num_threads * sizeof(thread_data_t),
                                                        PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    #endif
    
    // Улучшенное распределение работы - учитываем границы между потоками
    int base_chunk = n / num_threads;
    int remainder = n % num_threads;
    int current_start = 0;
    
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + base_chunk + (i < remainder ? 1 : 0);
        thread_data[i].thread_id = i;
        thread_data[i].local_sorted = 0;
        
        // Корректируем границы чтобы избежать пропусков между потоками
        if (i < num_threads - 1) {
            thread_data[i].end++; // Перекрытие для обработки граничных элементов
        }
        
        current_start = thread_data[i].end;
        
        #ifdef _WIN32
            threads[i] = CreateThread(NULL, 0, odd_even_sort_thread, &thread_data[i], 0, NULL);
        #else
            pthread_create(&threads[i], NULL, odd_even_sort_thread, &thread_data[i]);
        #endif
    }
    
    // Ожидание завершения
    for (int i = 0; i < num_threads; i++) {
        #ifdef _WIN32
            WaitForSingleObject(threads[i], INFINITE);
            CloseHandle(threads[i]);
        #else
            pthread_join(threads[i], NULL);
        #endif
    }
    
    // Освобождение ресурсов
    #ifdef _WIN32
        CloseHandle(barrier_mutex);
        CloseHandle(phase_complete);
        VirtualFree(threads, 0, MEM_RELEASE);
        VirtualFree(thread_data, 0, MEM_RELEASE);
    #else
        pthread_mutex_destroy(&barrier_mutex);
        pthread_cond_destroy(&phase_complete);
        munmap(threads, num_threads * sizeof(pthread_t));
        munmap(thread_data, num_threads * sizeof(thread_data_t));
    #endif
}

// Получение времени в миллисекундах
long long get_time_ms() {
    #ifdef _WIN32
        LARGE_INTEGER freq, count;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&count);
        return (count.QuadPart * 1000) / freq.QuadPart;
    #else
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    #endif
}

// Генерация массива
void generate_array(int* arr, int n) {
    #ifdef _WIN32
        DWORD seed = GetTickCount();
        for (int i = 0; i < n; i++) {
            arr[i] = (seed * (i + 1)) % 10000;
            seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        }
    #else
        int seed = getpid();
        for (int i = 0; i < n; i++) {
            arr[i] = (seed * (i + 1)) % 10000;
            seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
        }
    #endif
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
    
    #ifdef _WIN32
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), str, len, NULL, NULL);
    #else
        write(STDOUT_FILENO, str, len);
    #endif
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
        #ifdef _WIN32
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), " ", 1, NULL, NULL);
        #else
            write(STDOUT_FILENO, " ", 1);
        #endif
        width--;
    }
    
    buffer[i] = '\0';
    
    #ifdef _WIN32
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, i, NULL, NULL);
    #else
        write(STDOUT_FILENO, buffer, i);
    #endif
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
        #ifdef _WIN32
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), " ", 1, NULL, NULL);
        #else
            write(STDOUT_FILENO, " ", 1);
        #endif
        width--;
    }
    
    buffer[i] = '\0';
    
    #ifdef _WIN32
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), buffer, i, NULL, NULL);
    #else
        write(STDOUT_FILENO, buffer, i);
    #endif
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
    int logical_cores;
    #ifdef _WIN32
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);
        logical_cores = sysinfo.dwNumberOfProcessors;
    #else
        logical_cores = sysconf(_SC_NPROCESSORS_ONLN);
    #endif
    
    // Выделение памяти для массива
    #ifdef _WIN32
        int* array = (int*)VirtualAlloc(NULL, array_size * sizeof(int), 
                                      MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    #else
        int* array = (int*)mmap(NULL, array_size * sizeof(int),
                              PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    #endif
    
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
    
    // Тестируем разное количество потоков, начиная с 1 до max_threads
    for (int threads = 1; threads <= max_threads; threads *= 2) {
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
        
        // Также тестируем количество потоков равное количеству ядер
        if (threads * 2 > max_threads && logical_cores > threads && logical_cores <= max_threads) {
            threads = logical_cores - 1; // Чтобы на следующей итерации протестировали logical_cores
        }
    }
    
    // Освобождение памяти
    #ifdef _WIN32
        VirtualFree(array, 0, MEM_RELEASE);
    #else
        munmap(array, array_size * sizeof(int));
    #endif
    
    output_string("\nТестирование завершено успешно!\n");
    return 0;
}