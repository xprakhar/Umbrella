#pragma once

// Windows Native Headers
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// STL Headers
#include <stdexcept>

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}