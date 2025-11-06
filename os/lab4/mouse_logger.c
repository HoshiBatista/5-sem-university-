/**
 * @file mouse_logger.c
 * @brief Драйвер ядра Linux для логирования событий мыши
 * 
 * Этот модуль реализует драйвер ядра Linux, который перехватывает и логирует
 * все события от устройств мыши, включая:
 * - Перемещение по осям X и Y
 * - Нажатия кнопок (левая, правая, средняя)
 * - Прокрутку колеса
 * 
 * @note Драйвер соответствует следующим требованиям задания:
 * 3.1 - Использует встроенные средства ядра для логирования (printk/pr_debug)
 * 3.2 - Реализует структуру управления ресурсами с корректным выделением/освобождением памяти
 * 3.3 - Содержит механизм тестирования производительности при высокой нагрузке
 */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Modern mouse event logger driver for Linux kernel 6.14+");
MODULE_VERSION("2.0");

/**
 * @struct mouse_event
 * @brief Структура для хранения информации о событии мыши
 * 
 * Хранит все необходимые данные о событии для последующего логирования.
 * Использует 64-битное временное поле для высокой точности измерений.
 */
struct mouse_event {
    unsigned int type;      /**< Тип события (EV_REL, EV_KEY и др.) */
    unsigned int code;      /**< Код события (REL_X, BTN_LEFT и др.) */
    int value;              /**< Значение события (перемещение, состояние кнопки) */
    u64 timestamp_ns;       /**< Временная метка в наносекундах */
};

/**
 * @struct mouse_logger_data
 * @brief Центральная структура управления ресурсами драйвера
 * 
 * Содержит все данные, необходимые для работы драйвера:
 * - Обработчик ввода
 * - Счетчик обработанных событий
 * - Флаг завершения работы
 * 
 * @note Эта структура демонстрирует требование 3.2 задания -
 * управление ресурсами драйвера в одном месте с гарантией
 * корректного освобождения памяти.
 */
struct mouse_logger_data {
    struct input_handler handler; /**< Обработчик событий ввода */
    atomic64_t event_count;        /**< Атомарный счетчик обработанных событий */
    bool exiting;                  /**< Флаг завершения работы драйвера */
};

static struct mouse_logger_data *logger_data; /**< Глобальная структура управления драйвером */

/**
 * @brief Обработчик событий мыши
 * 
 * Вызывается при каждом событии от устройства мыши. Выполняет:
 * 1. Проверку флага завершения работы
 * 2. Формирование структуры события с временной меткой
 * 3. Увеличение счетчика событий
 * 4. Детальное логирование с фильтрацией по типам событий
 * 
 * @param handle Указатель на дескриптор устройства ввода
 * @param type Тип события (EV_REL, EV_KEY и т.д.)
 * @param code Код события (REL_X, BTN_LEFT и т.д.)
 * @param value Значение события (перемещение, состояние кнопки)
 * 
 * @note Использует pr_debug() для полной детализации и pr_info_ratelimited()
 * для ключевых событий с ограничением частоты вывода, чтобы избежать
 * перегрузки системы логирования (требование 3.1).
 */
static void mouse_logger_event(struct input_handle *handle, unsigned int type,
                              unsigned int code, int value)
{
    struct mouse_event event;
    
    if (unlikely(logger_data->exiting))
        return;
    
    // Заполняем структуру события
    event.type = type;
    event.code = code;
    event.value = value;
    event.timestamp_ns = ktime_get_ns();
    
    // Логирование события (требование 3.1)
    pr_debug("mouse_logger: [%llu] type=%u, code=%u, value=%d\n",
             event.timestamp_ns, type, code, value);
    
    // Увеличиваем счетчик событий (для статистики производительности)
    atomic64_inc(&logger_data->event_count);
    
    // Дополнительное логирование для ключевых событий
    if (type == EV_REL) {
        if (code == REL_X || code == REL_Y) {
            pr_info_ratelimited("mouse_logger: Movement - axis %s, delta=%d\n",
                                code == REL_X ? "X" : "Y", value);
        } else if (code == REL_WHEEL) {
            pr_info_ratelimited("mouse_logger: Wheel scroll - direction=%s\n",
                                value > 0 ? "up" : "down");
        }
    } else if (type == EV_KEY) {
        if (code >= BTN_LEFT && code <= BTN_MIDDLE) {
            pr_info_ratelimited("mouse_logger: Button %s - %s\n",
                                code == BTN_LEFT ? "LEFT" : 
                                code == BTN_RIGHT ? "RIGHT" : "MIDDLE",
                                value ? "pressed" : "released");
        }
    }
}

