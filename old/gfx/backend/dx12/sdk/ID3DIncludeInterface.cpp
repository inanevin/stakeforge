// Copyright (c) 2025 Inan Evin

#include "gfx/backend/dx12/sdk/ID3DIncludeInterface.hpp"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace SFG
{
	HRESULT __stdcall ID3DIncludeInterface::LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource)
	{

		return S_OK;
	}

	HRESULT __stdcall ID3DIncludeInterface::QueryInterface(REFIID riid, void** ppvObject)
	{
		return m_defaultIncludeHandler->QueryInterface(riid, ppvObject);
	}
	ULONG __stdcall ID3DIncludeInterface::AddRef(void)
	{
		return 0;
	}
	ULONG __stdcall ID3DIncludeInterface::Release(void)
	{
		return 0;
	}
}
