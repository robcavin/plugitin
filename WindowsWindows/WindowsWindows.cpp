#include "Helper.h"
#include "UnityPluginInterface.h"

#include <dxgi1_2.h>
#include <d3d11.h>

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(a) if (a) { a->Release(); a = NULL; }
#endif

typedef HRESULT(WINAPI *D3DCompileFunc)(
	const void* pSrcData,
	unsigned long SrcDataSize,
	const char* pFileName,
	const D3D10_SHADER_MACRO* pDefines,
	ID3D10Include* pInclude,
	const char* pEntrypoint,
	const char* pTarget,
	unsigned int Flags1,
	unsigned int Flags2,
	ID3D10Blob** ppCode,
	ID3D10Blob** ppErrorMsgs);


static const char* kD3D11VertexShaderText =
"struct Input {\n"\
"	float3 pos : POSITION;\n"\
"	float2 tex : TEXCOORD;\n"\
"};\n"\
"struct Output {\n"\
"	float4 pos : SV_POSITION;\n"\
"	float2 tex : TEXCOORD;\n"\
"};\n"\
"cbuffer VS_CONSTANT_BUFFER : register(b0) {\n"\
"	float4x4 projectionMatrix;\n"\
"}\n"\
"Output main(Input i) {\n"\
"	Output o;\n"\
"	o.pos = mul(float4(i.pos, 1), projectionMatrix);\n"\
/*"	o.pos = mul(projectionMatrix,float4(i.pos, 1));\n"\*/
"	o.tex = i.tex;\n"\
"	return o;\n"\
"}\n";

static const char* kD3D11PixelShaderText =
"struct Input {\n"\
"	float4 pos : SV_POSITION;\n"\
"	float2 tex : TEXCOORD;\n"\
"};\n"\
"Texture2D source : register(t0);\n"\
"sampler pointSampler : register(s0);\n"\
"float4 main(Input i) : SV_TARGET{\n"\
"	return float4(source.Sample(pointSampler, i.tex).rgb, 0);\n"\
"}";

static D3D11_INPUT_ELEMENT_DESC s_DX11InputElementDesc[] = {
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};


// Define the data-type that
// describes a vertex.
struct SimpleVertex
{
	float Pos[3];
	float TexCoord[2];
};

// Define the constant data used to communicate with shaders.
struct SimpleConstantBuffer
{
	float mWorldViewProj[16];
};

// Supply the actual vertex data.
SimpleVertex quadVertices[] =
{
	{ -0.5f, -0.5f, 0.0f, 0.0f, 1.0f },
	{ -0.5f, 0.5f, 0.0f, 0.0f, 0.0f },
	{ 0.5f, 0.5f, 0.0f, 1.0f, 0.0f },

	{ 0.5f, 0.5f, 0.0f, 1.0f, 0.0f },
	{ 0.5f, -0.5f, 0.0f, 1.0f, 1.0f },
	{ -0.5f, -0.5f, 0.0f, 0.0f, 1.0f },
};


struct WindowsWindows {

	ID3D11Device* device = nullptr;

	ID3D11VertexShader* vertexShader = nullptr;
	ID3D11PixelShader* pixelShader = nullptr;

	ID3D11Buffer* vertexBuffer = nullptr;
	ID3D11InputLayout* vertexBufferInputLayout = nullptr;
	ID3D11Buffer* vertexShaderConstantBuffer = nullptr;

	ID3D11RasterizerState* rasterState = nullptr;
	ID3D11DepthStencilState* depthStencilState = nullptr;
	ID3D11BlendState* blendState = nullptr;
	ID3D11SamplerState* samplerState = nullptr;

	IDXGIOutputDuplication* duplication = nullptr;

	ID3D11Texture2D* quadTexture = nullptr;
	ID3D11ShaderResourceView* quadTextureSRV = nullptr;
	D3D11_BOX quadTextureMip0Bounds;

	void compileShaders();

	float renderMatrix[16];
};

static WindowsWindows g_Windows;

void WindowsWindows::compileShaders() {
	// shaders
	HMODULE compiler = LoadLibraryA("D3DCompiler_43.dll");

	if (compiler == NULL)
	{
		// Try compiler from Windows 8 SDK
		compiler = LoadLibraryA("D3DCompiler_46.dll");
	}
	if (compiler)
	{
		ID3D10Blob* vsBlob = NULL;
		ID3D10Blob* psBlob = NULL;

		D3DCompileFunc compileFunc = (D3DCompileFunc)GetProcAddress(compiler, "D3DCompile");
		if (compileFunc)
		{
			HRESULT hr;
			hr = compileFunc(kD3D11VertexShaderText, strlen(kD3D11VertexShaderText), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &vsBlob, NULL);
			if (SUCCEEDED(hr))
			{
				device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &vertexShader);
			}

			hr = compileFunc(kD3D11PixelShaderText, strlen(kD3D11PixelShaderText), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &psBlob, NULL);
			if (SUCCEEDED(hr))
			{
				device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &pixelShader);
			}
		}

		// input layout
		if (vertexShader && vsBlob)
		{
			device->CreateInputLayout(s_DX11InputElementDesc, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &vertexBufferInputLayout);
		}

		SAFE_RELEASE(vsBlob);
		SAFE_RELEASE(psBlob);

		FreeLibrary(compiler);
	}
}


