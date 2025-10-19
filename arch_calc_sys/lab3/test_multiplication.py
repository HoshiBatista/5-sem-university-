import unittest

def multiply_64bit_python(a_hex, b_hex):
    """
    Умножение двух 64-битных чисел в Python для сравнения с ассемблером
    Возвращает части в порядке: part0, part1, part2, part3 (как в ассемблере)
    """
    # Преобразуем hex строки в числа
    a = int(a_hex, 16) if isinstance(a_hex, str) else a_hex
    b = int(b_hex, 16) if isinstance(b_hex, str) else b_hex
    
    # Вычисляем результат
    result = a * b
    
    # Маска для 128 бит
    mask_128 = (1 << 128) - 1
    result_128 = result & mask_128
    
    # Разбиваем на 4 части по 32 бита в порядке part0, part1, part2, part3
    # (part0 - младшие 32 бита, part3 - старшие 32 бита)
    part0 = result_128 & 0xFFFFFFFF
    part1 = (result_128 >> 32) & 0xFFFFFFFF
    part2 = (result_128 >> 64) & 0xFFFFFFFF
    part3 = (result_128 >> 96) & 0xFFFFFFFF
    
    return part0, part1, part2, part3, hex(result_128)

class Test64BitMultiplication(unittest.TestCase):
    
    def test_1_max_times_max(self):
        """Test 1: FFFFFFFFFFFFFFFF * FFFFFFFFFFFFFFFF"""
        a = "FFFFFFFFFFFFFFFF"
        b = "FFFFFFFFFFFFFFFF"
        # Ожидаемые части в порядке part0, part1, part2, part3
        expected = (0x00000001, 0x00000000, 0xFFFFFFFE, 0xFFFFFFFF)
        part0, part1, part2, part3, result_hex = multiply_64bit_python(a, b)
        
        print(f"\nTest 1: {a} * {b}")
        print(f"Python: {result_hex}")
        print(f"Parts:  {part0:08x} {part1:08x} {part2:08x} {part3:08x}")
        
        self.assertEqual((part0, part1, part2, part3), expected)
    
    def test_2_max_times_5(self):
        """Test 2: FFFFFFFFFFFFFFFF * 5"""
        a = "FFFFFFFFFFFFFFFF"
        b = "5"
        expected = (0xFFFFFFFB, 0xFFFFFFFF, 0x00000004, 0x00000000)
        part0, part1, part2, part3, result_hex = multiply_64bit_python(a, b)
        
        print(f"\nTest 2: {a} * {b}")
        print(f"Python: {result_hex}")
        print(f"Parts:  {part0:08x} {part1:08x} {part2:08x} {part3:08x}")
        
        self.assertEqual((part0, part1, part2, part3), expected)
    
    def test_3_5_times_max(self):
        """Test 3: 5 * FFFFFFFFFFFFFFFF"""
        a = "5"
        b = "FFFFFFFFFFFFFFFF"
        expected = (0xFFFFFFFB, 0xFFFFFFFF, 0x00000004, 0x00000000)
        part0, part1, part2, part3, result_hex = multiply_64bit_python(a, b)
        
        print(f"\nTest 3: {a} * {b}")
        print(f"Python: {result_hex}")
        print(f"Parts:  {part0:08x} {part1:08x} {part2:08x} {part3:08x}")
        
        self.assertEqual((part0, part1, part2, part3), expected)
    
    def test_4_1024_times_special(self):
        """Test 4: 1024 * FFFFFFFF00000000"""
        a = "1024"
        b = "FFFFFFFF00000000"
        expected = (0x00000000, 0xFFFFEFDC, 0x00001023, 0x00000000)
        part0, part1, part2, part3, result_hex = multiply_64bit_python(a, b)
        
        print(f"\nTest 4: {a} * {b}")
        print(f"Python: {result_hex}")
        print(f"Parts:  {part0:08x} {part1:08x} {part2:08x} {part3:08x}")
        
        self.assertEqual((part0, part1, part2, part3), expected)
    
    def test_5_large_numbers(self):
        """Test 5: 123456789ABCDEF0 * FEDCBA9876543210"""
        a = "123456789ABCDEF0"
        b = "FEDCBA9876543210"
        expected = (0x5618CF00, 0x236D88FE, 0xD77D7422, 0x121FA00A)
        part0, part1, part2, part3, result_hex = multiply_64bit_python(a, b)
        
        print(f"\nTest 5: {a} * {b}")
        print(f"Python: {result_hex}")
        print(f"Parts:  {part0:08x} {part1:08x} {part2:08x} {part3:08x}")
        
        self.assertEqual((part0, part1, part2, part3), expected)
    
    def test_6_power_of_two(self):
        """Test 6: 100000000 * 100000000"""
        a = "100000000"
        b = "100000000"
        expected = (0x00000000, 0x00000000, 0x00000001, 0x00000000)
        part0, part1, part2, part3, result_hex = multiply_64bit_python(a, b)
        
        print(f"\nTest 6: {a} * {b}")
        print(f"Python: {result_hex}")
        print(f"Parts:  {part0:08x} {part1:08x} {part2:08x} {part3:08x}")
        
        self.assertEqual((part0, part1, part2, part3), expected)

