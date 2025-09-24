#include <stdio.h>      
#include <stdlib.h>   
#include <unistd.h>     
#include <signal.h>     
#include <sys/wait.h>  
#include <time.h>      

/**
 * @brief PID дочернего процесса.
 * @details Глобальная переменная для хранения PID дочернего процесса, чтобы
 * родительский процесс мог отправить ему сигнал завершения.
 * `volatile` используется, так как переменная изменяется в основном потоке,
 * а читается в обработчике сигналов.
 */
static volatile pid_t child_pid = 0;

/**
 * @brief Флаг для корректного завершения программы.
 * @details `volatile sig_atomic_t` - безопасный тип для использования
 * в обработчиках сигналов. Устанавливается в 1, когда родитель
 * получает SIGINT, чтобы прервать основной цикл ожидания.
 */
static volatile sig_atomic_t stop_flag = 0;

/**
 * @brief Обработчик сигнала для родительского процесса (ловит "понг").
 * @details Срабатывает при получении сигнала SIGUSR2 от дочернего процесса.
 * Выводит полученное число, генерирует новое и отправляет его
 * дочернему процессу ("пинг") с сигналом SIGUSR1.
 * @param signo Номер полученного сигнала (ожидается SIGUSR2).
 * @param si Указатель на структуру `siginfo_t`, содержащую дополнительную
 * информацию о сигнале, включая переданное целочисленное значение.
 * @param ucontext Указатель на контекст сигнала (не используется).
 */
void parent_handler(int signo, siginfo_t *si, void *ucontext) {
    printf("[Parent %d] получил число: %d\n", (int)getpid(), si->si_value.sival_int);

    int new_val = rand() % 100;

    const union sigval value = {.sival_int = new_val};

    printf("[Parent %d] отправляет число: %d\n", (int)getpid(), new_val);
    sigqueue(child_pid, SIGUSR1, value);
}

/**
 * @brief Обработчик сигнала для дочернего процесса (ловит "пинг").
 * @details Срабатывает при получении сигнала SIGUSR1 от родительского процесса.
 * Выводит полученное число, генерирует новое и отправляет его
 * родительскому процессу ("понг") с сигналом SIGUSR2.
 * @param signo Номер полученного сигнала (ожидается SIGUSR1).
 * @param si Указатель на структуру `siginfo_t` с полученным значением.
 * @param ucontext Указатель на контекст сигнала (не используется).
 */
void child_handler(int signo, siginfo_t *si, void *ucontext) {
    printf("[Child %d] получил число: %d\n", (int)getpid(), si->si_value.sival_int);

    int new_val = rand() % 100;

    const union sigval value = {.sival_int = new_val};

    printf("[Child %d] отправляет число: %d\n", (int)getpid(), new_val);
    sigqueue(getppid(), SIGUSR2, value);
}

/**
 * @brief Обработчик сигнала SIGINT (Ctrl+C) для родительского процесса.
 * @details Инициирует корректное завершение программы. Отправляет
 * сигнал SIGTERM дочернему процессу и устанавливает флаг `stop_flag`
 * для выхода из основного цикла родителя.
 * @param signo Номер полученного сигнала (ожидается SIGINT).
 */
void stop_handler(int signo) {
    printf("\n[Parent %d] Остановка...\n", (int)getpid());

    if (child_pid > 0) {
        kill(child_pid, SIGTERM);
    }
    
    stop_flag = 1;
}

/**
 * @brief Главная функция программы.
 * @details Устанавливает обработчики сигналов, создает дочерний процесс,
 * инициирует обмен сигналами и обрабатывает завершение.
 * @return 0 в случае успешного выполнения, 1 в случае ошибки.
 */
int main() {
    // Отключаем буферизацию стандартного вывода для немедленного отображения
    setvbuf(stdout, NULL, _IONBF, 0);

    srand(time(NULL));

    // Настраиваем обработчики сигналов для родителя
    struct sigaction sa_parent;

    // Обработчик для "понга" (SIGUSR2)
    sa_parent.sa_flags = SA_SIGINFO; 
    sa_parent.sa_sigaction = parent_handler;
    sigemptyset(&sa_parent.sa_mask);

    if (sigaction(SIGUSR2, &sa_parent, NULL) == -1) {
        perror("sigaction parent");
        exit(1);
    }

    // Устанавливаем обработчик для Ctrl+C (SIGINT)
    if (signal(SIGINT, stop_handler) == SIG_ERR) {
        perror("signal SIGINT");
        exit(1);
    }

    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        exit(1);
    }

    if (child_pid == 0) { 
        // Сбрасываем унаследованный обработчик SIGINT на стандартное поведение
        signal(SIGINT, SIG_DFL);

        // Настраиваем обработчик для "пинга" (SIGUSR1)
        struct sigaction sa_child;
        sa_child.sa_flags = SA_SIGINFO;
        sa_child.sa_sigaction = child_handler;
        sigemptyset(&sa_child.sa_mask);

        if (sigaction(SIGUSR1, &sa_child, NULL) == -1) {
            perror("sigaction child");
            exit(1);
        }

        while (1) {
            pause(); 
        }
    } else { 
        // Даем дочернему процессу время на установку своего обработчика
        sleep(1);

        int first_val = rand() % 100;

        const union sigval value = {.sival_int = first_val};

        printf("[Parent %d] отправляет число: %d\n", (int)getpid(), first_val);
        sigqueue(child_pid, SIGUSR1, value);

        while (!stop_flag) {
            pause();
        }

        wait(NULL);
        
        printf("[Parent %d] Дочерний процесс завершен. Выход.\n", (int)getpid());
    }

    return 0;
}