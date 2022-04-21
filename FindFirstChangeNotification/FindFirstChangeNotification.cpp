// FindFirstChangeNotification.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "FindFirstChangeNotification.h"
#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

HANDLE g_hThreadExit = NULL;
HANDLE g_readyMutex = nullptr;
HANDLE g_notificationListMutex = nullptr;


void MyThread(void* param)
{
	DWORD dwWaitStatus;
	HANDLE dwChangeHandles[2];

	// Watch the C:\WINDOWS directory for file creation and 
	// deletion. 
	dwChangeHandles[0] = FindFirstChangeNotification(
		"C:\\Windows",                 // directory to watch 
		TRUE,                         // do not watch the subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY); // watch file name changes 

	if (dwChangeHandles[0] == INVALID_HANDLE_VALUE)
	{
		return;
	}

	// Watch the C:\ subtree for directory creation and 
	// deletion. 

	dwChangeHandles[1] = g_hThreadExit;
	DWORD drawListRet = WaitForSingleObject(g_readyMutex, INFINITE);

	// Change notification is set. Now wait on both notification 
	// handles and refresh accordingly. 
	BOOL doneFlag = false;
	while (doneFlag == false)
	{

		// Wait for notification.

		dwWaitStatus = WaitForMultipleObjects(2, dwChangeHandles, FALSE, INFINITE);

		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0:

			// A file was created or deleted in C:\WINDOWS. 
			// Refresh this directory and restart the 
			// change notification. RefreshDirectory is an 
			// application-defined function. 
			if (FindNextChangeNotification(dwChangeHandles[0]) != FALSE)
			{
				// Note from Greg: I used the dump to see the point of failure.
				// the main thread was getting stuck in GetReady() waiting for the 
				// g_readyMutex to be unlocked. This lead me to think that the 
				// condition for releasing the mutex was not being met. So doneFlag 
				// became a prime suspect. Since this is a stand-alone application, 
				// I was lucky enough to be able to use breakpoints and VS's thread 
				// tracing. I saw that the child thread was indeed getting stuck in this
				// while, which confirmed my suspicions. I investigated the cases here 
				// closley and saw that dwChangeHandles[0] == false is not a good return
				// from FindNextChangeNotification, it would actually indicate a failure

				doneFlag = true;
			}
			else
			{
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_OBJECT_0 + 1:

			// exit from loop and thread
			doneFlag = true;
			break;

		default:
			// exit from loop and thread
			doneFlag = true;
			break;
		}
	} // while doneFlag == false

	FindCloseChangeNotification(dwChangeHandles[0]);
	if (drawListRet == WAIT_OBJECT_0 || drawListRet == WAIT_ABANDONED)
	{
		ReleaseMutex(g_readyMutex);
	}

	return;
}


void GetReady()
{
	DWORD drawListRet = WaitForSingleObject(g_readyMutex, INFINITE);

	DWORD notificationRet = WaitForSingleObject(g_notificationListMutex, INFINITE);

	if (notificationRet == WAIT_OBJECT_0 || notificationRet == WAIT_ABANDONED)
	{
		ReleaseMutex(g_notificationListMutex);
	}
}


void DoneReady()
{
	ReleaseMutex(g_readyMutex);
}


int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	g_readyMutex = CreateMutex(NULL, FALSE, NULL);
	g_notificationListMutex = CreateMutex(NULL, FALSE, NULL);

	DWORD threadId = 0;
	g_hThreadExit = CreateEvent(NULL, 0, 0, NULL);
	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)MyThread, NULL, 0, &threadId);



	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_FINDFIRSTCHANGENOTIFICATION, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_FINDFIRSTCHANGENOTIFICATION);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	SetEvent(g_hThreadExit);
	WaitForSingleObject(hThread, INFINITE);

	return (int)msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_FINDFIRSTCHANGENOTIFICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = (LPCTSTR)IDC_FINDFIRSTCHANGENOTIFICATION;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}


//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}


//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	GetReady();

	switch (message)
	{
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	DoneReady();
	return 0;
}


// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
