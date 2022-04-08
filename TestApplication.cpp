// Using visual studio the following program should be able to output the numbers 1 to 1000 in order
// Output is to be in order with no over lapping text/lines ... well formatted.
// Must use 2 threads
// Both threads must increment the counter.
// Does not use a global

#include <windows.h>
#include <iostream>

DWORD WINAPI countIt(LPVOID var)
{
    int* p = (int*)var;
    while (g_Counter < *p)
    {
        g_Counter++;
        std::cout << "Count " << g_Counter << "\r\n";
    }
    return g_Counter;
}

int main()
{
    int value = 1000;
    DWORD th1Id, th2Id;
    HANDLE th[2];
    th[0] = CreateThread(NULL, 0, countIt, &value, 0, &th1Id);
    th[1] = CreateThread(NULL, 0, countIt, &value, 0, &th1Id);

    WaitForMultipleObjects(2, th, TRUE, 1000);

    std::cout << "Numbers are listed from 1 to 1000 in order!\r\n";
}