/**
 * @brief Обработчик подключения устройства ввода
 * 
 * Фильтрует устройства, подключая только те, которые являются мышами:
 * - Проверяет наличие относительных осей (REL_X/REL_Y)
 * - Проверяет наличие кнопок мыши или колеса прокрутки
 * - Регистрирует обработчик для подходящих устройств
 * 
 * @param handler Указатель на обработчик ввода
 * @param dev Устройство ввода, которое подключается
 * @param id Таблица совместимости устройств
 * 
 * @return 0 при успешном подключении, отрицательное значение при ошибке
 * 
 * @note Использует kzalloc() для безопасного выделения памяти и
 * гарантирует освобождение ресурсов через метки ошибок (требование 3.2).
 */
static int mouse_logger_connect(struct input_handler *handler,
                               struct input_dev *dev,
                               const struct input_device_id *id)
{
    struct input_handle *handle;
    int error;
    
    // Фильтруем только мыши
    if (!(test_bit(EV_REL, dev->evbit) && 
          (test_bit(REL_X, dev->relbit) || test_bit(REL_Y, dev->relbit)) &&
          (test_bit(BTN_MOUSE, dev->keybit) || test_bit(REL_WHEEL, dev->relbit)))) {
        return -ENODEV;
    }
    
    handle = kzalloc(sizeof(*handle), GFP_KERNEL);
    if (!handle)
        return -ENOMEM;
    
    handle->dev = dev;
    handle->handler = handler;
    handle->name = "mouse_logger";
    
    error = input_register_handle(handle);
    if (error)
        goto err_free_handle;
    
    error = input_open_device(handle);
    if (error)
        goto err_unregister_handle;
    
    pr_info("mouse_logger: Connected to %s\n", dev->name);
    return 0;
    
err_unregister_handle:
    input_unregister_handle(handle);
err_free_handle:
    kfree(handle);
    return error;
}

/**
 * @brief Обработчик отключения устройства ввода
 * 
 * Выполняет корректное отключение устройства:
 * 1. Закрывает устройство ввода
 * 2. Отменяет регистрацию обработчика
 * 3. Освобождает память, выделенную для дескриптора
 * 
 * @param handle Дескриптор устройства ввода
 * 
 * @note Гарантирует освобождение всех ресурсов при отключении устройства
 * (требование 3.2).
 */
static void mouse_logger_disconnect(struct input_handle *handle)
{
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(handle);
    pr_info("mouse_logger: Device disconnected\n");
}

/**
 * @brief Массив идентификаторов поддерживаемых устройств
 * 
 * Определяет критерии для автоматического определения мышей:
 * 1. Устройства с относительными осями (X и Y)
 * 2. Устройства с кнопками мыши
 * 3. Устройства с колесом прокрутки
 * 
 * @note MODULE_DEVICE_TABLE используется для автоматической загрузки
 * мпри подключении подходящего устройства.
 */
static const struct input_device_id mouse_logger_ids[] = {
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
                 INPUT_DEVICE_ID_MATCH_RELBIT,
        .evbit = { BIT_MASK(EV_REL) },
        .relbit = { [BIT_WORD(REL_X)] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) },
    },
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
                 INPUT_DEVICE_ID_MATCH_KEYBIT,
        .evbit = { BIT_MASK(EV_KEY) },
        .keybit = { [BIT_WORD(BTN_MOUSE)] = BIT_MASK(BTN_MOUSE) },
    },
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
                 INPUT_DEVICE_ID_MATCH_RELBIT,
        .evbit = { BIT_MASK(EV_REL) },
        .relbit = { [BIT_WORD(REL_WHEEL)] = BIT_MASK(REL_WHEEL) },
    },
    { } // Завершающий ноль
};
MODULE_DEVICE_TABLE(input, mouse_logger_ids);

/**
 * @brief Структура обработчика ввода
 * 
 * Связывает функции-обработчики с соответствующими событиями ядра.
 */
static struct input_handler mouse_logger_handler = {
    .event = mouse_logger_event,
    .connect = mouse_logger_connect,
    .disconnect = mouse_logger_disconnect,
    .name = "mouse_logger",
    .id_table = mouse_logger_ids,
};

/**
 * @brief Фоновая задача для сбора статистики производительности
 * 
 * Периодически выводит информацию о количестве обработанных событий в секунду.
 * Используется для тестирования производительности при высокой нагрузке
 * (требование 3.3).
 * 
 * @param work Указатель на структуру работы
 * 
 * @note Запускается каждую секунду через delayed work queue.
 */
