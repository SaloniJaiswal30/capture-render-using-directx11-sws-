// FFmpegX264Codec.cpp : Defines the entry point for the application.
//


#include "FFmpegX264Codec.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>

//#include "ScreenCapture.cpp"
#include <d3d9.h>
#define NUMVERTICES 6

#define MAX_LOADSTRING 100

CRITICAL_SECTION  m_critial;

IDirect3D9 *m_pDirect3D9 = NULL;
IDirect3DDevice9 *m_pDirect3DDevice = NULL;
IDirect3DSurface9 *m_pDirect3DSurfaceRender = NULL;

int m_ImagePitch;
RECT m_rtViewport;
RECT desktop;

AVFrame* m_resChangeFrame;
struct SwsContext* m_resChangeCntxt;

#define INBUF_SIZE 20480

#define MAX_LOADSTRING 100

UCHAR* CaptureBuffer = NULL;

// Global Variables:
HINSTANCE hInst;                        // current instance
TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];    // the main window class name

// Forward declarations of functions included in this code module:

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


bool captured = false;
AVPacket *pkt;
ScreenCaptureProcessorGDI* D3D11 = new ScreenCaptureProcessorGDI();

void Cleanup()
{
	EnterCriticalSection(&m_critial);
	if (m_pDirect3DSurfaceRender)
		m_pDirect3DSurfaceRender->Release();
	if (m_pDirect3DDevice)
		m_pDirect3DDevice->Release();
	if (m_pDirect3D9)
		m_pDirect3D9->Release();
	LeaveCriticalSection(&m_critial);
}

void AVCodecCleanup()
{
	free(is_keyframe);

	fclose(log_file);
	free(CaptureBuffer);

	avcodec_close(encodingCodecContext);
	avcodec_close(decodingCodecContext);
	avcodec_free_context(&decodingCodecContext);
	av_frame_free(&iframe);
	av_frame_free(&oframe);
	avcodec_free_context(&encodingCodecContext);
	av_frame_free(&inframe);
	av_frame_free(&outframe);
	av_packet_free(&pkt);

	Cleanup();

	exit(0);
}

int InitD3D(HWND hwnd, unsigned long lWidth, unsigned long lHeight)
{
	HRESULT lRet;
	InitializeCriticalSection(&m_critial);
	Cleanup();
	
	
	//return DUPL_RETURN_SUCCESS;
	m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (m_pDirect3D9 == NULL)
		return -1;

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	GetClientRect(hwnd, &m_rtViewport);

	lRet = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &m_pDirect3DDevice);
	if (FAILED(lRet))
		return -1;

	lRet = m_pDirect3DDevice->CreateOffscreenPlainSurface(
		lWidth, lHeight,
		(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'), //FourCC for YUV420P Pixal Color Format
		D3DPOOL_DEFAULT,
		&m_pDirect3DSurfaceRender,
		NULL);
	//LPRECT lpRect
	//RECT rect;// = NULL;
	//int width;
	//int height;
	//if (GetWindowRect(hwnd, &rect))
	//{
	//	 width = rect.right - rect.left;
	//	 height = rect.bottom - rect.top;
	//}
	int height = 768;
	int width = 1366;
	if (width % 16 != 0) {
		int rem = width % 16;
		width = width + (16 - rem);
	}

	m_resChangeFrame = av_frame_alloc();
	if (!m_resChangeFrame)
		return false;//RPCLogger::Log(0, "Sudheer: Could not allocate video frame for Recording");
	m_resChangeFrame->format = AV_PIX_FMT_RGB32;
	m_resChangeFrame->width = width;
	m_resChangeFrame->height = height;
	int ret = av_frame_get_buffer(m_resChangeFrame, 8);
	if (ret < 0)
		return -1;//RPCLogger::Log(0, "Could not allocate the video frame data");
	
	m_resChangeCntxt = NULL;
	m_resChangeCntxt = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGB32, SWS_BILINEAR, NULL, NULL, NULL);
	if (!m_resChangeCntxt)
		return -1;//RPCLogger::Log(0, "Sudheer: sws context fro recording failed");

	if (FAILED(lRet))
		return -1;

	return 0;
}



