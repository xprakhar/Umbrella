#pragma once

// Windows Native Headers
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <wrl.h>

// DirectX12 Headers
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

using namespace Microsoft::WRL;

