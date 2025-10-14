#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define FILE_PATH "/tmp/nums.bin"
#define PAGE_SIZE 4096

typedef struct {
    double seq_time;
    double rand_time;
    uint64_t seq_sum;
    uint64_t rand_sum;
    const char* advice_name;
} test_result_t;

uint64_t generate_random_uint64() {
    return ((uint64_t)rand() << 48) ^ 
           ((uint64_t)rand() << 32) ^ 
           ((uint64_t)rand() << 16) ^ 
           (uint64_t)rand();
}

void generate_data_file(size_t num_pages) {
    printf("Создание файла данных: %zu страниц (%.2f MB)\n", 
           num_pages, (num_pages * PAGE_SIZE) / (1024.0 * 1024));
    
    size_t file_size = num_pages * PAGE_SIZE;
    size_t num_elements = file_size / sizeof(uint64_t);
    
    remove(FILE_PATH);
    
    int fd = open(FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("Ошибка создания файла");
        exit(1);
    }
    
    if (ftruncate(fd, file_size) == -1) {
        perror("Ошибка установки размера файла");
        close(fd);
        exit(1);
    }
    
    uint64_t *data = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("Ошибка mmap для записи");
        close(fd);
        exit(1);
    }
    
    srand(time(NULL));
    for (size_t i = 0; i < num_elements; i++) {
        data[i] = generate_random_uint64();
    }
    
    msync(data, file_size, MS_SYNC);
    munmap(data, file_size);
    close(fd);
    
    printf("Файл создан: %s, элементов: %zu\n", FILE_PATH, num_elements);
}