/*bool Render(AVFrame* frame)
{
	D3D11_MAPPED_SUBRESOURCE resource;
	UINT subresource = D3D11CalcSubresource(0, 0, 0);
	lImmediateContext->Map(m_DestImage, subresource, D3D11_MAP_WRITE, 0, &resource);
	BYTE* sptr = reinterpret_cast<BYTE*>(resource.pData);
	//BYTE* d;

	/*D3DLOCKED_RECT d3d_rect;
	lRet =m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return -1;
		
	byte* pDest = (BYTE*)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;*/

	/*int stride = (resource.RowPitch < (frame->width * 4)) ? resource.RowPitch : (frame->width * 4);
	unsigned long i = 0;

	int pixel_h = frame->height, pixel_w = frame->width;
	memcpy_s(sptr, resource.RowPitch * pixel_h, frame->data[0], stride * pixel_h);
	//Copy Data
	/*if (frame->format == AV_PIX_FMT_YUV420P)
	{
		

		for (i = 0; i < pixel_h; i++) {
			memcpy(sptr + i * stride, frame->data[0] + i * pixel_w, pixel_w);
		}
		for (i = 0; i < pixel_h / 2; i++) {
			memcpy(sptr + stride * pixel_h + i * stride / 2, frame->data[2] + i * pixel_w / 2, pixel_w / 2);
		}
		for (i = 0; i < pixel_h / 2; i++) {
			memcpy(sptr + stride * pixel_h + stride * pixel_h / 4 + i * stride / 2, frame->data[1] + i * pixel_w / 2, pixel_w / 2);
		}
	}*/
	/*m_DxRes->Context->Unmap(m_DestImage, subresource);


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
	hr = m_DxRes->Device->CreateShaderResourceView(m_SharedSurf, &ShaderDesc, &ShaderResource);

	if (FAILED(hr))
	{
		return -1;
	}
	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;
	FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	m_DxRes->Context->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	m_DxRes->Context->CopyResource(m_SharedSurf, m_DestImage);
	m_DxRes->Context->OMSetRenderTargets(1, &m_RTV, nullptr);
	m_DxRes->Context->VSSetShader(m_VertexShader, nullptr, 0);
	m_DxRes->Context->PSSetShader(m_PixelShader, nullptr, 0);
	m_DxRes->Context->PSSetSamplers(0, 1, &m_SamplerLinear);
	m_DxRes->Context->PSSetShaderResources(0, 1, &ShaderResource);
	m_DxRes->Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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
	hr = m_DxRes->Device->CreateBuffer(&BufferDesc, &InitData, &VertexBuffer);
	m_DxRes->Context->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
	m_DxRes->Context->Draw(NUMVERTICES, 0);


	VertexBuffer->Release();
	VertexBuffer = nullptr;

	// Release shader resource
	ShaderResource->Release();
	ShaderResource = nullptr;

	m_DxRes->Context->RSSetViewports(1, &VP);
	m_DxRes->swapchain->Present(1, 0);
	/*HRESULT lRet;

	if (m_pDirect3DSurfaceRender == NULL)
		return -1;
	D3DLOCKED_RECT d3d_rect;
	lRet = m_pDirect3DSurfaceRender->LockRect(&d3d_rect, NULL, D3DLOCK_DONOTWAIT);
	if (FAILED(lRet))
		return -1;

	byte * pDest = (BYTE *)d3d_rect.pBits;
	int stride = d3d_rect.Pitch;
	unsigned long i = 0;

	int pixel_h = frame->height, pixel_w = frame->linesize[0];

	//Copy Data
	if (frame->format == AV_PIX_FMT_YUV420P)
	{
		for (i = 0; i < pixel_h; i++) {
			memcpy(pDest + i * stride, frame->data[0] + i * pixel_w, pixel_w);
		}
		for (i = 0; i < pixel_h / 2; i++) {
			memcpy(pDest + stride * pixel_h + i * stride / 2, frame->data[2] + i * pixel_w / 2, pixel_w / 2);
		}
		for (i = 0; i < pixel_h / 2; i++) {
			memcpy(pDest + stride * pixel_h + stride * pixel_h / 4 + i * stride / 2, frame->data[1] + i * pixel_w / 2, pixel_w / 2);
		}
	}
	else if (frame->format == AV_PIX_FMT_NV12)
	{
		for (i = 0; i < pixel_h; i++) {
			memcpy(pDest + i * stride, frame->data[0] + i * pixel_w, pixel_w);
		}
		for (i = 0; i < pixel_h / 2; i++) {
			memcpy(pDest + pixel_h * stride + i * stride, frame->data[1] + i * pixel_w, pixel_w);
		}
	}
	lRet = m_pDirect3DSurfaceRender->UnlockRect();
	if (FAILED(lRet))
		return -1;

	if (m_pDirect3DDevice == NULL)
		return -1;

	m_pDirect3DDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
	m_pDirect3DDevice->BeginScene();
	IDirect3DSurface9 * pBackBuffer = NULL;

	m_pDirect3DDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	m_pDirect3DDevice->StretchRect(m_pDirect3DSurfaceRender, NULL, pBackBuffer, &m_rtViewport, D3DTEXF_LINEAR);
	m_pDirect3DDevice->EndScene();
	m_pDirect3DDevice->Present(NULL, NULL, NULL, NULL);
	pBackBuffer->Release();*/

	/*return true;
}*/

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_FFMPEGX264CODEC, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FFMPEGX264CODEC));

    MSG msg;

    // Main message loop:
	// Main message loop:
	
	ZeroMemory(&msg, sizeof(msg));

	while (msg.message != WM_QUIT) {
		//PeekMessage, not GetMessage
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			driver(msg.hwnd);
			AVCodecCleanup();
		}
	}

	UnregisterClass(szWindowClass, hInstance);
	return 0;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FFMPEGX264CODEC));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_FFMPEGX264CODEC);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);

	
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
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	int pixel_w = 1008, pixel_h = 465;

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, 0, 0, pixel_w, pixel_h, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	GetWindowRect(GetDesktopWindow(), &desktop);
	pixel_w = desktop.right - desktop.left;
	pixel_h = desktop.bottom - desktop.top;
	
	if (InitD3D(hWnd, pixel_w, pixel_h) == E_FAIL) {
		return FALSE;
	}
	D3D11->get_hwnd(hWnd);
	

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

