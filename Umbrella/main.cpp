
#include "stdafx.h"
#include "resource.h"
#include "helpers.h"

#include <chrono>
#include <algorithm>
#include <cassert>

#define MAX_LOAD_STRING 256

/* ------ Main Window --------*/
HWND g_Hwnd;
int g_Width;
int g_Height;
/* ------------------------------- */

/* ------ DXGI\D3D12 Objects --------*/
constexpr int nBackBuffers = 3;
ComPtr<IDXGIAdapter4> g_Adapter;
ComPtr<ID3D12Device> g_Device;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<IDXGISwapChain4> g_SwapChain;
ComPtr<ID3D12Resource> g_BackBuffers[nBackBuffers];
ComPtr<ID3D12GraphicsCommandList> g_CommandList;
ComPtr<ID3D12CommandAllocator> g_CommandAllocators[nBackBuffers];
ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
UINT g_RTVDescriptorSize;
UINT g_CurrentBackBufferIndex;
/* ------------------------------- */

/* ------ CPU\GPU Syncronization --------*/
ComPtr<ID3D12Fence> g_Fence;
uint64_t g_FenceValue = 0;
uint64_t g_FrameFenceValues[nBackBuffers] = {};
HANDLE g_FenceEvent;
/* ------------------------------- */

/* ------ Misc --------*/
bool g_UseWarp = false;
bool g_VSync = true;
bool g_TearingSupported = false;
bool g_Fullscreen = false;
/* ------------------------------- */

/* ------ String Table --------*/
WCHAR wszEngClass[MAX_LOAD_STRING];
WCHAR wszTitle[MAX_LOAD_STRING];
/* ------------------------------- */

/* ------ Forward Decleration --------*/
LRESULT CALLBACK MsgHandler(HWND, UINT, WPARAM, LPARAM);
void ParseCommandLineArguments();
void CreateMainWindow(HINSTANCE);
void EnableDebugLayer();
ComPtr<IDXGIAdapter4> GetAdapter(bool);
ComPtr<ID3D12Device2> GetDevice(ComPtr<IDXGIAdapter4> adapter);
/* ------------------------------- */

int WINAPI wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ PWSTR pCmdLine,
	_In_ int nCmdShow) 
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	ParseCommandLineArguments();
	CreateMainWindow(hInstance);
#if defined(_DEBUG)
	EnableDebugLayer();
#endif
	g_Adapter = GetAdapter(g_UseWarp);
	g_Device = GetDevice(g_Adapter);

/* ------ Game Loop --------*/
	MSG msg = {};
	while (msg.message != WM_QUIT) {
		if (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) 
		{
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}
		else 
		{

		}
	}
/* ------------------------------- */
	UnregisterClassW(wszEngClass, hInstance);
	ExitProcess(0);
}

void ParseCommandLineArguments() 
{
	int argc;
	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

	for (size_t i = 0; i < argc; ++i) {
		if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
		{
			g_Width = wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
		{
			g_Height = wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0) 
		{
			g_UseWarp = true;
		}
	}

	::LocalFree(argv);
}

void CreateMainWindow(HINSTANCE hInstance) 
{
	::LoadStringW(hInstance, IDS_ENGCLS, wszEngClass, MAX_LOAD_STRING);
	::LoadStringW(hInstance, IDS_TITLE, wszTitle, MAX_LOAD_STRING);

	WNDCLASSEXW wcex;
	wcex.cbSize = sizeof(WNDCLASSEXW);
	wcex.lpszClassName = wszEngClass;
	wcex.lpfnWndProc = MsgHandler;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wcex.hCursor = LoadCursorW(hInstance, IDC_ARROW);
	wcex.hIcon = LoadIconW(hInstance, IDI_APPLICATION);
	wcex.hIconSm = LoadIconW(hInstance, IDI_APPLICATION);
	wcex.lpszMenuName = MAKEINTRESOURCEW(IDR_ENGMENU);
	wcex.style = CS_HREDRAW | CS_VREDRAW;

	ATOM atom = ::RegisterClassExW(&wcex);
	assert(atom > 0);

	RECT windowRect = { 0, 0, g_Width, g_Height };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, TRUE);
	g_Width = windowRect.right - windowRect.left;
	g_Height = windowRect.bottom - windowRect.top;

	g_Hwnd = ::CreateWindowExW(0, wszEngClass, wszTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, g_Width, g_Height, nullptr, nullptr, hInstance, nullptr);
	assert(g_Hwnd && "Failed to create window");

	::ShowWindow(g_Hwnd, SW_SHOW);
	::UpdateWindow(g_Hwnd);
}

void EnableDebugLayer()
{
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
}

ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
	
	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;
	if (useWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
					D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}

ComPtr<ID3D12Device2> GetDevice(ComPtr<IDXGIAdapter4> adapter)
{
	ComPtr<ID3D12Device2> d3d12Device2;
	D3D12CreateDevice(g_Adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&d3d12Device2));

#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif
	return d3d12Device2;
}

LRESULT CALLBACK MsgHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) 
{
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