void fisher_yates_shuffle(size_t *array, size_t n) {
    for (size_t i = n - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

uint64_t sequential_access(uint64_t *data, size_t num_elements) {
    uint64_t sum = 0;
    // Добавляем volatile чтобы предотвратить чрезмерную оптимизацию
    volatile uint64_t temp;
    for (size_t i = 0; i < num_elements; i++) {
        temp = data[i];
        sum += temp * temp;
    }
    return sum;
}

uint64_t random_access_fisher_yates(uint64_t *data, size_t num_elements) {
    size_t *indices = malloc(num_elements * sizeof(size_t));
    if (!indices) {
        perror("Ошибка выделения памяти для индексов");
        exit(1);
    }
    
    for (size_t i = 0; i < num_elements; i++) {
        indices[i] = i;
    }
    fisher_yates_shuffle(indices, num_elements);
    
    uint64_t sum = 0;
    volatile uint64_t temp;
    for (size_t i = 0; i < num_elements; i++) {
        temp = data[indices[i]];
        sum += temp * temp;
    }
    
    free(indices);
    return sum;
}

test_result_t run_single_test(const char* test_name, int advice, size_t num_pages) {
    struct timespec start, end;
    test_result_t result;
    result.advice_name = test_name;
    
    size_t file_size = num_pages * PAGE_SIZE;
    size_t num_elements = file_size / sizeof(uint64_t);
    
    int fd = open(FILE_PATH, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(1);
    }
    
    uint64_t *data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("Ошибка mmap для чтения");
        close(fd);
        exit(1);
    }
    
    // Применяем совет ядру
    if (advice != -1) {
        if (madvise(data, file_size, advice) == -1) {
            fprintf(stderr, "Ошибка madvise(%s): %s\n", test_name, strerror(errno));
        }
        usleep(50000); // 50ms для обработки советов ядром
    }
    
    // Тест последовательного доступа
    clock_gettime(CLOCK_MONOTONIC, &start);
    result.seq_sum = sequential_access(data, num_elements);
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.seq_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    usleep(10000); // 10ms пауза
    
    // Тест случайного доступа
    clock_gettime(CLOCK_MONOTONIC, &start);
    result.rand_sum = random_access_fisher_yates(data, num_elements);
    clock_gettime(CLOCK_MONOTONIC, &end);
    result.rand_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    // Проверяем корректность
    if (result.seq_sum != result.rand_sum) {
        printf("ВНИМАНИЕ: Суммы не совпадают! seq=%lu, rand=%lu\n", 
               result.seq_sum, result.rand_sum);
    }
    
    munmap(data, file_size);
    close(fd);
    
    return result;
}

void clear_system_cache() {
    printf("Очистка системного кэша...\n");
    sync();
    int ret = system("sudo sh -c 'echo 3 > /proc/sys/vm/drop_caches'");
    if (ret != 0) {
        printf("Предупреждение: не удалось очистить кэш (нужны права sudo)\n");
    }
    usleep(1000000); 
}

void print_test_result(const test_result_t *result) {
    double ratio = result->rand_time / result->seq_time;
    printf("│ %-16s │ %9.6f │ %9.6f │ %6.1fx │ %-20lu │\n",
           result->advice_name,
           result->seq_time,
           result->rand_time,
           ratio,
           result->seq_sum);
}

int main(int argc, char *argv[]) {
    printf("=== ЛАБОРАТОРНАЯ РАБОТА: ВЕЛИКИЙ ВИЗИРЬ ===\n\n");
    
    size_t num_pages = 5000;
    int num_runs = 3;
    
    if (argc > 1) num_pages = atol(argv[1]);
    if (argc > 2) num_runs = atoi(argv[2]);
    
    printf("Параметры тестирования:\n");
    printf("- Размер данных: %zu страниц (%.2f MB)\n", 
           num_pages, (num_pages * PAGE_SIZE) / (1024.0 * 1024));
    printf("- Количество прогонов: %d\n", num_runs);
    printf("- Размер страницы: %d байт\n\n", PAGE_SIZE);
    
    // Генерируем данные один раз
    generate_data_file(num_pages);
    
    struct {
        const char* name;
        int advice;
    } tests[] = {
        {"NO_ADVICE", -1},
        {"MADV_SEQUENTIAL", MADV_SEQUENTIAL},
        {"MADV_RANDOM", MADV_RANDOM},
        {"MADV_WILLNEED", MADV_WILLNEED}
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    test_result_t total_results[4] = {0};
    
    printf("=== НАЧАЛО ТЕСТИРОВАНИЯ ===\n\n");
    
    for (int run = 0; run < num_runs; run++) {
        printf("\n--- Прогон %d/%d ---\n", run + 1, num_runs);
        
        if (run > 0) {
            clear_system_cache();
        }
        
        printf("┌──────────────────┬───────────┬───────────┬────────┬──────────────────────┐\n");
        printf("│ Режим           │ Послед. (с)│ Случ. (с) │ Отн.   │ Сумма               │\n");
        printf("├──────────────────┼───────────┼───────────┼────────┼──────────────────────┤\n");
        
        for (int i = 0; i < num_tests; i++) {
            // КАЖДЫЙ ТЕСТ с независимым mmap
            test_result_t result = run_single_test(tests[i].name, tests[i].advice, num_pages);
            print_test_result(&result);
            
            total_results[i].seq_time += result.seq_time;
            total_results[i].rand_time += result.rand_time;
            total_results[i].seq_sum = result.seq_sum;
            total_results[i].advice_name = tests[i].name;
        }
        
        printf("└──────────────────┴───────────┴───────────┴────────┴──────────────────────┘\n");
    }
    
    printf("\n=== СРЕДНИЕ РЕЗУЛЬТАТЫ (%d прогонов) ===\n", num_runs);
    printf("┌──────────────────┬───────────┬───────────┬────────┬──────────────────────┐\n");
    printf("│ Режим           │ Послед. (с)│ Случ. (с) │ Отн.   │ Сумма               │\n");
    printf("├──────────────────┼───────────┼───────────┼────────┼──────────────────────┤\n");
    
    for (int i = 0; i < num_tests; i++) {
        double avg_seq = total_results[i].seq_time / num_runs;
        double avg_rand = total_results[i].rand_time / num_runs;
        double ratio = avg_rand / avg_seq;
        
        printf("│ %-16s │ %9.6f │ %9.6f │ %6.1fx │ %-20lu │\n",
               tests[i].name, avg_seq, avg_rand, ratio, total_results[i].seq_sum);
    }
    printf("└──────────────────┴───────────┴───────────┴────────┴──────────────────────┘\n");
    
    printf("\n=== АНАЛИЗ РЕЗУЛЬТАТОВ ===\n");
    printf("1. Случайный доступ значительно медленнее последовательного\n");
    printf("2. MADV_RANDOM ускоряет случайный доступ\n");
    printf("3. MADV_SEQUENTIAL может замедлять случайный доступ\n");
    printf("4. MADV_WILLNEED полезен для предварительной загрузки\n");
    printf("5. Размер данных влияет на эффективность стратегий\n");
    
    remove(FILE_PATH);
    printf("\nВременный файл %s удален\n", FILE_PATH);
    printf("\n=== ТЕСТИРОВАНИЕ ЗАВЕРШЕНО ===\n");
    return 0;
}