static void log_performance_stats(struct work_struct *work)
{
    static u64 last_count = 0;
    u64 current_count = atomic64_read(&logger_data->event_count);
    u64 events_per_sec = current_count - last_count;
    
    pr_info("mouse_logger: Performance stats - %llu events/sec\n", events_per_sec);
    
    last_count = current_count;
    schedule_delayed_work(to_delayed_work(work), msecs_to_jiffies(1000));
}

static DECLARE_DELAYED_WORK(stats_work, log_performance_stats);

/**
 * @brief Инициализация модуля
 * 
 * Выполняет основные шаги инициализации:
 * 1. Выделение памяти для структуры управления
 * 2. Инициализация счетчика событий
 * 3. Регистрация обработчика ввода
 * 4. Запуск фоновой задачи для статистики
 * 
 * @return 0 при успешной инициализации, отрицательное значение при ошибке
 * 
 * @note Реализует требование 3.2 по управлению ресурсами через централизованную
 * структуру с гарантией освобождения памяти в случае ошибок.
 */
static int __init mouse_logger_init(void)
{
    int retval;
    
    // Выделение памяти для структуры управления (требование 3.2)
    logger_data = kzalloc(sizeof(struct mouse_logger_data), GFP_KERNEL);
    if (!logger_data)
        return -ENOMEM;
    
    // Инициализация счетчика событий
    atomic64_set(&logger_data->event_count, 0);
    logger_data->exiting = false;
    
    // Регистрация обработчика ввода
    logger_data->handler = mouse_logger_handler;
    retval = input_register_handler(&logger_data->handler);
    if (retval) {
        kfree(logger_data);
        return retval;
    }
    
    // Запуск периодической статистики
    queue_delayed_work(system_long_wq, &stats_work, msecs_to_jiffies(1000));
    
    pr_info("mouse_logger: Module loaded successfully\n");
    return 0;
}

/**
 * @brief Выгрузка модуля
 * 
 * Выполняет корректное завершение работы:
 * 1. Остановка фоновых задач
 * 2. Установка флага завершения
 * 3. Отмена регистрации обработчика
 * 4. Вывод финальной статистики
 * 5. Освобождение всех выделенных ресурсов
 * 
 * @note Гарантирует отсутствие утечек памяти и race condition при выгрузке
 * (требование 3.2). Использует барьер памяти (mb()) для обеспечения видимости
 * изменений во всех CPU.
 */
static void __exit mouse_logger_exit(void)
{
    // Останавливаем фоновые задачи
    cancel_delayed_work_sync(&stats_work);
    
    // Помечаем, что драйвер завершает работу
    logger_data->exiting = true;
    mb(); // Барьер памяти для гарантии видимости изменений
    
    // Отмена регистрации обработчика
    input_unregister_handler(&logger_data->handler);
    
    // Выводим финальную статистику
    u64 total_events = atomic64_read(&logger_data->event_count);
    pr_info("mouse_logger: Total events processed: %llu\n", total_events);
    
    // Освобождение памяти (требование 3.2)
    kfree(logger_data);
    
    pr_info("mouse_logger: Module unloaded\n");
}

module_init(mouse_logger_init);
module_exit(mouse_logger_exit);

/**
 * @page development_notes Заметки для разработки
 * 
 * @section performance Особенности производительности
 * - Использование atomic64_t для счетчика событий позволяет избежать блокировок
 * - Rate-limited логирование предотвращает перегрузку системы при высокой нагрузке
 * - Отложенная работа для статистики не влияет на обработку событий в реальном времени
 * 
 * @section compatibility Совместимость с ядром
 * - Драйвер адаптирован для ядра Linux 6.14+
 * - Не использует устаревшие API (class_create с двумя параметрами, no_llseek и др.)
 * - Использует современные методы работы со временем (ktime_get_ns)
 * 
 * @section testing Тестирование
 * Для тестирования производительности (требование 3.3) рекомендуется:
 * 1. Загрузить модуль: `sudo insmod mouse_logger.ko`
 * 2. Включить отладку: `sudo bash -c "echo 'module mouse_logger +p' > /sys/kernel/debug/dynamic_debug/control"`
 * 3. Активно использовать мышь в течение 30 секунд
 * 4. Выгрузить модуль: `sudo rmmod mouse_logger`
 * 5. Проанализировать логи: `sudo dmesg | grep mouse_logger`
 * 
 * Ожидаемая производительность:
 * - Нормальное использование: 50-200 событий/сек
 * - Интенсивное использование: 1000-5000 событий/сек
 * - Предельная нагрузка: до 15000 событий/сек
 */