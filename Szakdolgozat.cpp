// Szakdolgozat.cpp : Defines the entry point for the application.
//

#include "Szakdolgozat.h"

#include "framework.h"
#include <iostream>
#include <string.h>


#undef far
#undef near

bool ExitSimu;

#define MAX_LOADSTRING 100


// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    HowToCalibrate(HWND, UINT, WPARAM, LPARAM);

HFONT hFontCim;
HFONT hFontSzoveg;
HWND hComboBoxPort;
HWND hComboBoxSim;
HWND hBtCalibrate;
HWND hBtnStart;
HWND hWindow;
int SimIndex;
int PortIndex;
bool gotPort;
char* ListItem;
HBITMAP hBitmap = NULL;
Serial _serial;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// TODO: Place code here.

	// Initialize global strings
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_SZAKDOLGOZAT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SZAKDOLGOZAT));

	MSG msg;


	while (GetMessage(&msg, nullptr, 0, 0) && !ExitSimu)
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	/*char s[256];
	sprintf_s(s, "ASD");
	OutputDebugStringA(s);*/

	return (int)msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);
	HBRUSH hBrush = CreateSolidBrush(RGB(160, 160, 160));

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SZAKDOLGOZAT));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = hBrush;
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_SZAKDOLGOZAT);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassExW(&wcex);
}



