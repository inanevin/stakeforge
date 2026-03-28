// Copyright (c) 2025 Inan Evin

#pragma once

#ifndef ID3DIncludeInterface_HPP
#define ID3DIncludeInterface_HPP

#include "d3dcommon.h"
#include <dxcapi.h>
#include <wrl/client.h>

namespace SFG
{
	class ID3DIncludeInterface : public IDxcIncludeHandler
	{
	public:
		ID3DIncludeInterface()
		{
		}
		virtual ~ID3DIncludeInterface()
		{
		}

		virtual HRESULT STDMETHODCALLTYPE LoadSource(_In_z_ LPCWSTR							  pFilename,	  // Candidate filename.
													 _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource // Resultant source object for included file, nullptr if not found.
													 ) override;

	protected:
		// Inherited via IDxcIncludeHandler
		virtual HRESULT __stdcall QueryInterface(REFIID riid, void** ppvObject) override;
		virtual ULONG __stdcall AddRef(void) override;
		virtual ULONG __stdcall Release(void) override;

	private:
		Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_defaultIncludeHandler;
	};

}

#endif
