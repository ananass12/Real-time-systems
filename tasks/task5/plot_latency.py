import matplotlib.pyplot as plt
import numpy as np

def read_csv_data(filename):
    """Чтение данных из CSV файла без pandas"""
    iterations = []
    latencies = []
    
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    # Пропускаем заголовок и читаем данные
    for line in lines[1:]:  # пропускаем первую строку с заголовком
        parts = line.strip().split(',')
        if len(parts) >= 2:
            iterations.append(int(parts[0]))
            latencies.append(int(parts[1]))
    
    return iterations, latencies

def read_simple_benchmark(filename):
    """Чтение простого формата бенчмарка"""
    iterations = []
    latencies = []
    
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    for line in lines[1:]:  # пропускаем заголовок
        parts = line.strip().split(',')
        if len(parts) >= 2:
            iterations.append(int(parts[0]))
            latencies.append(int(parts[1]))
    
    return iterations, latencies

# Чтение данных из файлов
try:
    task1_iter, task1_lat = read_csv_data('task1_data.csv')
    task2_iter, task2_lat = read_csv_data('task2_data.csv')
    malloc_iter, malloc_lat = read_simple_benchmark('malloc_benchmark.csv')
    pool_iter, pool_lat = read_simple_benchmark('pool_benchmark.csv')
except FileNotFoundError as e:
    print(f"Файлы данных не найдены: {e}")
    print("Сначала запустите программы task1, task2 и task3_benchmark")
    exit(1)

plt.figure(figsize=(16, 12))

# График 1: Детальный вид Задания 1
plt.subplot(3, 2, 1)
plt.plot(task1_iter, task1_lat, 'r-', alpha=0.8, linewidth=1)
plt.title('Задание 1: Page Faults (без mlockall)')
plt.xlabel('Итерация')
plt.ylabel('Латентность (нс)')
plt.grid(True, alpha=0.3)
plt.ylim(0, 50000)

# График 2: Детальный вид Задания 2
plt.subplot(3, 2, 2)
plt.plot(task2_iter, task2_lat, 'g-', alpha=0.8, linewidth=1)
plt.title('Задание 2: Стабильность с mlockall')
plt.xlabel('Итерация')
plt.ylabel('Латентность (нс)')
plt.grid(True, alpha=0.3)
plt.ylim(800, 2500)

# График 3: Сравнение malloc vs pool (Задание 3)
plt.subplot(3, 2, 3)
plt.plot(malloc_iter, malloc_lat, 'orange', alpha=0.7, linewidth=1, label='malloc/free')
plt.plot(pool_iter, pool_lat, 'purple', alpha=0.7, linewidth=1, label='pool_alloc/pool_free')
plt.title('Задание 3: malloc vs Memory Pool')
plt.xlabel('Итерация')
plt.ylabel('Латентность (нс)')
plt.legend()
plt.grid(True, alpha=0.3)

# График 4: Детальный вид malloc
plt.subplot(3, 2, 4)
plt.plot(malloc_iter, malloc_lat, 'orange', alpha=0.8, linewidth=1)
plt.title('Задание 3: alloc/free')
plt.xlabel('Итерация')
plt.ylabel('Латентность (нс)')
plt.grid(True, alpha=0.3)

# График 5: Детальный вид Memory Pool
plt.subplot(3, 2, 5)
plt.plot(pool_iter, pool_lat, 'purple', alpha=0.8, linewidth=1)
plt.title('Задание 3: Memory Pool')
plt.xlabel('Итерация')
plt.ylabel('Латентность (нс)')
plt.grid(True, alpha=0.3)

# График 6: Bar chart для средней латентности
plt.subplot(3, 2, 6)
avg_latencies = [np.mean(task1_lat), np.mean(task2_lat), np.mean(malloc_lat), np.mean(pool_lat)]
methods = ['Task1\n(no mlock)', 'Task2\n(mlock)', 'Task3\nmalloc', 'Task3\npool']
colors = ['lightcoral', 'lightblue', 'wheat', 'plum']
bars = plt.bar(methods, avg_latencies, color=colors, alpha=0.8)

for bar, value in zip(bars, avg_latencies):
    plt.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 1000, 
             f'{value:.0f} нс', ha='center', va='bottom', fontsize=10)

plt.title('Средняя латентность по методам')
plt.ylabel('Латентность (нс)')
plt.grid(True, alpha=0.3)

plt.tight_layout()
plt.savefig('detailed_analysis.png', dpi=300, bbox_inches='tight')
plt.show()

# Статистический анализ
print("=" * 60)
print("ПОЛНЫЙ АНАЛИЗ РЕЗУЛЬТАТОВ ВСЕХ ЗАДАНИЙ")
print("=" * 60)

print("\nЗАДАНИЕ 1 ---")
print(f"  • Средняя латентность: {np.mean(task1_lat):.0f} нс")
print(f"  • Максимальная латентность: {np.max(task1_lat)} нс")
print(f"  • Минимальная латентность: {np.min(task1_lat)} нс")

print("\nЗАДАНИЕ 2: mlockall ---")
print(f"  • Средняя латентность: {np.mean(task2_lat):.0f} нс")
print(f"  • Максимальная латентность: {np.max(task2_lat)} нс")

print("\nЗАДАНИЕ 3: malloc/free ---")
print(f"  • Средняя латентность: {np.mean(malloc_lat):.0f} нс")
print(f"  • Максимальная латентность: {np.max(malloc_lat)} нс")
print(f"  • Минимальная латентность: {np.min(malloc_lat)} нс")

print("\nЗАДАНИЕ 3: Memory Pool ---")
print(f"  • Средняя латентность: {np.mean(pool_lat):.0f} нс")
print(f"  • Максимальная латентность: {np.max(pool_lat)} нс")
print(f"  • Минимальная латентность: {np.min(pool_lat)} нс")