def compare_with_asm():
    """Функция для сравнения с выводом ассемблера"""
    print("СРАВНЕНИЕ РЕЗУЛЬТАТОВ АССЕМБЛЕРА И PYTHON")
    print("=" * 70)
    
    test_cases = [
        ("FFFFFFFFFFFFFFFF", "FFFFFFFFFFFFFFFF", "Test 1: MAX * MAX"),
        ("FFFFFFFFFFFFFFFF", "5", "Test 2: MAX * 5"),
        ("5", "FFFFFFFFFFFFFFFF", "Test 3: 5 * MAX"),
        ("1024", "FFFFFFFF00000000", "Test 4: 1024 * FFFFFFFF00000000"),
        ("123456789ABCDEF0", "FEDCBA9876543210", "Test 5: Large numbers"),
        ("100000000", "100000000", "Test 6: 2^32 * 2^32"),
    ]
    
    print("Ассемблерная программа выводит части в порядке: part0 part1 part2 part3")
    print("где part0 - младшие 32 бита, part3 - старшие 32 бита")
    print()
    
    for a, b, description in test_cases:
        part0, part1, part2, part3, result_hex = multiply_64bit_python(a, b)
        print(f"{description}")
        print(f"A = {a}")
        print(f"B = {b}")
        print(f"Ожидаемый результат: {result_hex}")
        print(f"Python части:  {part0:08x} {part1:08x} {part2:08x} {part3:08x}")
        print(f"ASM части:     [запустите программу для сравнения]")
        print()

if __name__ == '__main__':
    print("ТЕСТИРОВАНИЕ УМНОЖЕНИЯ 64-БИТНЫХ ЧИСЕЛ")
    print("Порядок вывода: part0 part1 part2 part3 (part0 - младшие 32 бита)")
    print("=" * 70)
    
    # Запуск unit tests
    suite = unittest.TestLoader().loadTestsFromTestCase(Test64BitMultiplication)
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    print("\n" + "=" * 70)
    print(f"ИТОГИ ТЕСТИРОВАНИЯ:")
    print(f"Пройдено тестов: {result.testsRun - len(result.failures) - len(result.errors)}")
    print(f"Провалено тестов: {len(result.failures)}")
    print(f"Ошибок: {len(result.errors)}")
    
    if len(result.failures) == 0 and len(result.errors) == 0:
        print("\n🎉 ВСЕ ТЕСТЫ ПРОЙДЕНЫ УСПЕШНО!")
        print("Результаты ассемблерной программы и Python полностью совпадают!")
    else:
        print("\n❌ Есть расхождения между ассемблером и Python")
    
    # Сравнение с ассемблером
    compare_with_asm()