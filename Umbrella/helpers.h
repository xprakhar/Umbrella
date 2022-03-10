#pragma once

// Precompiled Headers
#include "stdafx.h"

// STL Headers
#include <stdexcept>

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::exception();
    }
}