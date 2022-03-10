
#include "stdafx.h"
#include "resource.h"


#define MAX_LOAD_STRING 256

const INT WIDTH = 640;
const INT HEIGHT = 480;

HWND hWnd;
WCHAR wszClassName[MAX_LOAD_STRING];
WCHAR wszTitle[MAX_LOAD_STRING];

LRESULT CALLBACK msgHandler(HWND, UINT, WPARAM, LPARAM);

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR pCmdLine,
	_In_ int nCmdShow) 
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	LoadStringW(hInstance, IDS_ENGCLS, wszClassName, MAX_LOAD_STRING);
	LoadStringW(hInstance, IDS_TITLE, wszTitle, MAX_LOAD_STRING);

	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.lpszClassName = wszClassName;
	wcex.lpfnWndProc = msgHandler;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wcex.hCursor = LoadCursorW(hInstance, IDC_ARROW);
	wcex.hIcon = LoadIconW(hInstance, IDI_APPLICATION);
	wcex.hIconSm = LoadIconW(hInstance, IDI_APPLICATION);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDR_ENGMENU);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	
	RegisterClassExW(&wcex);


	hWnd = CreateWindowExW(0, wszClassName, wszTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, nullptr, nullptr, hInstance, nullptr);
	if (!hWnd) ExitProcess(0);

	ShowWindow(hWnd, SW_SHOW);

	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
		else {

		}
	}

	ExitProcess(0);
}

LRESULT CALLBACK msgHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {

	case WM_COMMAND:

		switch (LOWORD(wParam))
		{
		case ID_FILE_EXIT:
			PostQuitMessage(0);
			break;
		default:
			break;
		}
		return 0;

	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}

	return NULL;
}