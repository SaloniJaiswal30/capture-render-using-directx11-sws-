
#include "ScreenCapture.h"
#include "time.h"
#include <windows.h>
HWND m_WindowHandle;
#define NUMVERTICES 6
D3D_DRIVER_TYPE gDriverTypes[] =
{
	D3D_DRIVER_TYPE_HARDWARE
};
UINT gNumDriverTypes = ARRAYSIZE(gDriverTypes);
int count = 1;
// Feature levels supported
D3D_FEATURE_LEVEL gFeatureLevels[] =
{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_1
};

UINT gNumFeatureLevels = ARRAYSIZE(gFeatureLevels);

ScreenCaptureProcessorGDI::ScreenCaptureProcessorGDI() 
	{
		
	}


RPCScreenCapture::RPCScreenCapture(void)
{
}

RPCScreenCapture::~RPCScreenCapture(void)
{
}
void ScreenCaptureProcessorGDI::get_hwnd(HWND hwnd1) {
	m_WindowHandle = hwnd1;
}
bool ScreenCaptureProcessorGDI::init()
	{
		// Create device			
		for (UINT DriverTypeIndex = 0; DriverTypeIndex < gNumDriverTypes; ++DriverTypeIndex)
		{
			hr = D3D11CreateDevice(
				nullptr,
				gDriverTypes[DriverTypeIndex],
				nullptr,
				0,
				gFeatureLevels,
				gNumFeatureLevels,
				D3D11_SDK_VERSION,
				&lDevice,
				&lFeatureLevel,
				&lImmediateContext);
			if (SUCCEEDED(hr))
			{
				// Device creation success, no need to loop anymore
				break;
			}

			lDevice.Release();
			lImmediateContext.Release();
		}

		if (FAILED(hr))
		{
			return false;
		}
		if (lDevice == nullptr)
		{
			return false;
		}

		// Get DXGI device
		CComPtrCustom<IDXGIDevice> lDxgiDevice;
		hr = lDevice->QueryInterface(IID_PPV_ARGS(&lDxgiDevice));
		if (FAILED(hr))
		{
			return false;
		}

		// Get DXGI adapter
		CComPtrCustom<IDXGIAdapter> lDxgiAdapter;
		hr = lDxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&lDxgiAdapter));
		if (FAILED(hr))
		{
			return false;
		}
		lDxgiDevice.Release();
		UINT Output = 0;
		//factory
		hr = lDxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&m_Factory));
		if (FAILED(hr))
		{
			return -1;
		}
		// Get output
		CComPtrCustom<IDXGIOutput> lDxgiOutput;
		hr = lDxgiAdapter->EnumOutputs(Output,&lDxgiOutput);
		if (FAILED(hr))
		{
			return false;
		}
		lDxgiAdapter.Release();

		hr = lDxgiOutput->GetDesc(&lOutputDesc);
		if (FAILED(hr))
		{
			return false;
		}

		// QI for Output 1
		CComPtrCustom<IDXGIOutput1> lDxgiOutput1;
		hr = lDxgiOutput->QueryInterface(IID_PPV_ARGS(&lDxgiOutput1));
		if (FAILED(hr))
		{
			return false;
		}
		lDxgiOutput.Release();

		// Create desktop duplication
		hr = lDxgiOutput1->DuplicateOutput(lDevice,&lDeskDupl);
		if (FAILED(hr))
		{
			return false;
		}
		lDxgiOutput1.Release();

		// Create GUI drawing texture
		lDeskDupl->GetDesc(&lOutputDuplDesc);
		D3D11_TEXTURE2D_DESC desc;
		desc.Width = lOutputDuplDesc.ModeDesc.Width;
		desc.Height = lOutputDuplDesc.ModeDesc.Height;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.ArraySize = 1;
		desc.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET;
		desc.MiscFlags = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.MipLevels = 1;
		desc.CPUAccessFlags = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		hr = lDevice->CreateTexture2D(&desc, NULL, &lGDIImage);

		if (FAILED(hr))
		{
			return false;
		}

		if (lGDIImage == nullptr)
		{
			return false;
		}

		// Create CPU access texture

		desc.Width = lOutputDuplDesc.ModeDesc.Width;
		desc.Height = lOutputDuplDesc.ModeDesc.Height;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.ArraySize = 1;
		desc.BindFlags = 0;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.MipLevels = 1;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_STAGING;
		hr = lDevice->CreateTexture2D(&desc, NULL, &lDestImage);
		if (FAILED(hr))
		{
			return false;
		}
		if (lDestImage == nullptr)
		{
			return false;
		}

		desc.Width = lOutputDuplDesc.ModeDesc.Width;
		desc.Height = lOutputDuplDesc.ModeDesc.Height;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.ArraySize = 1;
		desc.BindFlags = 0;
		desc.MiscFlags = 0;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.MipLevels = 1;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		desc.Usage = D3D11_USAGE_STAGING;
		hr = lDevice->CreateTexture2D(&desc, NULL, &m_DestImage);
		if (FAILED(hr))
		{
			return false;
		}
		if (m_DestImage == nullptr)
		{
			return false;
		}
		// Create swapchain for window

		RtlZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));

		SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		SwapChainDesc.BufferCount = 2;
		SwapChainDesc.Width = lOutputDuplDesc.ModeDesc.Width;
		SwapChainDesc.Height = lOutputDuplDesc.ModeDesc.Height;
		SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		SwapChainDesc.SampleDesc.Count = 1;
		SwapChainDesc.SampleDesc.Quality = 0;
		hr = m_Factory->CreateSwapChainForHwnd(lDevice, m_WindowHandle, &SwapChainDesc, nullptr, nullptr, &swapchain);
		hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&BackBuffer));

		if (FAILED(hr))
		{
			return -1;
		}
		// Create a render target view
		hr = lDevice->CreateRenderTargetView(BackBuffer, nullptr, &m_RTV);
		BackBuffer->Release();
		if (FAILED(hr))
		{
			return -1;
		}


		//sampler(not imp for displaying)
		D3D11_SAMPLER_DESC SampDesc;
		RtlZeroMemory(&SampDesc, sizeof(SampDesc));
		SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		SampDesc.MinLOD = 0;
		SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = lDevice->CreateSamplerState(&SampDesc, &m_SamplerLinear);

		//Create the blend state(not imp for displaying)
		D3D11_BLEND_DESC BlendStateDesc;
		BlendStateDesc.AlphaToCoverageEnable = FALSE;
		BlendStateDesc.IndependentBlendEnable = FALSE;
		BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
		BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		hr = lDevice->CreateBlendState(&BlendStateDesc, &m_BlendState);

		//2d texture for sharedsurf

		D3D11_TEXTURE2D_DESC DeskTexD;
		RtlZeroMemory(&DeskTexD, sizeof(D3D11_TEXTURE2D_DESC));
		DeskTexD.Width = lOutputDuplDesc.ModeDesc.Width;
		DeskTexD.Height = lOutputDuplDesc.ModeDesc.Height;
		DeskTexD.MipLevels = 1;
		DeskTexD.ArraySize = 1;
		DeskTexD.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		DeskTexD.SampleDesc.Count = 1;
		DeskTexD.Usage = D3D11_USAGE_DEFAULT;
		DeskTexD.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		DeskTexD.CPUAccessFlags = 0;
		//DeskTexD.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

		hr = lDevice->CreateTexture2D(&DeskTexD, nullptr, &m_SharedSurf);

		//Create 2D texture here... Same as shared surface
		//===============================
		UINT Size = ARRAYSIZE(g_VS);
		hr = lDevice->CreateVertexShader(g_VS, Size, nullptr, &m_VertexShader);
		if (FAILED(hr))
		{
			return -1;
		}

		D3D11_INPUT_ELEMENT_DESC Layout[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};
		UINT NumElements = ARRAYSIZE(Layout);
		hr = lDevice->CreateInputLayout(Layout, NumElements, g_VS, Size, &m_InputLayout);
		if (FAILED(hr))
		{
			return -1;
		}
		lImmediateContext->IASetInputLayout(m_InputLayout);

		Size = ARRAYSIZE(g_PS);
		hr = lDevice->CreatePixelShader(g_PS, Size, nullptr, &m_PixelShader);
		if (FAILED(hr))
		{
			return -1;
		}
		//===================================================

		//viewport description
		VP.Width = static_cast<FLOAT>(lOutputDuplDesc.ModeDesc.Width);
		VP.Height = static_cast<FLOAT>(lOutputDuplDesc.ModeDesc.Height);
		VP.MinDepth = 0.0f;
		VP.MaxDepth = 1.0f;
		VP.TopLeftX = 0;
		VP.TopLeftY = 0;
		return true;
	}
	bool ScreenCaptureProcessorGDI::Render(AVFrame* frame)
	{
		D3D11_MAPPED_SUBRESOURCE resource;
		UINT subresource = D3D11CalcSubresource(0, 0, 0);
		lImmediateContext->Map(m_DestImage, subresource, D3D11_MAP_WRITE, 0, &resource);
		BYTE* sptr = reinterpret_cast<BYTE*>(resource.pData);
		int stride = (resource.RowPitch < (frame->width * 4)) ? resource.RowPitch : (frame->width * 4);
		unsigned long i = 0;
		int pixel_h = frame->height;
		int pixel_w = frame->linesize[1];
		/*BYTE* dptr = reinterpret_cast<BYTE*>(resource.pData);

		int dist = 0, dd = frame->pitch / 2;
		uint8_t u = 0, v = 0;
		for (int h = 0; h < frame->height; h += 2)
		{
			for (int w = 0; w < frame->width; w += 2)
			{
				u = frame->U[frame->pitch / 2 * h / 2 + w / 2];
				v = frame->V[frame->pitch / 2 * h / 2 + w / 2];

				for (int i = 0; i < 2; i++)
				{
					for (int j = 0; j < 2; j++)
					{
						dist = resource.RowPitch * (frame->height - 1 - (h + i));
						dptr[dist + (w + j) * 4 + 2] = frame->Y[frame->pitch * (h + i) + (w + j)];
						dptr[dist + (w + j) * 4 + 1] = u;
						dptr[dist + (w + j) * 4] = v;
					}
				}
			}
		}*/
		memcpy_s(sptr, resource.RowPitch * pixel_h, frame->data[0], stride * pixel_h);
		lImmediateContext->Unmap(m_DestImage, subresource);
		//lImmediateContext->CopyResource(m_DestImage, m_DestImage);//Copy the entire contents of the source resource to the destination resource using the GPU
		//D3D11_MAPPED_SUBRESOURCE resource1;
		//BYTE* ImageData=NULL;
		//UINT subresource1 = D3D11CalcSubresource(0, 0, 0);//Calculates a subresource index for a texture.
		//lImmediateContext->Map(m_DestImage, subresource1, D3D11_MAP_READ, 0, &resource1);//Gets a pointer to the data contained in a subresource, and denies the GPU access to that subresource.
		//BYTE* sptr1 = reinterpret_cast<BYTE*>(resource.pData);//sptr pointing to resource.pdata
		////Store Image Pitch
		//int m_ImagePitch = resource1.RowPitch;
		//int height = frame->height;
		//memcpy_s(ImageData, resource1.RowPitch * height, sptr1, resource1.RowPitch * height);//copy
		//lImmediateContext->Unmap(m_DestImage, subresource1);
		//

		//FILE* f;
		//char file_name[MAX_PATH];

		//sprintf_s(file_name, "%d.bmp", count);
		//char* filename = file_name;
		//save_as_bitmap(ImageData, CaptureSize, lOutputDuplDesc.ModeDesc.Width, lOutputDuplDesc.ModeDesc.Height, file_name);
		
		VERTEX Vertices[NUMVERTICES] =
		{
			{DirectX::XMFLOAT3(-1.0f, -1.0f, 0), DirectX::XMFLOAT2(0.0f, 1.0f)},
			{DirectX::XMFLOAT3(-1.0f, 1.0f, 0), DirectX::XMFLOAT2(0.0f, 0.0f)},
			{DirectX::XMFLOAT3(1.0f, -1.0f, 0), DirectX::XMFLOAT2(1.0f, 1.0f)},
			{DirectX::XMFLOAT3(1.0f, -1.0f, 0), DirectX::XMFLOAT2(1.0f, 1.0f)},
			{DirectX::XMFLOAT3(-1.0f, 1.0f, 0), DirectX::XMFLOAT2(0.0f, 0.0f)},
			{DirectX::XMFLOAT3(1.0f, 1.0f, 0), DirectX::XMFLOAT2(1.0f, 0.0f)},
		};
		//sharesurf decription
		m_SharedSurf->GetDesc(&FrameDesc);

		D3D11_SHADER_RESOURCE_VIEW_DESC ShaderDesc;
		ShaderDesc.Format = FrameDesc.Format;
		ShaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		ShaderDesc.Texture2D.MostDetailedMip = FrameDesc.MipLevels - 1;
		ShaderDesc.Texture2D.MipLevels = FrameDesc.MipLevels;


		// Create new shader resource view
		HRESULT hr;
		ID3D11ShaderResourceView* ShaderResource = nullptr;
		hr = lDevice->CreateShaderResourceView(m_SharedSurf, &ShaderDesc, &ShaderResource);

		if (FAILED(hr))
		{
			return -1;
		}
		UINT Stride = sizeof(VERTEX);
		UINT Offset = 0;
		FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
		lImmediateContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
		lImmediateContext->OMSetRenderTargets(1, &m_RTV, nullptr);
		lImmediateContext->VSSetShader(m_VertexShader, nullptr, 0);
		lImmediateContext->PSSetShader(m_PixelShader, nullptr, 0);
		lImmediateContext->PSSetSamplers(0, 1, &m_SamplerLinear);
		lImmediateContext->PSSetShaderResources(0, 1, &ShaderResource);
		lImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		

		D3D11_BUFFER_DESC BufferDesc;
		RtlZeroMemory(&BufferDesc, sizeof(BufferDesc));
		BufferDesc.Usage = D3D11_USAGE_DEFAULT;
		BufferDesc.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA InitData;
		RtlZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = Vertices;

		ID3D11Buffer* VertexBuffer = nullptr;

		// Create vertex buffer
		hr = lDevice->CreateBuffer(&BufferDesc, &InitData, &VertexBuffer);
		lImmediateContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
		lImmediateContext->Draw(NUMVERTICES, 0);


		VertexBuffer->Release();
		VertexBuffer = nullptr;

		// Release shader resource
		ShaderResource->Release();
		ShaderResource = nullptr;

		
		lImmediateContext->RSSetViewports(1, &VP);
		swapchain->Present(1, 0);

		/*D3D11_MAPPED_SUBRESOURCE resource;
		UINT subresource = D3D11CalcSubresource(0, 0, 0);

		hr = lImmediateContext->Map(m_SharedSurf, subresource, D3D11_MAP_WRITE, 0, &resource);

		if (FAILED(hr))
		{
			return false;
		}

		BYTE* sptr1 = reinterpret_cast<BYTE*>(resource.pData);
		BYTE* dptr1 = frame->data[0];

		RECT captureRECT;

		hr = S_OK;

		for (int i = 0; i < frame->height; i++)
		{
			if (memcpy_s(sptr1, frame->linesize[0], dptr1, frame->width))
			{
				//fprintf_s(m_log_file, "There is a problem transferring the luminance data to YUV texture.\n");
			}
			dptr1 += frame->linesize[0];
			sptr1 += resource.RowPitch;
		}
		frame->width /= 2;
		//pFrame->linesize[1] = pFrame->linesize[2] = resource.RowPitch / 2;
		//dptr1 = frame->data[1];
		//sptr += resource.RowPitch * height;
		BYTE* dptr2 = frame->data[2];

		for (int i = 0; i < frame->height / 2; i++)
		{
			if (memcpy_s(sptr1, frame->linesize[1], dptr2, frame->width))
			{
				//fprintf_s(m_log_file, "There is a problem transferring the chrominance U data to YUV texture.\n");
			}
			//sptr1 += resource.RowPitch;
			dptr2 += frame->linesize[1];
			if (memcpy_s(sptr1, frame->linesize[2], dptr2 + frame->width, frame->width))
			{
				//fprintf_s(m_log_file, "There is a problem transferring the chrominance V data to YUV texture.\n");
			}
			sptr1 += resource.RowPitch;
			dptr2 += frame->linesize[2];
		}

		lImmediateContext->Unmap(m_SharedSurf, subresource);*/

		lImmediateContext->CopyResource(m_SharedSurf, m_DestImage);
		return true;
	}

	bool ScreenCaptureProcessorGDI::GrabImage(UCHAR* &CaptureBuffer, long &CaptureSize)
	{
		char file_name[1024];
		int lTryCount = 4;
		CComPtrCustom<IDXGIResource> lDesktopResource;
		CComPtrCustom<ID3D11Texture2D> lAcquiredDesktopImage;
		DXGI_OUTDUPL_FRAME_INFO lFrameInfo;

		do
		{
			hr = lDeskDupl->AcquireNextFrame(INFINITE, &lFrameInfo, &lDesktopResource);

			if (SUCCEEDED(hr))
				break;

			if (hr == DXGI_ERROR_WAIT_TIMEOUT)
			{
				continue;
			}
			else if (FAILED(hr))
			{
				break;
			}

		} while (--lTryCount > 0);

		if (FAILED(hr))
		{
			error = true;
			return false;
		}

		hr = lDesktopResource->QueryInterface(IID_PPV_ARGS(&lAcquiredDesktopImage));

		lDesktopResource.Release();

		if (FAILED(hr))
		{
			error = true;
			return false;
		}

		if (lAcquiredDesktopImage == nullptr)
		{
			error = true;
			return false;
		}

		// Copy image into GDI drawing texture
		lImmediateContext->CopyResource(lGDIImage, lAcquiredDesktopImage);

		lAcquiredDesktopImage.Release();

		lDeskDupl->ReleaseFrame();

		// Copy image into CPU access texture
		lImmediateContext->CopyResource(lDestImage, lGDIImage);

		// Copy from CPU access texture to bitmap buffer
		D3D11_MAPPED_SUBRESOURCE resource;
		UINT subresource = D3D11CalcSubresource(0, 0, 0);
		lImmediateContext->Map(lDestImage, subresource, D3D11_MAP_READ_WRITE, 0, &resource);

		BITMAPINFO	lBmpInfo;

		// BMP 32 bpp
		ZeroMemory(&lBmpInfo, sizeof(BITMAPINFO));

		lBmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

		lBmpInfo.bmiHeader.biBitCount = 32;

		lBmpInfo.bmiHeader.biCompression = BI_RGB;

		lBmpInfo.bmiHeader.biWidth = lOutputDuplDesc.ModeDesc.Width;

		lBmpInfo.bmiHeader.biHeight = lOutputDuplDesc.ModeDesc.Height;

		lBmpInfo.bmiHeader.biPlanes = 1;

		lBmpInfo.bmiHeader.biSizeImage = lOutputDuplDesc.ModeDesc.Width
			* lOutputDuplDesc.ModeDesc.Height * 4;

		std::unique_ptr<BYTE> pBuf(new BYTE[lBmpInfo.bmiHeader.biSizeImage]);

		UINT lBmpRowPitch = lOutputDuplDesc.ModeDesc.Width * 4;

		BYTE* sptr = reinterpret_cast<BYTE*>(resource.pData);
		BYTE* dptr = CaptureBuffer;//pBuf.get() /*+ lBmpInfo.bmiHeader.biSizeImage - lBmpRowPitch*/;

		UINT lRowPitch = std::min<UINT>(lBmpRowPitch, resource.RowPitch);

		
		clock_t start = clock();
		for (size_t h = 0; h < lOutputDuplDesc.ModeDesc.Height; ++h)
		{
			memcpy_s(dptr, lBmpRowPitch, sptr, lRowPitch);
			sptr += resource.RowPitch;
			dptr += lBmpRowPitch;
		}
		clock_t stop = clock();

		clock_t nDiff = stop - start;

		//fprintf(log_file, "average time for scale - %ld\n", nDiff);


		lImmediateContext->Unmap(lDestImage, subresource);

		CaptureSize = lBmpRowPitch;
		/*_snprintf_s(file_name, sizeof(file_name), "Content\\Captured_Image\\%d.bmp", frame_count);
		save_as_bitmap(CaptureBuffer, CaptureSize, lOutputDuplDesc.ModeDesc.Width, lOutputDuplDesc.ModeDesc.Height, file_name);
		*/
		frame_count++;

		return true;
	}