//
//   FUNCTION: InitInstance(HINSTANCE, int)
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
	hInst = hInstance; // Store instance handle in our global variable
	hFontCim = CreateFont(40, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	hFontSzoveg = CreateFont(20, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");

	hWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		450, 200, 500, 350, nullptr, nullptr, hInstance, nullptr);
	if (!hWindow) return FALSE;

	hComboBoxPort = CreateWindow(TEXT("combobox"), TEXT(""), CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
		100, 100, 200, 200, hWindow, (HMENU)ID_CBSP, NULL, NULL);
	SendMessage(hComboBoxPort, WM_SETFONT, (WPARAM)hFontSzoveg, MAKELPARAM(TRUE, 0));
	//SetWindowLong(hComboBoxPort, GWL_ID, ID_CBSP);

	hComboBoxSim = CreateWindow(TEXT("combobox"), TEXT(""), CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
		100, 220, 200, 200, hWindow, (HMENU)ID_CBSIM, NULL, NULL);
	SendMessage(hComboBoxSim, WM_SETFONT, (WPARAM)hFontSzoveg, MAKELPARAM(TRUE, 0));
	//SetWindowLong(hComboBoxSim, GWL_ID, ID_CBSIM);
	fillComboBoxSim();

	HWND hBtnRefresh = CreateWindow(TEXT("button"), TEXT("Refresh"), WS_VISIBLE | WS_CHILD, 320, 100, 80, 30, hWindow, (HMENU)IDM_REFRESH, NULL, NULL);
	SendMessage(hBtnRefresh, WM_SETFONT, (WPARAM)hFontSzoveg, MAKELPARAM(TRUE, 0));

	hBtCalibrate = CreateWindow(TEXT("button"), TEXT("Calibrate"), WS_VISIBLE | WS_CHILD, 120, 160, 80, 30, hWindow, (HMENU)IDM_CALIB, NULL, NULL);
	SendMessage(hBtCalibrate, WM_SETFONT, (WPARAM)hFontSzoveg, MAKELPARAM(TRUE, 0));

	hBtnStart = CreateWindow(TEXT("button"), TEXT("Start"), WS_VISIBLE | WS_CHILD, 320, 220, 80, 30, hWindow, (HMENU)IDM_START, NULL, NULL);
	SendMessage(hBtnStart, WM_SETFONT, (WPARAM)hFontSzoveg, MAKELPARAM(TRUE, 0));

	EnableWindow(hBtnStart, false);
	EnableWindow(hBtCalibrate, false);

	hBitmap = (HBITMAP)LoadImage(hInst, L"kepek\\MOGI_65x80.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

	SimIndex = 0;
	ExitSimu = false;
	SelectComPort();

	ShowWindow(hWindow, nCmdShow);
	UpdateWindow(hWindow);
	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
	{
		break;
	}
	case WM_COMMAND:
	{
		int LwmId = LOWORD(wParam);
		int HwmId = HIWORD(wParam);

		if (HwmId == CBN_SELCHANGE)
		{
			if (LwmId == ID_CBSIM) {
				SimIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			}
			else if (LwmId == ID_CBSP) {
				PortIndex = SendMessage((HWND)lParam, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				TCHAR item[256];
				(TCHAR)SendMessage((HWND)lParam, (UINT)CB_GETLBTEXT, (WPARAM)PortIndex, (LPARAM)item);
				//OutputDebugString(item);
				ListItem = new char[wcslen(item) + 1];
				size_t i;
				wcstombs_s(&i, ListItem, wcslen(item) + 1, item, wcslen(item) + 1);
				EnableWindow(hBtnStart, false);
				//OutputDebugStringA(ListItem);
			}

		}

		switch (LwmId)
		{
		case IDM_START:
		{
			disableWindow();
			/*char s[256];
			sprintf_s(s, "%i\n", strlen(ListItem));
			OutputDebugStringA(ListItem);*/
			/*char* TAG = new char[5];
			strcpy_s(TAG, 5, "COM3");*/
			char** argv = new char* [1];
			argv[0] = ListItem;
			int argc = 1;
			ThesisGraphics::Configuration conf;
			conf.addWindowFlags(ThesisGraphics::Configuration::WindowFlag::Maximized);
			conf.addWindowFlags(ThesisGraphics::Configuration::WindowFlag::Resizable);
			//conf.setSize(Vector2i(1700, 900));
			ThesisGraphics app({ argc,argv }, conf);
			app.exec();

			/*char s[256];
			sprintf_s(s, "There is %i numbers", 12);
			OutputDebugStringA(s);*/
			break;
		}
		case IDM_REFRESH:
		{
			SelectComPort();
			break;
		}
		case IDM_CALIB:
		{
			_serial.setPort(ListItem);
			_serial.write('c');
			EnableWindow(hBtnStart, true);
			break;
		}
		case IDM_HOWTOCAL:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_HOWTOCALBOX), hWnd, HowToCalibrate);
			break;
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		//RedrawWindow(hWindow, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
		break;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT psWindow;
		HDC hdc = BeginPaint(hWindow, &psWindow);
		SetBkColor(hdc, 0x000082E6);//BGR
		SetTextColor(hdc, 0x00000000);
		SelectObject(hdc, hFontCim);
		HBRUSH hBrush = CreateSolidBrush(RGB(230, 130, 0));
		RECT r;
		GetClientRect(hWindow, &r);
		r.bottom = 80;
		FillRect(hdc, &r, hBrush);
		TCHAR textHeader[] = _T("Haptic Pantograph");
		TCHAR nullpoint[] = _T("Set null point:");
		TCHAR serialport[] = _T("Serial port:");
		TCHAR simulations[] = _T("Simulations:");
		TextOut(hdc, 20, 20, textHeader, _tcslen(textHeader));
		SelectObject(hdc, hFontSzoveg);
		SetBkColor(hdc, 0x00A0A0A0);
		TextOut(hdc, 10, 105, serialport, _tcslen(serialport));
		TextOut(hdc, 10, 165, nullpoint, _tcslen(nullpoint));
		TextOut(hdc, 10, 225, simulations, _tcslen(simulations));


		HDC hdcMem = CreateCompatibleDC(hdc);
		HGDIOBJ oldBitmap = SelectObject(hdcMem, hBitmap);

		/*StretchBlt(hdc,         // destination DC
			rect.right - 200,        // x upper left
			0,         // y upper left
			65,       // destination width
			80,      // destination height
			hdcMem,      // you just created this above
			0,
			0,                      // x and y upper left
			560,                      // source bitmap width
			690,                      // source bitmap height
			SRCCOPY);       // raster operation*/
		BitBlt(hdc, r.right - 65, 0, 65, 80, hdcMem, 0, 0, SRCCOPY);

		SelectObject(hdcMem, oldBitmap);
		DeleteDC(hdcMem);

		EndPaint(hWindow, &psWindow);
		break;
	}
	case WM_DESTROY: {
		ExitSimu = true;
		DeleteObject(hBitmap);
		PostQuitMessage(0);
		break;
	}	
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Message handler for about box.
INT_PTR CALLBACK HowToCalibrate(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

void SelectComPort() //added function to find the present serial 
{
	SendMessage(hComboBoxPort, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
	TCHAR lpTargetPath[5000]; // buffer to store the path of the COMPORTS
	DWORD test;
	gotPort = 0; // in case the port is not found
	wchar_t buffer[256];

	for (int i = 0; i < 255; i++) // checking ports from COM0 to COM255
	{
		wsprintfW(buffer, L"COM%d", i);
		test = QueryDosDevice(buffer, (LPWSTR)lpTargetPath, 5000);
		if (test != 0)
		{
			TCHAR A[16];
			memset(&A, 0, sizeof(A));
			wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)buffer);
			SendMessage(hComboBoxPort, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
			gotPort = 1; // found port
		}

		if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			lpTargetPath[10000]; // in case the buffer got filled, increase size of the buffer.
			continue;
		}

	}

	if (!gotPort) {
		wsprintfW(buffer, L"No Port");
		TCHAR A[16];
		memset(&A, 0, sizeof(A));
		wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)buffer);
		SendMessage(hComboBoxPort, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
		EnableWindow(hBtCalibrate, false);
	}
	else {
		EnableWindow(hBtCalibrate, true);
	}
	EnableWindow(hBtnStart, false);
	SendMessage(hComboBoxPort, CB_SETCURSEL, (WPARAM)0, (LPARAM)0); /*Wparam set init item*/
	PortIndex = 0;
	TCHAR item[256];
	(TCHAR)SendMessage(hComboBoxPort, (UINT)CB_GETLBTEXT, (WPARAM)PortIndex, (LPARAM)item);
	ListItem = new char[wcslen(item) + 1];
	size_t i;
	wcstombs_s(&i, ListItem, wcslen(item) + 1, item, wcslen(item) + 1);
}

void fillComboBoxSim() {
	SendMessage(hComboBoxSim, CB_RESETCONTENT, (WPARAM)0, (LPARAM)0);
	TCHAR Sims[5][15] =
	{
		TEXT("Billiard"), TEXT("Air Hockey"), TEXT("Spinner"), TEXT("Labyrinth"), TEXT("Coffee")
	};

	TCHAR A[16];
	int  k = 0;

	memset(&A, 0, sizeof(A));
	for (k = 0; k < 5; k += 1)
	{
		wcscpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)Sims[k]);
		// Add string to combobox.
		SendMessage(hComboBoxSim, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)A);
	}

	// Send the CB_SETCURSEL message to display an initial item in the selection field  
	SendMessage(hComboBoxSim, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
}

void enableWindow() {
	EnableWindow(hWindow,true);
	EnableCloseButton(hWindow, true);
}

void disableWindow() {
	EnableCloseButton(hWindow, false);
	EnableWindow(hWindow, false);
}

void EnableCloseButton(const HWND hwnd, const BOOL bState)
{
	HMENU hMenu = GetSystemMenu(hwnd, FALSE);
	UINT dwExtra = bState ? MF_ENABLED : (MF_DISABLED | MF_GRAYED);
	EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | dwExtra);
}