extern "C" void EXPORT_API UnitySetGraphicsDevice(void* device, int deviceType, int eventType)
{

	if (deviceType == kGfxRendererD3D11 && eventType == kGfxDeviceEventInitialize)
	{
		g_Windows.device = (ID3D11Device*)device;

		// render states
		D3D11_RASTERIZER_DESC rsdesc;
		memset(&rsdesc, 0, sizeof(rsdesc));
		rsdesc.FillMode = D3D11_FILL_SOLID;
		rsdesc.CullMode = D3D11_CULL_BACK;
		rsdesc.FrontCounterClockwise = false;
		rsdesc.DepthClipEnable = true;
		rsdesc.MultisampleEnable = true;
		g_Windows.device->CreateRasterizerState(&rsdesc, &g_Windows.rasterState);

		D3D11_DEPTH_STENCIL_DESC dsdesc;
		memset(&dsdesc, 0, sizeof(dsdesc));
		dsdesc.DepthEnable = true;
		dsdesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dsdesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		g_Windows.device->CreateDepthStencilState(&dsdesc, &g_Windows.depthStencilState);

		D3D11_BLEND_DESC bdesc;
		memset(&bdesc, 0, sizeof(bdesc));
		bdesc.RenderTarget[0].BlendEnable = false;
		bdesc.RenderTarget[0].RenderTargetWriteMask = 0xF;
		g_Windows.device->CreateBlendState(&bdesc, &g_Windows.blendState);

		D3D11_SAMPLER_DESC samplerDesc = {};
		float color[4] = { 0, 0, 0, 0 };
		samplerDesc.BorderColor[0] = 0;
		samplerDesc.BorderColor[1] = 0;
		samplerDesc.BorderColor[2] = 0;
		samplerDesc.BorderColor[3] = 0;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;

		validateHR(g_Windows.device->CreateSamplerState(&samplerDesc, &g_Windows.samplerState), "CreateSamplerState");

		// Fill in a buffer description.
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		bufferDesc.ByteWidth = sizeof(SimpleVertex)* _countof(quadVertices);
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;

		// Fill in the subresource data.
		D3D11_SUBRESOURCE_DATA InitData;
		InitData.pSysMem = quadVertices;
		InitData.SysMemPitch = 0;
		InitData.SysMemSlicePitch = 0;

		// Create the vertex buffer.
		validateHR(g_Windows.device->CreateBuffer(&bufferDesc, &InitData, &g_Windows.vertexBuffer), "BuildVertexBuffer");


		/// CREATE CONSTANT BUFFER FOR VERTEX SHADER
		//

		// Fill in a buffer description.
		D3D11_BUFFER_DESC cbDesc;
		cbDesc.ByteWidth = sizeof(SimpleConstantBuffer);
		cbDesc.Usage = D3D11_USAGE_DYNAMIC;
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cbDesc.MiscFlags = 0;
		cbDesc.StructureByteStride = 0;

		// Create the buffer.
		validateHR(g_Windows.device->CreateBuffer(&cbDesc, nullptr, &g_Windows.vertexShaderConstantBuffer), "BuildConstantBuffer");

		g_Windows.compileShaders();

		IDXGIDevice* DxgiDevice = nullptr;
		validateHR(g_Windows.device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice)), "GetDXGIDevice");

		IDXGIAdapter* DxgiAdapter = nullptr;
		validateHR(DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter)), "GetDXGIAdapter");

		IDXGIOutput* DxgiOutput = nullptr;
		validateHR(DxgiAdapter->EnumOutputs(0, &DxgiOutput), "GetDXGIOutput");

		IDXGIOutput1* DxgiOutput1 = nullptr;
		validateHR(DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1)), "GetDXGIOuput1");

		validateHR(DxgiOutput1->DuplicateOutput(g_Windows.device, &g_Windows.duplication), "GetDuplicateOutput");
	}


	if (deviceType == kGfxRendererD3D11 && eventType == kGfxDeviceEventShutdown) {

		SAFE_RELEASE(g_Windows.duplication);

		SAFE_RELEASE(g_Windows.quadTextureSRV);
		SAFE_RELEASE(g_Windows.quadTexture);
		SAFE_RELEASE(g_Windows.samplerState);
		SAFE_RELEASE(g_Windows.blendState);
		SAFE_RELEASE(g_Windows.depthStencilState);
		SAFE_RELEASE(g_Windows.rasterState);
		SAFE_RELEASE(g_Windows.vertexShaderConstantBuffer);
		SAFE_RELEASE(g_Windows.vertexBufferInputLayout);
		SAFE_RELEASE(g_Windows.vertexBuffer);
		SAFE_RELEASE(g_Windows.vertexShader);
		SAFE_RELEASE(g_Windows.pixelShader);

		g_Windows.device = nullptr;
	}

}


