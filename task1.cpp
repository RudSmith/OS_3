#include <windows.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#define THREAD_COUNT 4
#define TOTAL_ITERATIONS 1000000000
#define BLOCK_SIZE 3316220

struct SharedData {
    double piSum;
    CRITICAL_SECTION cs;
    int nextIteration;
    bool shutdownRequested;
};

struct ThreadData {
    int threadId;
    HANDLE hThread;
    bool isSuspended;
    SharedData* shared;
};

DWORD WINAPI ThreadFunction(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    SharedData* shared = data->shared;

    while (true) {
        // Проверяем запрос на завершение
        EnterCriticalSection(&shared->cs);
        if (shared->shutdownRequested) {
            LeaveCriticalSection(&shared->cs);
            break;
        }
        LeaveCriticalSection(&shared->cs);

        // Получаем блок работы
        int startIteration = -1;
        EnterCriticalSection(&shared->cs);
        if (shared->nextIteration < TOTAL_ITERATIONS) {
            startIteration = shared->nextIteration;
            shared->nextIteration += BLOCK_SIZE;
        }
        LeaveCriticalSection(&shared->cs);

        if (startIteration == -1) {
            // Нет работы - приостанавливаемся
            data->isSuspended = true;
            SuspendThread(data->hThread);
            continue;
        }

        // Вычисляем часть суммы для π
        double blockSum = 0.0;
        for (int i = startIteration; i < startIteration + BLOCK_SIZE && i < TOTAL_ITERATIONS; i++) {
            double x = (i + 0.5) * (1.0 / TOTAL_ITERATIONS);
            blockSum += 4.0 / (1.0 + x * x);
        }

        // Добавляем к общей сумме
        EnterCriticalSection(&shared->cs);
        shared->piSum += blockSum;
        LeaveCriticalSection(&shared->cs);

        std::stringstream ss;
        ss << "Thread " << data->threadId << " processed iterations "
            << startIteration << "-" << (startIteration + BLOCK_SIZE - 1)
            << " (partial sum = " << blockSum << ")\n";
        std::cout << ss.str();

        // Приостанавливаемся после работы
        data->isSuspended = true;
        SuspendThread(data->hThread);
    }
    return 0;
}

int main() {
    HANDLE hThreads[THREAD_COUNT];
    ThreadData threadData[THREAD_COUNT];
    SharedData shared;

    // Инициализация
    shared.piSum = 0.0;
    shared.nextIteration = 0;
    shared.shutdownRequested = false;
    InitializeCriticalSection(&shared.cs);

    // Создаем потоки
    for (int i = 0; i < THREAD_COUNT; i++) {
        threadData[i].threadId = i + 1;
        threadData[i].isSuspended = false;
        threadData[i].shared = &shared;

        hThreads[i] = CreateThread(
            NULL, 0, ThreadFunction, &threadData[i], CREATE_SUSPENDED, NULL);

        threadData[i].hThread = hThreads[i];
        threadData[i].isSuspended = true;
        std::cout << "Created thread " << threadData[i].threadId << "\n";
    }

    // Запускаем все потоки
    for (int i = 0; i < THREAD_COUNT; i++) {
        ResumeThread(hThreads[i]);
        threadData[i].isSuspended = false;
    }

    // Главный цикл управления
    while (true) {
        EnterCriticalSection(&shared.cs);
        bool workDone = (shared.nextIteration >= TOTAL_ITERATIONS);
        LeaveCriticalSection(&shared.cs);

        if (workDone) 
            break;

        // Возобновляем приостановленные потоки
        for (int i = 0; i < THREAD_COUNT; i++) {
            if (threadData[i].isSuspended) {
                ResumeThread(hThreads[i]);
                threadData[i].isSuspended = false;
            }
        }
        Sleep(100);
    }

    // Корректное завершение
    EnterCriticalSection(&shared.cs);
    shared.shutdownRequested = true;
    LeaveCriticalSection(&shared.cs);

    // Возобновляем все потоки для выхода
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (threadData[i].isSuspended) {
            ResumeThread(hThreads[i]);
        }
    }

    // Ожидаем завершения
    WaitForMultipleObjects(THREAD_COUNT, hThreads, TRUE, INFINITE);

    // Вычисляем окончательное значение Pi
    double pi = shared.piSum * (1.0 / TOTAL_ITERATIONS);

    // Освобождаем ресурсы
    for (int i = 0; i < THREAD_COUNT; i++) {
        CloseHandle(hThreads[i]);
    }
    DeleteCriticalSection(&shared.cs);

    // Выводим результат
    std::cout << std::setprecision(15);
    std::cout << "\nCalculated pi = " << pi << "\n";
    std::cout << "  Expected pi = 3.14159265358979\n";

    return 0;
}