import cv2
import numpy as np

def compare_images(cpu_path, gpu_path):
    img_cpu = cv2.imread(cpu_path)
    img_gpu = cv2.imread(gpu_path)

    if img_cpu is None:
        print(f"Ошибка: не удалось загрузить изображение {cpu_path}")
        return
    if img_gpu is None:
        print(f"Ошибка: не удалось загрузить изображение {gpu_path}")
        return

    print(f"Размер CPU изображения: {img_cpu.shape}")
    print(f"Размер GPU изображения: {img_gpu.shape}")

    if img_cpu.shape != img_gpu.shape:
        print("Размеры изображений не совпадают. Изменяем размер GPU изображения...")
        img_gpu = cv2.resize(img_gpu, (img_cpu.shape[1], img_cpu.shape[0]))
        print(f"Новый размер GPU изображения: {img_gpu.shape}")

    difference = cv2.absdiff(img_cpu, img_gpu)

    gray_diff = cv2.cvtColor(difference, cv2.COLOR_BGR2GRAY)
    _, threshold_diff = cv2.threshold(gray_diff, 10, 255, cv2.THRESH_BINARY)

    diff_percentage = np.mean(gray_diff) / 255 * 100
    print(f"Процент различий: {diff_percentage:.2f}%")

    cv2.imshow('CPU Render', img_cpu)
    cv2.imshow('GPU Render', img_gpu)
    
    cv2.imshow('Difference (Enhanced)', threshold_diff)

    cv2.imshow('Difference (Raw)', difference)

    print("Нажмите любую клавишу для закрытия окон...")
    cv2.waitKey(0)
    cv2.destroyAllWindows()


compare_images('lab3_cpu_result.png', 'lab4_gpu_result.png')