extern "C" void EXPORT_API UnityRenderEvent(int eventID)
{
	if (g_Windows.device)
	{
		ID3D11DeviceContext* context = nullptr;
		g_Windows.device->GetImmediateContext(&context);
		context->OMSetDepthStencilState(g_Windows.depthStencilState, 0);
		context->RSSetState(g_Windows.rasterState);
		context->OMSetBlendState(g_Windows.blendState, NULL, 0xFFFFFFFF);

		DXGI_OUTDUPL_FRAME_INFO frameInfo;
		IDXGIResource* desktopResource = nullptr;

		HRESULT hr = g_Windows.duplication->AcquireNextFrame(0, &frameInfo, &desktopResource);
		if (hr != DXGI_ERROR_WAIT_TIMEOUT) {

			ID3D11Texture2D* desktopImage = nullptr;
			validateHR(desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&desktopImage)), "GetDesktopImage");

			if (g_Windows.quadTexture == nullptr) {
				D3D11_TEXTURE2D_DESC desktopImageDesc;
				desktopImage->GetDesc(&desktopImageDesc);
				desktopImageDesc.MipLevels = 12;
				desktopImageDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
				desktopImageDesc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

				validateHR(g_Windows.device->CreateTexture2D(&desktopImageDesc, nullptr, &g_Windows.quadTexture), "CreateQuadTexture");
				validateHR(g_Windows.device->CreateShaderResourceView(g_Windows.quadTexture, nullptr, &g_Windows.quadTextureSRV), "GetQuadTextureSRV");

				D3D11_BOX box;
				box.left = 0; box.right = desktopImageDesc.Width;
				box.top = 0; box.bottom = desktopImageDesc.Height;
				box.front = 0; box.back = 1;
				g_Windows.quadTextureMip0Bounds = box;
			}

			context->CopySubresourceRegion(g_Windows.quadTexture, 0, 0, 0, 0, desktopImage, 0, &g_Windows.quadTextureMip0Bounds);
			context->GenerateMips(g_Windows.quadTextureSRV);
			desktopImage->Release(); desktopImage = nullptr;
			desktopResource->Release(); desktopResource = nullptr;
			validateHR(g_Windows.duplication->ReleaseFrame(), "ReleaseFrame");
		}

		D3D11_MAPPED_SUBRESOURCE map;
		validateHR(context->Map(g_Windows.vertexShaderConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map), "Map");
		memcpy(map.pData, g_Windows.renderMatrix, sizeof(SimpleConstantBuffer));
		context->Unmap(g_Windows.vertexShaderConstantBuffer, 0);		

		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		context->IASetVertexBuffers(0, 1, &g_Windows.vertexBuffer, &stride, &offset);
		context->IASetInputLayout(g_Windows.vertexBufferInputLayout);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->VSSetConstantBuffers(0, 1, &g_Windows.vertexShaderConstantBuffer);
		context->VSSetShader(g_Windows.vertexShader, nullptr, 0);
		context->PSSetShader(g_Windows.pixelShader, nullptr, 0);
		context->PSSetSamplers(0, 1, &g_Windows.samplerState);
		context->PSSetShaderResources(0, 1, &g_Windows.quadTextureSRV);
		context->Draw(6, 0);

		context->Release();
	}
}

extern "C" void EXPORT_API SayHello() {
	g_Windows = g_Windows;
}

extern "C" void EXPORT_API SetRenderMatrix(float renderMatrix[16]) {
	memcpy(g_Windows.renderMatrix, renderMatrix, 16 * sizeof(float));
}

extern "C" EXPORT_API UINT GetScreenWidth() {
	return g_Windows.quadTextureMip0Bounds.right - g_Windows.quadTextureMip0Bounds.left;
}

extern "C" EXPORT_API UINT GetScreenHeight() {
	return g_Windows.quadTextureMip0Bounds.bottom - g_Windows.quadTextureMip0Bounds.top;
}