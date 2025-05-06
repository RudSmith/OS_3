#include <iostream>
#include <iomanip>
#include <omp.h>
#include <windows.h>

#define TOTAL_ITERATIONS 1000000000
#define BLOCK_SIZE 331622
#define THREAD_COUNT 16

int main() {
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    double interval;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    long double pi_sum = 0.0L;
    const double step = 1.0L / TOTAL_ITERATIONS;

    // Устанавливаем количество потоков
    omp_set_num_threads(THREAD_COUNT);

    // Вычисляем π с динамическим распределением блоков
#pragma omp parallel for reduction(+:pi_sum) schedule(dynamic, BLOCK_SIZE)
    for (int i = 0; i < TOTAL_ITERATIONS; i++) {
        long double x = (i + 0.5L) * step;
        pi_sum += 4.0L / (1.0L + x * x);
    }

    // Финальное вычисление π
    double pi = pi_sum * step;

    // Вывод результата с максимальной точностью для double
    std::cout << std::setprecision(12);
    std::cout << "Calculated π = " << pi << std::endl;
    std::cout << "Expected π   = 3.14159265358979" << std::endl;

    QueryPerformanceCounter(&end);
    interval = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;

    printf("Time: %f\n", interval);

    return 0;
}