void decode(AVPacket *pkt)
{
	char buf[1024];
	int ret;

	ret = avcodec_send_packet(decodingCodecContext, pkt);//send packet for decoding
	if (ret < 0)
	{
		fprintf(log_file, "Error sending a packet for decoding\n");
		AVCodecCleanup();
	}

	while (ret >= 0) {
		ret = avcodec_receive_frame(decodingCodecContext, iframe);// read decoded frame and saved in iframe
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(log_file, "Error during decoding\n");
			AVCodecCleanup();
		}

		//printf("saving frame %3d\n", dc->frame_number);
		fflush(log_file);



		is_keyframe[decodingCodecContext->frame_number - 1] = av_get_picture_type_char(iframe->pict_type);
		if (iframe->pict_type == AV_PICTURE_TYPE_I)
		{
			number_of_I_frames++;
			fprintf(log_file, "I - frame index : %d\n", decodingCodecContext->frame_number);
		}
		else if (iframe->pict_type == AV_PICTURE_TYPE_P)
			number_of_P_frames++;
		iframe->width;
		/*if (iframe->format == AV_PIX_FMT_YUV420P)
			std::cout << "dcgfvgbh";
		if (iframe->format == AV_PIX_FMT_RGB32)
			std::cout << "dcdrftgyhjmkvgbh";*/
		int resp = sws_scale(m_resChangeCntxt, iframe->data, iframe->linesize, 0, iframe->height,
			m_resChangeFrame->data, m_resChangeFrame->linesize);

		D3D11->Render(m_resChangeFrame);



	}
}



void encode(AVFrame *frame, AVPacket *pkt)
{
	int ret;

	/* send the frame to the encoder */
	if (frame)
		printf("Send frame %d\n", frame->pts);

	ret = avcodec_send_frame(encodingCodecContext, frame);// send frame to encode
	if (ret < 0) {
		fprintf(log_file, "Error sending a frame for encoding\n");
		AVCodecCleanup();
	}

	while (ret >= 0)
	{
		ret = avcodec_receive_packet(encodingCodecContext, pkt);// read encoded data and saved in pkt
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return;
		}

		else if (ret < 0) {
			fprintf(log_file, "Error during encoding\n");
			AVCodecCleanup();
		}



		start_decode = clock();

		decode(pkt);
		stop_decode = clock();
		nDiff_decode = stop_decode - start_decode;
		time_for_decode += nDiff_decode;
		number_of_packets_decoded++;
		//fprintf(log_file, "time for decode - %d\n", nDiff);

		//fprintf(log_file, "size of packet- %d\n", pkt->size);
		total_size_of_packets += pkt->size;
		number_of_packets++;
		fwrite(pkt->data, 1, pkt->size, h264_file);
		av_packet_unref(pkt);

	}
}



