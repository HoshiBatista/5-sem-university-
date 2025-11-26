import cv2
import numpy as np
import os

def compare_images(cpu_path, gpu_path):
    # Загружаем изображения
    img_cpu = cv2.imread(cpu_path)
    img_gpu = cv2.imread(gpu_path)

    # Проверяем загрузку изображений
    if img_cpu is None:
        print(f"Ошибка: не удалось загрузить изображение {cpu_path}")
        return
    if img_gpu is None:
        print(f"Ошибка: не удалось загрузить изображение {gpu_path}")
        return

    print(f"Размер CPU изображения: {img_cpu.shape}")
    print(f"Размер GPU изображения: {img_gpu.shape}")

    # Проверяем и выравниваем размеры при необходимости
    if img_cpu.shape != img_gpu.shape:
        print("Размеры изображений не совпадают. Изменяем размер GPU изображения...")
        img_gpu = cv2.resize(img_gpu, (img_cpu.shape[1], img_cpu.shape[0]))
        print(f"Новый размер GPU изображения: {img_gpu.shape}")

    # Вычисляем разницу между изображениями
    difference = cv2.absdiff(img_cpu, img_gpu)
    
    # Создаем бинарную маску различий
    gray_diff = cv2.cvtColor(difference, cv2.COLOR_BGR2GRAY)
    _, threshold_diff = cv2.threshold(gray_diff, 30, 255, cv2.THRESH_BINARY)

    # Вычисляем процент различий
    diff_percentage = np.sum(threshold_diff) / (threshold_diff.size * 255) * 100
    print(f"Процент различий: {diff_percentage:.4f}%")

    # Создаем визуализацию различий
    highlighted_diff = img_cpu.copy()
    highlighted_diff[threshold_diff > 0] = [0, 0, 255]  # Помечаем различия красным

    # Показываем все результаты
    cv2.imshow('CPU Render', img_cpu)
    cv2.imshow('GPU Render', img_gpu)
    cv2.imshow('Difference Mask', threshold_diff)
    cv2.imshow('Highlighted Differences', highlighted_diff)

    print("\nИнформация о сравнении:")
    print(f"- Порог чувствительности: 30/255")
    print(f"- Различия выделены красным цветом")
    print(f"- Совпадение: {100 - diff_percentage:.2f}%")
    print("\nНажмите любую клавишу для закрытия окон...")
    
    cv2.waitKey(0)
    cv2.destroyAllWindows()

    # Сохраняем результат сравнения
    cv2.imwrite('comparison_result.png', highlighted_diff)
    print("Результат сравнения сохранен как 'comparison_result.png'")

if __name__ == "__main__":
    # Автоматический поиск путей к файлам
    cpu_path = 'lab3_cpu_result.bmp'
    gpu_path = 'static_letters.bmp'  # Предполагаем, что GPU результат в папке lab4_gpu

    # Проверяем существование файлов
    if not os.path.exists(cpu_path):
        print(f"Файл CPU не найден: {cpu_path}")
        exit(1)
        
    if not os.path.exists(gpu_path):
        print(f"Файл GPU не найден: {gpu_path}")
        print("Проверьте путь к GPU результату")
        exit(1)

    compare_images(cpu_path, gpu_path)