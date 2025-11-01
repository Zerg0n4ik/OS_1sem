#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define COUNT 50000  // Количество чисел для генерации

int main() {
    // Инициализация генератора случайных чисел
    srand(time(NULL));
    
    // Открываем файл для записи
    FILE *file = fopen("input.txt", "w");
    if (file == NULL) {
        printf("Ошибка открытия файла!\n");
        return 1;
    }
    
    printf("Генерация %d случайных чисел...\n", COUNT);
    
    // Генерируем и записываем числа от 1 до 50000
    for (int i = 0; i < COUNT; i++) {
        // Генерируем число от 1 до 50000 (больше 0)
        int number = (rand() % 50000) + 1;
        
        fprintf(file, "%d", number);
        
        // Добавляем пробел после каждого числа, кроме последнего
        if (i < COUNT - 1) {
            fprintf(file, " ");
        }
        
        // Выводим прогресс каждые 5000 чисел
        if ((i + 1) % 5000 == 0) {
            printf("Сгенерировано %d чисел...\n", i + 1);
        }
    }
    
    // Закрываем файл
    fclose(file);
    
    printf("Готово! %d чисел записано в файл 'random_numbers.txt'\n", COUNT);
    
    return 0;
}