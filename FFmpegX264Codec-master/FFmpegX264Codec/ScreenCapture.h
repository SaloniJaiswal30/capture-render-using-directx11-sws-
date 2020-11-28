#pragma once
#define WIN32_LEAN_AND_MEAN

#include <dxgi1_2.h>
#include <d3d11.h>
#include <memory>
#include <algorithm>
#include <string>
#include <iostream>
#include <functional>
#include "resource.h"
#include <time.h>
#include<DirectXMath.h>


#include "VertexShader.h"
#include "PixelShader.h"
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/avassert.h"
#include "libavutil/opt.h"
#include "libavutil/imgutils.h" 
#include <libswscale/swscale.h>
#include "libavformat/avio.h"
#include "libavutil/hwcontext.h"
#include "libavutil/hwcontext_qsv.h"
}
#pragma comment(lib, "D3D11.lib")

#pragma endregion

template <typename T>

class CComPtrCustom
{
public:

	CComPtrCustom(T *aPtrElement)
		:element(aPtrElement)
	{
	}

	CComPtrCustom()
		:element(nullptr)
	{
	}

	virtual ~CComPtrCustom()
	{
		Release();
	}

	T* Detach()
	{
		auto lOutPtr = element;

		element = nullptr;

		return lOutPtr;
	}

	T* detach()
	{
		return Detach();
	}

	void Release()
	{
		if (element == nullptr)
			return;

		auto k = element->Release();

		element = nullptr;
	}

	CComPtrCustom& operator = (T *pElement)
	{
		Release();

		if (pElement == nullptr)
			return *this;

		auto k = pElement->AddRef();

		element = pElement;

		return *this;
	}

	void Swap(CComPtrCustom& other)
	{
		T* pTemp = element;
		element = other.element;
		other.element = pTemp;
	}

	T* operator->()
	{
		return element;
	}

	operator T*()
	{
		return element;
	}

	operator T*() const
	{
		return element;
	}


	T* get()
	{
		return element;
	}

	T* get() const
	{
		return element;
	}

	T** operator &()
	{
		return &element;
	}

	bool operator !()const
	{
		return element == nullptr;
	}

	operator bool()const
	{
		return element != nullptr;
	}

	bool operator == (const T *pElement)const
	{
		return element == pElement;
	}

	CComPtrCustom(const CComPtrCustom& aCComPtrCustom)
	{
		if (aCComPtrCustom.operator!())
		{
			element = nullptr;

			return;
		}

		element = aCComPtrCustom;

		auto h = element->AddRef();

		h++;
	}

	CComPtrCustom& operator = (const CComPtrCustom& aCComPtrCustom)
	{
		Release();

		element = aCComPtrCustom;

		auto k = element->AddRef();

		return *this;
	}

	_Check_return_ HRESULT CopyTo(T** ppT) throw()
	{
		if (ppT == NULL)
			return E_POINTER;

		*ppT = element;

		if (element)
			element->AddRef();

		return S_OK;
	}

	HRESULT CoCreateInstance(const CLSID aCLSID)
	{
		T* lPtrTemp;

		auto lresult = ::CoCreateInstance(aCLSID, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&lPtrTemp));

		if (SUCCEEDED(lresult))
		{
			if (lPtrTemp != nullptr)
			{
				Release();

				element = lPtrTemp;
			}

		}

		return lresult;
	}

protected:

	T* element;
};


class ScreenCaptureProcessorGDI
{
	#pragma region Private & public 

private:
	CComPtrCustom<ID3D11Device> lDevice;
	CComPtrCustom<ID3D11DeviceContext> lImmediateContext;
	CComPtrCustom<IDXGIOutputDuplication> lDeskDupl;
	CComPtrCustom<ID3D11Texture2D> lGDIImage;
	CComPtrCustom<ID3D11Texture2D> lDestImage;
	CComPtrCustom<ID3D11Texture2D> m_DestImage;
	CComPtrCustom<ID3D11VertexShader>VertexShader;
	CComPtrCustom<ID3D11PixelShader> PixelShader;
	CComPtrCustom<ID3D11InputLayout> InputLayout;
	CComPtrCustom<ID3D11SamplerState> SamplerLinear;
	CComPtrCustom<IDXGISwapChain1> swapchain;

	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
	D3D11_VIEWPORT VP;
	ID3D11Texture2D* BackBuffer = nullptr;
	D3D11_TEXTURE2D_DESC FrameDesc;
	ID3D11Texture2D* m_SharedSurf = nullptr;
	ID3D11VertexShader* m_VertexShader;
	ID3D11PixelShader* m_PixelShader;
	ID3D11InputLayout* m_InputLayout;
	ID3D11SamplerState* m_SamplerLinear;
	ID3D11BlendState* m_BlendState;
	ID3D11RenderTargetView* backbuffer;
	IDXGIFactory2* m_Factory;
	ID3D11RenderTargetView* m_RTV;
	
	RECT m_rtViewport;
	DXGI_OUTPUT_DESC lOutputDesc;
	DXGI_OUTDUPL_DESC lOutputDuplDesc;	
	unsigned int frame_count;
	unsigned int max_count_frames;
	int lresult;
	D3D_FEATURE_LEVEL lFeatureLevel;
	HRESULT hr;
	bool error;	
	long CaptureSize;
	typedef struct _VERTEX
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT2 TexCoord;
	} VERTEX;

public:
	bool Render(AVFrame* frame);
	ScreenCaptureProcessorGDI();	
	void get_hwnd(HWND hwnd1);
	std::wstring lMyDocPath;	
	bool init();
	bool GrabImage(UCHAR* &CaptureBuffer, long &CaptureSize);	
	bool release();
	void setMaxFrames(unsigned int maxFrames);
	bool hasFailed();
	void save_as_bitmap(unsigned char *bitmap_data, int rowPitch, int width, int height, char *filename);
	
	
#pragma endregion
};

class RPCScreenCapture
{
public:
	RPCScreenCapture(void);
	~RPCScreenCapture(void);
};

