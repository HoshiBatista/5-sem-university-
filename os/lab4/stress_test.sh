#!/bin/bash
echo "Начало тестирования драйвера mouse_logger..."
echo "Двигайте мышью максимально активно в течение 30 секунд"

# Создаем файл для логов
LOG_FILE="mouse_logger_test_$(date +%Y%m%d_%H%M%S).log"
echo "Логи будут сохранены в: $LOG_FILE"

# Очищаем буфер dmesg и начинаем запись
sudo dmesg -C
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Начало тестирования" > $LOG_FILE

# Включаем отладочные сообщения для модуля
if [ -f /sys/kernel/debug/dynamic_debug/control ]; then
    sudo bash -c "echo 'module mouse_logger +p' > /sys/kernel/debug/dynamic_debug/control" 2>/dev/null
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Отладочные сообщения включены" >> $LOG_FILE
else
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Не удалось включить отладочные сообщения (файл debug/control отсутствует)" >> $LOG_FILE
    echo "Попробуйте: sudo modprobe dynamic_debug" >> $LOG_FILE
fi

# Проверяем, загружен ли модуль
if ! lsmod | grep -q mouse_logger; then
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] ОШИБКА: Модуль mouse_logger не загружен!" >> $LOG_FILE
    echo "Загрузите модуль командой: sudo insmod mouse_logger.ko"
    exit 1
fi

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Модуль mouse_logger загружен успешно" >> $LOG_FILE

# Запускаем мониторинг в фоне
sudo dmesg -wH > dmesg_temp.log &
DMESG_PID=$!

# Ждем 1 секунду для стабилизации
sleep 1

# Основной цикл тестирования (30 секунд)
START_TIME=$(date +%s)
for i in {1..30}; do
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - START_TIME))
    
    # Показываем прогресс
    echo -ne "Осталось времени: $((30 - ELAPSED)) секунд    \r"
    
    # Каждые 5 секунд выводим статистику
    if [ $((ELAPSED % 5)) -eq 0 ] && [ $ELAPSED -gt 0 ]; then
        EVENTS=$(sudo dmesg | grep -c "mouse_logger:")
        EVENTS_PER_SEC=$((EVENTS / ELAPSED))
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] Прогресс: $ELAPSED/30 сек, событий: $EVENTS ($EVENTS_PER_SEC/сек)" >> $LOG_FILE
    fi
    
    sleep 1
done
echo -e "\nТестирование завершено!"

# Останавливаем мониторинг dmesg
sudo kill $DMESG_PID 2>/dev/null
wait $DMESG_PID 2>/dev/null

# Собираем финальные данные
sudo dmesg | grep "mouse_logger:" > driver_events.log
TOTAL_EVENTS=$(wc -l < driver_events.log)

# Анализируем результаты
echo "" >> $LOG_FILE
echo "[$(date '+%Y-%m-%d %H:%M:%S')] === ФИНАЛЬНЫЙ ОТЧЕТ ===" >> $LOG_FILE
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Общее количество обработанных событий: $TOTAL_EVENTS" >> $LOG_FILE

# Анализ по типам событий
MOVEMENT_EVENTS=$(grep -c "Movement - axis" driver_events.log)
BUTTON_EVENTS=$(grep -c "Button" driver_events.log)
WHEEL_EVENTS=$(grep -c "Wheel scroll" driver_events.log)
PERF_STATS=$(grep -c "Performance stats" driver_events.log)

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Типы событий:" >> $LOG_FILE
echo "[$(date '+%Y-%m-%d %H:%M:%S')]   Движение мыши: $MOVEMENT_EVENTS" >> $LOG_FILE
echo "[$(date '+%Y-%m-%d %H:%M:%S')]   Нажатия кнопок: $BUTTON_EVENTS" >> $LOG_FILE
echo "[$(date '+%Y-%m-%d %H:%M:%S')]   Прокрутка колеса: $WHEEL_EVENTS" >> $LOG_FILE
echo "[$(date '+%Y-%m-%d %H:%M:%S')]   Записей статистики: $PERF_STATS" >> $LOG_FILE

# Показываем последние записи статистики производительности
echo "" >> $LOG_FILE
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Последние записи статистики производительности:" >> $LOG_FILE
sudo dmesg | grep "Performance stats" | tail -5 >> $LOG_FILE

# Выгружаем модуль
sudo rmmod mouse_logger 2>/dev/null
if [ $? -eq 0 ]; then
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Модуль mouse_logger успешно выгружен" >> $LOG_FILE
else
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Предупреждение: Не удалось выгрузить модуль (возможно, он уже выгружен)" >> $LOG_FILE
fi

# Выводим сводку на экран
echo ""
echo "=== СВОДКА ТЕСТИРОВАНИЯ ==="
echo "Общее количество событий: $TOTAL_EVENTS"
echo "Движение мыши: $MOVEMENT_EVENTS"
echo "Нажатия кнопок: $BUTTON_EVENTS"
echo "Прокрутка колеса: $WHEEL_EVENTS"
echo ""
echo "Детальный отчет сохранен в: $LOG_FILE"
echo "Сырые события сохранены в: driver_events.log"
echo ""
echo "Просмотреть логи можно командой:"
echo "cat $LOG_FILE"