#pragma region Save Image 
	void ScreenCaptureProcessorGDI::save_as_bitmap(unsigned char *bitmap_data, int rowPitch, int width, int height, char *filename)
	{
		// A file is created, this is where we will save the screen capture.

		FILE *f;

		BITMAPFILEHEADER   bmfHeader;
		BITMAPINFOHEADER   bi;

		bi.biSize = sizeof(BITMAPINFOHEADER);
		bi.biWidth = width;
		//Make the size negative if the image is upside down.
		bi.biHeight = -height;
		//There is only one plane in RGB color space where as 3 planes in YUV.
		bi.biPlanes = 1;
		//In windows RGB, 8 bit - depth for each of R, G, B and alpha.
		bi.biBitCount = 32;
		//We are not compressing the image.
		bi.biCompression = BI_RGB;
		// The size, in bytes, of the image. This may be set to zero for BI_RGB bitmaps.
		bi.biSizeImage = 0;
		bi.biXPelsPerMeter = 0;
		bi.biYPelsPerMeter = 0;
		bi.biClrUsed = 0;
		bi.biClrImportant = 0;

		// rowPitch = the size of the row in bytes.
		DWORD dwSizeofImage = rowPitch * height;

		// Add the size of the headers to the size of the bitmap to get the total file size
		DWORD dwSizeofDIB = dwSizeofImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

		//Offset to where the actual bitmap bits start.
		bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

		//Size of the file
		bmfHeader.bfSize = dwSizeofDIB;

		//bfType must always be BM for Bitmaps
		bmfHeader.bfType = 0x4D42; //BM   

								   // TODO: Handle getting current directory
		fopen_s(&f, filename, "wb");

		DWORD dwBytesWritten = 0;
		dwBytesWritten += fwrite(&bmfHeader, sizeof(BITMAPFILEHEADER), 1, f);
		dwBytesWritten += fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, f);
		dwBytesWritten += fwrite(bitmap_data, 1, dwSizeofImage, f);

		fclose(f);
	}
	
#pragma endregion
	bool ScreenCaptureProcessorGDI::release()
	{
		return true;
	}

	void ScreenCaptureProcessorGDI::setMaxFrames(unsigned int maxFrames)
	{
		max_count_frames = maxFrames;
	}

	bool ScreenCaptureProcessorGDI::hasFailed()
	{
		return error == true;
	}





	
