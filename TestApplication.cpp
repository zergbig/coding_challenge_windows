// Using visual studio the following program should be able to output the numbers 1 to 1000 in order
// Output is to be in order with no over lapping text/lines ... well formatted.
// Must use 2 threads
// Both threads must increment the counter.
// Does not use a global

#include <windows.h>
#include <iostream>
#include <mutex>

struct vars
{
	int g_Counter;
	int value;
	std::mutex mtx;
};

///////////////////////////////////////////////////////////////
// countIt - THREAD SAFE 
// 
// input - vars struct
// output  - DWORD counter value
//
// purpose: count up to a specified value, printing the current count to
// stdout on a new line
//
DWORD WINAPI countIt(LPVOID var)
{
	vars* v = (vars*)var;
	{
		// critical section: updating g_Counter
		std::unique_lock<std::mutex> uLock{ v->mtx };
		while (v->g_Counter < v->value)
		{
			v->g_Counter++;
			std::cout << "Count " << v->g_Counter << "\r\n";
		}
	}
	return v->g_Counter;
}

int main()
{
	DWORD th1Id, th2Id;
	HANDLE th[2];
	vars v{ 0, 1000 };

	th[0] = CreateThread(NULL, 0, countIt, &v, 0, &th1Id);
	th[1] = CreateThread(NULL, 0, countIt, &v, 0, &th2Id);

	WaitForMultipleObjects(2, th, TRUE, 1000);

	std::cout << "Numbers are listed from 1 to 1000 in order!\r\n";
}