//void createDirectory(const char * folder_name)
//{
//	CreateDirectory(folder_name, NULL);
//
//	if (ERROR_ALREADY_EXISTS == GetLastError())
//	{
//		fprintf(log_file, "The Captured directory already exists.\n");
//	}
//	else
//	{
//		fprintf(log_file, "The Captured directory could not be created.\n");
//	}
//}

void driver(HWND hWnd)
{
	const char *filename, *codec_name, *folder_name;
	const AVCodec *codec, *decodec;
	int i, ret, x, y;

	uint8_t endcode[] = { 0, 0, 1, 0xb7 };//footer

	fopen_s(&log_file, "log.txt", "w");


	filename = "Content\\encoded.h264";

	avcodec_register_all();

	/* find the video encoder */
	codec = avcodec_find_encoder_by_name("libx264");
	if (!codec) {
		fprintf(log_file, "Codec libx264 not found\n");
		AVCodecCleanup();
	}

	encodingCodecContext = avcodec_alloc_context3(codec);
	if (!encodingCodecContext) {
		fprintf(log_file, "Could not allocate video codec context\n");
		AVCodecCleanup();
	}

	pkt = av_packet_alloc();// after encoding save in this
	if (!pkt)
		AVCodecCleanup();

	/* put sample parameters */
	encodingCodecContext->bit_rate = 4000000;

	/* resolution must be a multiple of two */
	// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(NULL, &rcClient);

	int width = 0, height = 0;  //initial values


								// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();

	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);

	// The top left corner will have coordinates (0,0)
	// and the bottom right corner will have coordinates
	// (horizontal, vertical)
	width = desktop.right;
	height = desktop.bottom;

	// We always want the input to encode as 
	// a frame with a fixed resolution 
	// irrespective of screen resolution.
	encodingCodecContext->width = width;
	encodingCodecContext->height = height;

	/* frames per second */
	encodingCodecContext->time_base.num = 1;
	encodingCodecContext->time_base.den = 60;
	encodingCodecContext->framerate.num = 60;
	encodingCodecContext->framerate.den = 1;


	encodingCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;


	if (codec->id == AV_CODEC_ID_H264)
	{
		av_opt_set(encodingCodecContext->priv_data, "preset", "veryfast", 0);
		av_opt_set(encodingCodecContext->priv_data, "tune", "zerolatency", 0);
		av_opt_set(encodingCodecContext->priv_data, "crf", "10", 0);

	}

	/* open it */
	ret = avcodec_open2(encodingCodecContext, codec, NULL);
	if (ret < 0) {
		printf("Could not open codec: \n");
		AVCodecCleanup();
	}


	fopen_s(&h264_file, filename, "wb");
	if (!h264_file) {
		fprintf(log_file, "Could not open %s\n", filename);
		AVCodecCleanup();
	}


	inframe = av_frame_alloc();
	if (!inframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		AVCodecCleanup();
	}
	inframe->format = AV_PIX_FMT_RGB32;
	inframe->width = width;
	inframe->height = height;

	ret = av_frame_get_buffer(inframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		AVCodecCleanup();
	}


	outframe = av_frame_alloc();
	if (!outframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		AVCodecCleanup();
	}
	outframe->format = AV_PIX_FMT_YUV420P;
	outframe->width = encodingCodecContext->width;
	outframe->height = encodingCodecContext->height;



	ret = av_frame_get_buffer(outframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		AVCodecCleanup();
	}


	iframe = av_frame_alloc();
	if (!iframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		AVCodecCleanup();
	}

	iframe->format = AV_PIX_FMT_YUV420P;
	iframe->width = encodingCodecContext->width;
	iframe->height = encodingCodecContext->height;

	ret = av_frame_get_buffer(iframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		AVCodecCleanup();
	}

	oframe = av_frame_alloc();
	if (!oframe) {
		fprintf(log_file, "Could not allocate video frame\n");
		AVCodecCleanup();
	}

	oframe->format = AV_PIX_FMT_RGB32;
	oframe->width = width;
	oframe->height = height;

	ret = av_frame_get_buffer(oframe, 32);
	if (ret < 0) {
		fprintf(log_file, "Could not allocate the video frame data\n");
		AVCodecCleanup();
	}

	/* initialize the decoder */
	decodec = avcodec_find_decoder_by_name("h264");
	if (!decodec) {
		fprintf(log_file, "The QSV decoder is not present in libavcodec\n");
		AVCodecCleanup();
	}

	decodingCodecContext = avcodec_alloc_context3(decodec);
	if (!decodingCodecContext) {
		ret = AVERROR(ENOMEM);
		AVCodecCleanup();
	}
	decodingCodecContext->codec_id = AV_CODEC_ID_H264;


	/* open it */
	ret = avcodec_open2(decodingCodecContext, NULL, NULL);
	if (ret < 0) {
		fprintf(log_file, "Error opening the output context: \n");
		AVCodecCleanup();
	}

	//Capture flow is here

	long CaptureLineSize = NULL;

	//Capture part ends here.
	D3D11->init();


	CaptureBuffer = (UCHAR*)malloc(inframe->height*inframe->linesize[0]);

	rgb_to_yuv_SwsContext = sws_getContext(inframe->width, inframe->height, AV_PIX_FMT_RGB32,
		outframe->width, outframe->height, AV_PIX_FMT_YUV420P,
		SWS_BICUBIC, NULL, NULL, NULL);
	/* create scaling context */

	if (!rgb_to_yuv_SwsContext) {
		fprintf(log_file,
			"Impossible to create scale context for the conversion "
			"fmt:%s s:%dx%d -> fmt:%s s:%dx%d\n",
			av_get_pix_fmt_name(AV_PIX_FMT_RGB32), inframe->width, inframe->height,
			av_get_pix_fmt_name(AV_PIX_FMT_YUV420P), outframe->width, outframe->height);
		ret = AVERROR(EINVAL);
		AVCodecCleanup();
	}

	char buf[1024];

	int number_of_frames_to_process = 300;
	is_keyframe = (char *)malloc(number_of_frames_to_process + 10);

	for (i = 0; i < number_of_frames_to_process; i++)
	{
		fflush(stdout);

		char * srcData = NULL;

		start = clock();
		D3D11->GrabImage(CaptureBuffer, CaptureLineSize);
		stop = clock();


		nDiff = stop - start;
		time_for_capture += nDiff;
		number_of_frames_captured++;

		//fprintf(file, "time for capture - %d\n", nDiff);

		uint8_t * inData[1] = { (uint8_t *)CaptureBuffer }; // RGB have one plane
		int inLinesize[1] = { av_image_get_linesize(AV_PIX_FMT_RGB32, inframe->width, 0) }; // RGB stride

		start = clock();

		int scale_rc = sws_scale(rgb_to_yuv_SwsContext, inData, inLinesize, 0, inframe->height, outframe->data,
			outframe->linesize);


		stop = clock();
		nDiff = stop - start;
		//fprintf(file, "time for scale - %d\n", nDiff);
		time_for_scaling_rgb_to_yuv += nDiff;
		number_of_rgb_frames_scaled++;

		outframe->pts = i;

		/* encode the image */
		start = clock();
		encode(outframe, pkt);
		stop = clock();
		nDiff = stop - start;
		time_for_encode += nDiff;
		number_of_frames_encoded++;
		//fprintf(file, "time for encode - %d\n", nDiff);

	}

	/* flush the encoder */
	start = clock();
	encode(NULL, pkt);
	stop = clock();
	nDiff = stop - start;
	time_for_encode += nDiff;
	number_of_frames_encoded++;

	/* add sequence end code to have a real MPEG file */
	fwrite(endcode, 1, sizeof(endcode), h264_file);
	fclose(h264_file);



	fprintf(log_file, "average time for scale - %f\n", time_for_scaling_rgb_to_yuv / (float)number_of_rgb_frames_scaled);
	fprintf(log_file, "average time for encode - %f\n", time_for_encode / (float)number_of_frames_encoded);
	fprintf(log_file, "average time for decode - %f\n", time_for_decode / (float)number_of_packets_decoded);
	fprintf(log_file, "average time for capture - %f\n", time_for_capture / (float)number_of_frames_captured);
	fprintf(log_file, "average size after encoding - %f\n", total_size_of_packets / (float)number_of_packets);
	is_keyframe[decodingCodecContext->frame_number] = '\0';
	fwrite(is_keyframe, 1, decodingCodecContext->frame_number + 1, log_file);
	fprintf(log_file, "\n", NULL);
	fprintf(log_file, "Number of I - frames : %d\n", number_of_I_frames);
	fprintf(log_file, "Number of P - frames : %d\n", number_of_packets);

}
//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
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

    case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		return 0;
	}
   return DefWindowProc(hWnd, message, wParam, lParam);
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
