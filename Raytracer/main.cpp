#include "stdafx.h"
#include "main.h"
#include "WindowManager.h"

//render target
ID3D11Buffer* g_pOutputBuffer = 0;
ID3D11UnorderedAccessView* g_pOutputUAV = 0;
ID3D11ShaderResourceView* g_pOutputSRV = 0;

//random seed buffer
ID3D11Buffer* g_pRandSeedBuffer = 0;
ID3D11ShaderResourceView* g_pRandSeedBufferSRV = 0;

//shaders
ID3D11ComputeShader* g_pCS = 0;
ID3D11VertexShader* g_pVS = 0;
ID3D11PixelShader* g_pPS = 0;
ID3D11InputLayout* g_pIL = 0;

//VB for full screen quad
ID3D11Buffer* g_pVB = 0;

ID3D11DepthStencilState* g_pDepthStencilState = 0;
ID3D11SamplerState* g_pSamplerState = 0;

Input gInput;
Timer gTimer;
Scene gScene;




#define STRINGIFY(x) #x
#define TO_TEXT(a) STRINGIFY(a)


#define THREADS_X (32)
#define THREADS_Y (16)
#define STACK_DEPTH (3)

#define THREAD_COUNT (THREADS_X*THREADS_Y)



struct ThreadData
{
	u32 stackPtr;
	float globals[64]; // 64 global floats (for dimensions etc. )
	float data[64][64]; // 64 calls ( 1 call can have 64 floats )
};

struct ThreadDataBuffer // if THREAD_COUNT == 512 then sizeof(ThreadDataBuffer) = ~128MB
{
	ThreadData thdatas[THREAD_COUNT];
}g_threadDataBuffer;



class IncludeHandler : public ID3D10Include
{
public:
	 HRESULT __stdcall Open(D3D10_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
	{
		char path[256];
		strcpy(path, "..\\Data\\Shaders\\");
		strcat(path, pFileName);

		FILE* pSrcFile = 0;
		errno_t err = fopen_s(&pSrcFile, path, "rb");
		if (err != 0)
		{
			return E_FAIL;
		}

		fseek(pSrcFile, 0L, SEEK_END);
		UINT SrcFileSize = ftell(pSrcFile);
		fseek(pSrcFile, 0L, SEEK_SET);
		char* pSrcFileData = (char*)malloc(SrcFileSize);
		fread(pSrcFileData, SrcFileSize, 1, pSrcFile);
		fclose (pSrcFile);

		*ppData = pSrcFileData;
		*pBytes = SrcFileSize;

		return S_OK;
	}

	HRESULT __stdcall Close(LPCVOID pData)
	{
		//free(pData);
		return S_OK;
	}
};

bool CompileShader(const char* pName, const char* pTarget, void** ppShader, ID3DBlob** ppShaderCode, const D3D_SHADER_MACRO* pDefines)
{
	FILE* pSrcFile = 0;
	if (fopen_s(&pSrcFile, pName, "rb") != 0)
	{
		return false;
	}

	fseek(pSrcFile, 0L, SEEK_END);
	UINT SrcFileSize = ftell(pSrcFile);
	fseek(pSrcFile, 0L, SEEK_SET);
	char* pSrcFileData = (char*)malloc(SrcFileSize);
	fread(pSrcFileData, SrcFileSize, 1, pSrcFile);
	fclose (pSrcFile);

	ID3DBlob* pBufferErrors = 0;
	UINT Flags = 0;
	Flags |= D3DCOMPILE_SKIP_VALIDATION;
	Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;

	/*
	Flags |= D3DCOMPILE_PREFER_FLOW_CONTROL;
	Flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
	*/
	IncludeHandler includeHandler;
	HRESULT HR = D3DCompile(pSrcFileData, SrcFileSize, pName, pDefines, &includeHandler, "main", pTarget, Flags, 0, ppShaderCode, &pBufferErrors);
	free(pSrcFileData);

	if (pBufferErrors)
	{
		//MessageBox(0,(LPCSTR)pBufferErrors->GetBufferPointer(), (LPCSTR)pBufferErrors->GetBufferPointer(), 0);
		OutputDebugStringA((LPCSTR)pBufferErrors->GetBufferPointer());
		pBufferErrors->Release();
	}

	if (FAILED(HR))
		return false;



	ID3DBlob* pDisassembly = 0;
	D3DDisassemble((*ppShaderCode)->GetBufferPointer(), (*ppShaderCode)->GetBufferSize(), 
					D3D_DISASM_ENABLE_COLOR_CODE, 0, &pDisassembly);

	if (pDisassembly)
	{
		char pDisassemblyPath[512];
		strcpy(pDisassemblyPath, pName);
		strcat(pDisassemblyPath, "_disassembly.html");
		FILE* pFile = 0;
		if (fopen_s(&pFile, pDisassemblyPath, "w") == 0)
		{
			fwrite(pDisassembly->GetBufferPointer(), pDisassembly->GetBufferSize(), 1, pFile);
			fclose (pFile);
		}

		pDisassembly->Release();
	}


	HR = 1;

	if ((strcmp(pTarget, "cs_4_0") == 0) || (strcmp(pTarget, "cs_4_1") == 0) || (strcmp(pTarget, "cs_5_0") == 0))
		HR = g_pd3dDevice->CreateComputeShader((*ppShaderCode)->GetBufferPointer(), (*ppShaderCode)->GetBufferSize(), 0, (ID3D11ComputeShader**)ppShader);

	if ((strcmp(pTarget, "vs_4_0") == 0) || (strcmp(pTarget, "vs_4_1") == 0) || (strcmp(pTarget, "vs_5_0") == 0))
		HR = g_pd3dDevice->CreateVertexShader((*ppShaderCode)->GetBufferPointer(), (*ppShaderCode)->GetBufferSize(), 0, (ID3D11VertexShader**)ppShader);
	
	if ((strcmp(pTarget, "ps_4_0") == 0) || (strcmp(pTarget, "ps_4_1") == 0) || (strcmp(pTarget, "ps_5_0") == 0))
		HR = g_pd3dDevice->CreatePixelShader((*ppShaderCode)->GetBufferPointer(), (*ppShaderCode)->GetBufferSize(), 0, (ID3D11PixelShader**)ppShader);

	return SUCCEEDED(HR);
}

static float g_numSample = 0.0f;
bool g_simpleMode = true;

void update(u32 width, u32 height)
{
	gTimer.update();
	gInput.update();
	float3 move(0,0,0), rotate(0,0,0);
	float speed = 0.005f, speedRot = 0.003f;
	bool changedView = false;
	if(gInput.isKeyDown(DIK_LSHIFT)){ changedView = true; speed *= 10.0f; }
	if(gInput.isKeyDown(DIK_LCONTROL)){ changedView = true; speed *= 0.1f; }
	if(gInput.isKeyDown(DIK_W)){ changedView = true; move.z = speed*(f32)gTimer.get(); }
	else if(gInput.isKeyDown(DIK_S)){ changedView = true; move.z = -speed*(f32)gTimer.get(); }
	if(gInput.isKeyDown(DIK_A)){ changedView = true; move.x = -speed*(f32)gTimer.get(); }
	else if(gInput.isKeyDown(DIK_D)){ changedView = true; move.x = speed*(f32)gTimer.get(); }
	if(gInput.isMousePressed(0) || gInput.isKeyDown(DIK_SPACE)){ g_simpleMode = true; changedView = true; rotate.x = gInput.getMouseRel()->y*speedRot; rotate.y = gInput.getMouseRel()->x*speedRot; }
	if(gInput.isMousePressed(1) || gInput.isMousePressed(2))g_simpleMode = false;
	gScene.cam.moveByRel(move);
	gScene.cam.rotateBy(rotate);
	gScene.cam.update(width, height);
	if(changedView)g_numSample = 0.0f;
	else g_numSample += 1.0f;
	g_simpleMode = false;
	if(g_simpleMode)g_numSample = 0.0f;
}

void Demo_OnWindowResize()
{
	SAFE_RELEASE(g_pOutputSRV);
	SAFE_RELEASE(g_pOutputUAV);
	SAFE_RELEASE(g_pOutputBuffer);

	// RENDER BUFFER
	D3D11_BUFFER_DESC rbd;
	rbd.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
	rbd.StructureByteStride = 16; // 4 * float
	rbd.ByteWidth = g_Window.Height * g_Window.Width * rbd.StructureByteStride;
	rbd.CPUAccessFlags = 0;
	rbd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	rbd.Usage = D3D11_USAGE_DEFAULT;
	g_pd3dDevice->CreateBuffer(&rbd, 0, &g_pOutputBuffer);

	//create UAV for compute shader to write results
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
	uavd.Buffer.FirstElement = 0;
	uavd.Buffer.Flags = 0;
	uavd.Buffer.NumElements = g_Window.Height * g_Window.Width;
	uavd.Format = DXGI_FORMAT_UNKNOWN;
	uavd.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	g_pd3dDevice->CreateUnorderedAccessView(g_pOutputBuffer, &uavd, &g_pOutputUAV);

	//create SRV for pixel shader to diplay generated buffer
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	srvd.Format = DXGI_FORMAT_UNKNOWN;
	srvd.Buffer.FirstElement = 0;
	srvd.Buffer.ElementWidth = g_Window.Height * g_Window.Width;

	//srvd.Buffer.ElementOffset = 0;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	g_pd3dDevice->CreateShaderResourceView(g_pOutputBuffer, &srvd, &g_pOutputSRV);




	// generate random seed buffer
	SAFE_RELEASE(g_pRandSeedBufferSRV);
	SAFE_RELEASE(g_pRandSeedBuffer);

	rbd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	rbd.StructureByteStride = 8; // 2 * uint
	rbd.ByteWidth = g_Window.Height * g_Window.Width * rbd.StructureByteStride;
	rbd.CPUAccessFlags = 0;
	rbd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	rbd.Usage = D3D11_USAGE_IMMUTABLE;

	u32* pBuff = new u32 [g_Window.Height * g_Window.Width * 2];
	for (int i = 0; i<g_Window.Height * g_Window.Width * 2; i++)
		pBuff[i] = (1<<0)*(rand()%256) + (1<<8)*(rand()%256) + (1<<16)*(rand()%256) + (1<<24)*(rand()%256);


	D3D11_SUBRESOURCE_DATA sd;
	sd.pSysMem = pBuff;
	sd.SysMemPitch = 0;
	sd.SysMemSlicePitch = 0;
	g_pd3dDevice->CreateBuffer(&rbd, &sd, &g_pRandSeedBuffer);

	delete[] pBuff;

	srvd.Format = DXGI_FORMAT_UNKNOWN;
	srvd.Buffer.FirstElement = 0;
	srvd.Buffer.ElementWidth = g_Window.Height * g_Window.Width;
	//srvd.Buffer.ElementOffset = 0;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	g_pd3dDevice->CreateShaderResourceView(g_pRandSeedBuffer, &srvd, &g_pRandSeedBufferSRV);
}

struct OutputInfo
{
	u32 seedA;
	u32 seedB;
	u32 res[2];
	float aspectRatio;
	float numSamples;
	float2 sth;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	srand(time(0));

	g_Window.Top = 0;
	g_Window.Left = 0;
	g_Window.Width = 512;
	g_Window.Height = 512;
	g_Window.Fullscreen = false;
	g_Window.Vsync = false;
	g_Window.hInstance = hInstance;

	if (!Demo_InitWindow("Raytracer"))
		return 2;

	if (!Demo_InitDirect3D(0))
	{
		Demo_ReleaseDirect3D();
		return 2;
	}


	const D3D_SHADER_MACRO pMacros[] = { {"THREADS_X", TO_TEXT(THREADS_X)},
										 {"THREADS_Y", TO_TEXT(THREADS_Y)},
										 {"STACK_DEPTH", TO_TEXT(STACK_DEPTH)},
										 {0, 0} };

	ID3DBlob* pShaderCode;
	if (!CompileShader("..\\Data\\Shaders\\cs.hlsl", "cs_5_0", (void**)&g_pCS, &pShaderCode, pMacros))
	{
		MessageBoxA(g_Window.hWnd, "Failed to compile CS!", "Error", 0);
		return 1;
	}
	pShaderCode->Release();


	if (!CompileShader("..\\Data\\Shaders\\ps.hlsl", "ps_4_0", (void**)&g_pPS, &pShaderCode, 0))
	{
		MessageBoxA(g_Window.hWnd, "Failed to compile PS!", "Error", 0);
		return 1;
	}
	pShaderCode->Release();


	if (!CompileShader("..\\Data\\Shaders\\vs.hlsl", "vs_4_0", (void**)&g_pVS, &pShaderCode, 0))
	{
		MessageBoxA(g_Window.hWnd, "Failed to compile VS!", "Error", 0);
		return 1;
	}

    // Create our vertex input layout
	const D3D11_INPUT_ELEMENT_DESC LayoutElements[] =
    {
		{ "POSITION",	0, DXGI_FORMAT_R32G32_FLOAT,	0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

	g_pd3dDevice->CreateInputLayout(LayoutElements, 1, 
									pShaderCode->GetBufferPointer(), pShaderCode->GetBufferSize(),
									&g_pIL);
	pShaderCode->Release();


	//bind shaders
	g_pImmediateContext->CSSetShader(g_pCS, 0, 0);
	g_pImmediateContext->VSSetShader(g_pVS, 0, 0);
	g_pImmediateContext->PSSetShader(g_pPS, 0, 0);



	float pBufferData[] = {0.0f, 0.0f,
						  1.0f, 1.0f,
						  1.0f, 0.0f,
						  0.0f, 0.0f,
						  0.0f, 1.0f,
						  1.0f, 1.0f};

	D3D11_BUFFER_DESC bd;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.ByteWidth = 12 * sizeof(float);
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
	bd.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA id;
	id.pSysMem = pBufferData;
	id.SysMemPitch = 0;
	id.SysMemSlicePitch = 0;
	g_pd3dDevice->CreateBuffer(&bd, &id, &g_pVB);


	UINT strides[] = {2*sizeof(float)};
	UINT offsets[] = {0};
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVB, strides, offsets);
	g_pImmediateContext->IASetInputLayout(g_pIL);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);



	D3D11_SAMPLER_DESC samplerDesc =
	{
		D3D11_FILTER_MIN_MAG_MIP_POINT,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		D3D11_TEXTURE_ADDRESS_CLAMP,
		0.0f,
		16,
		D3D11_COMPARISON_ALWAYS,
		0.0f, 0.0f, 0.0f, 0.0f,
		-3.402823466e+38F,
		3.402823466e+38F,
	};
	g_pd3dDevice->CreateSamplerState(&samplerDesc, &g_pSamplerState);

	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerState);


	Demo_OnWindowResize();




	/*************************LOAD SCENE*************************/
	bool bLoadSponza = 0;
	gScene.load(bLoadSponza?"..\\Data\\objects\\sponza.obj":"..\\Data\\objects\\room1h.obj");

	// VERTEX
	D3D11_BUFFER_DESC vbd;
	vbd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	vbd.ByteWidth = gScene.vertex_buffer.size() * sizeof(Vertex);
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	vbd.StructureByteStride = sizeof(Vertex);
	vbd.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA vsd;
	vsd.pSysMem =  &gScene.vertex_buffer[0];
	vsd.SysMemPitch = 0;
	vsd.SysMemSlicePitch = 0;

	ID3D11Buffer* g_pVertexBuffer = 0;
	g_pd3dDevice->CreateBuffer(&vbd, &vsd, &g_pVertexBuffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC vsrvd;
	vsrvd.Format = DXGI_FORMAT_UNKNOWN;
	vsrvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	vsrvd.Buffer.ElementOffset = 0;
	vsrvd.Buffer.ElementWidth = gScene.vertex_buffer.size();

	ID3D11ShaderResourceView* g_pVertexBufferSRV = 0;
	g_pd3dDevice->CreateShaderResourceView(g_pVertexBuffer, &vsrvd, &g_pVertexBufferSRV);

	//3D TEXTURE DATA

	const size_t t3dsize = 64;
	float4 *t3ddata = new float4[t3dsize*t3dsize*t3dsize];
	for(size_t x = 0; x < t3dsize; x++)
	{
		for(size_t y = 0; y < t3dsize; y++)
		{
			for(size_t z = 0; z < t3dsize; z++)
			{
				float s = t3dsize/2.0f;
				float sphere = (s-x)*(s-x)+(s-y)*(s-y)+(s-z)*(s-z);
				if(rand()%128 < 92 || sphere > s*s)
					t3ddata[x*t3dsize*t3dsize+y*t3dsize+z] = float4(0,0,0,0);
				else
				{
					float r = rand()%1000/1000.0f, g = rand()%1000/1000.0f, b = rand()%1000/1000.0f, a = (rand()%10<2)?(0.9f+rand()%11/30.0f):(0.2);
					t3ddata[x*t3dsize*t3dsize+y*t3dsize+z] = float4(r, g, b, a);
				}
			}
		}
	}


	D3D11_BUFFER_DESC t3dtexture;
	t3dtexture.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	t3dtexture.ByteWidth = t3dsize*t3dsize*t3dsize* sizeof(float4);
	t3dtexture.CPUAccessFlags = 0;
	t3dtexture.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	t3dtexture.StructureByteStride = sizeof(float4);
	t3dtexture.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA t3dsd;
	t3dsd.pSysMem =  t3ddata;
	t3dsd.SysMemPitch = 0;
	t3dsd.SysMemSlicePitch = 0;

	ID3D11Buffer* g_p3dTexBuffer = 0;
	g_pd3dDevice->CreateBuffer(&t3dtexture, &t3dsd, &g_p3dTexBuffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC t3dsrvd;
	t3dsrvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	t3dsrvd.Buffer.ElementOffset = 0;
	t3dsrvd.Buffer.ElementWidth = t3dsize*t3dsize*t3dsize;
	t3dsrvd.Format = DXGI_FORMAT_UNKNOWN;

	ID3D11ShaderResourceView* g_p3dTexBufferSRV = 0;
	g_pd3dDevice->CreateShaderResourceView(g_p3dTexBuffer, &t3dsrvd, &g_p3dTexBufferSRV);

	// TRIANGLE
	D3D11_BUFFER_DESC tbd;
	tbd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	tbd.ByteWidth = gScene.face_buffer.size() * sizeof(Face);
	tbd.CPUAccessFlags = 0;
	tbd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	tbd.StructureByteStride = sizeof(Face);
	tbd.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA tsd;
	tsd.pSysMem =  &gScene.face_buffer[0];
	tsd.SysMemPitch = 0;
	tsd.SysMemSlicePitch = 0;

	ID3D11Buffer* g_pTriangleBuffer = 0;
	g_pd3dDevice->CreateBuffer(&tbd, &tsd, &g_pTriangleBuffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC tsrvd;
	tsrvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	tsrvd.Buffer.ElementOffset = 0;
	tsrvd.Buffer.ElementWidth = gScene.face_buffer.size();
	tsrvd.Format = DXGI_FORMAT_UNKNOWN;

	ID3D11ShaderResourceView* g_pTriangleBufferSRV = 0;
	g_pd3dDevice->CreateShaderResourceView(g_pTriangleBuffer, &tsrvd, &g_pTriangleBufferSRV);


	// MATERIAL
	D3D11_BUFFER_DESC mbd;
	mbd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	mbd.ByteWidth = gScene.material_buffer.size() * sizeof(Material);
	mbd.CPUAccessFlags = 0;
	mbd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	mbd.StructureByteStride = sizeof(Material);
	mbd.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA msd;
	msd.pSysMem =  &gScene.material_buffer[0];
	msd.SysMemPitch = 0;
	msd.SysMemSlicePitch = 0;

	ID3D11Buffer* g_pMaterialBuffer = 0;
	g_pd3dDevice->CreateBuffer(&mbd, &msd, &g_pMaterialBuffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC msrvd;
	msrvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	msrvd.Buffer.ElementOffset = 0;
	msrvd.Buffer.ElementWidth = gScene.material_buffer.size();
	msrvd.Format = DXGI_FORMAT_UNKNOWN;

	ID3D11ShaderResourceView* g_pMaterialBufferSRV = 0;
	g_pd3dDevice->CreateShaderResourceView(g_pMaterialBuffer, &msrvd, &g_pMaterialBufferSRV);


	// BIH
	D3D11_BUFFER_DESC bihbd;
	bihbd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bihbd.ByteWidth = gScene.bih_buffer.size() * sizeof(BIHNode);
	bihbd.CPUAccessFlags = 0;
	bihbd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bihbd.StructureByteStride = sizeof(BIHNode);
	bihbd.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA bihsd;
	bihsd.pSysMem =  &gScene.bih_buffer[0];
	bihsd.SysMemPitch = 0;
	bihsd.SysMemSlicePitch = 0;

	ID3D11Buffer* g_pBIHBuffer = 0;
	g_pd3dDevice->CreateBuffer(&bihbd, &bihsd, &g_pBIHBuffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC bihsrvd;
	bihsrvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	bihsrvd.Buffer.ElementOffset = 0;
	bihsrvd.Buffer.ElementWidth = gScene.bih_buffer.size();
	bihsrvd.Format = DXGI_FORMAT_UNKNOWN;

	ID3D11ShaderResourceView* g_pBIHBufferSRV = 0;
	g_pd3dDevice->CreateShaderResourceView(g_pBIHBuffer, &bihsrvd, &g_pBIHBufferSRV);

	////// 



	/// LIGHTS //////////////////////////////////////////////////////////////////////

	Light light;
	if(bLoadSponza)
	{
		float maxL = 5.0f, r = 240.0f/maxL + 10.0f;
		int maxV = 500;
		/*for(float i = 0.0f; i < maxL; i+=1.0f)
		{
			light.pos = XMFLOAT3(-120.0f + 240.0, 0.0f, 40.0f);
			light.color = XMFLOAT3(rand()%maxV, rand()%maxV, rand()%maxV);
			light.radius = r;
			gScene.light_buffer.push_back(light);

			light.pos = XMFLOAT3(-120.0f + 240.0f/maxL*i, 60.5f, -40.0f);
			light.color = XMFLOAT3(rand()%maxV, rand()%maxV, rand()%maxV);
			light.radius = r;
			gScene.light_buffer.push_back(light);

			light.pos = XMFLOAT3(-120.0f + 240.0f/maxL*i, 10.5f, 40.0f);
			light.color = XMFLOAT3(rand()%maxV, rand()%maxV, rand()%maxV);
			light.radius = r;
			gScene.light_buffer.push_back(light);

			light.pos = XMFLOAT3(-120.0f + 240.0f/maxL*i, 10.5f, -40.0f);
			light.color = XMFLOAT3(rand()%maxV, rand()%maxV, rand()%maxV);
			light.radius = r;
			gScene.light_buffer.push_back(light);
		}*/
	}

	light.pos = XMFLOAT3(-30.0f, 45.5f, 0.0f);
	light.color = XMFLOAT3(500.5f, 300.1f, 100.7f);
	light.radius = 100.0f;
	//gScene.light_buffer.push_back(light);

	light.pos = XMFLOAT3(0.0f, 20.5f, 0.0f);
	light.color = XMFLOAT3(100.2f, 200.6f, 500.0f);
	light.radius = 100.0f;
	//gScene.light_buffer.push_back(light);

	light.pos = XMFLOAT3(30.0f, 50.5f, 0.0f);
	light.color = XMFLOAT3(3000.6f, 50.3f, 50.3f);
	light.radius = 100.0f;
	//gScene.light_buffer.push_back(light);

	if(!bLoadSponza)
	{
		light.pos = XMFLOAT3(3.0f, 3.3f, -2.0f);
		light.color = XMFLOAT3(1.5f, 3.0f, 1.6f);
		light.radius = 100.0f;
		//gScene.light_buffer.push_back(light);

		light.pos = XMFLOAT3(3.6f, 3.5f, 3.2f);
		light.color = XMFLOAT3(3.0f, 1.9f, 1.4f);
		light.radius = 100.0f;
		//gScene.light_buffer.push_back(light);

		light.pos = XMFLOAT3(-3.0f, -0.2f, 2.1f);
		light.color = XMFLOAT3(2.0f, 1.2f, 1.2f);
		light.radius = 100.0f;
		//gScene.light_buffer.push_back(light);
	}
	
	gScene.buildLightsBih();


	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.ByteWidth = gScene.light_buffer.size() * sizeof(Light);
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(Light);
	bd.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA sd;
	sd.pSysMem =  &gScene.light_buffer[0];
	sd.SysMemPitch = 0;
	sd.SysMemSlicePitch = 0;

	ID3D11Buffer* g_pLightBuffer = 0;
	g_pd3dDevice->CreateBuffer(&bd, &sd, &g_pLightBuffer);

	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	srvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvd.Buffer.ElementOffset = 0;
	srvd.Buffer.ElementWidth = gScene.light_buffer.size();
	srvd.Format = DXGI_FORMAT_UNKNOWN;

	ID3D11ShaderResourceView* g_pLightBufferSRV = 0;
	g_pd3dDevice->CreateShaderResourceView(g_pLightBuffer, &srvd, &g_pLightBufferSRV);


	// BIH
	bihbd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bihbd.ByteWidth = gScene.lightBih_buffer.size() * sizeof(BIHNode);
	bihbd.CPUAccessFlags = 0;
	bihbd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bihbd.StructureByteStride = sizeof(BIHNode);
	bihbd.Usage = D3D11_USAGE_IMMUTABLE;

	bihsd.pSysMem =  &gScene.lightBih_buffer[0];
	bihsd.SysMemPitch = 0;
	bihsd.SysMemSlicePitch = 0;

	ID3D11Buffer* g_pLightBIHBuffer = 0;
	g_pd3dDevice->CreateBuffer(&bihbd, &bihsd, &g_pLightBIHBuffer);

	bihsrvd.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	bihsrvd.Buffer.ElementOffset = 0;
	bihsrvd.Buffer.ElementWidth = gScene.lightBih_buffer.size();
	bihsrvd.Format = DXGI_FORMAT_UNKNOWN;

	ID3D11ShaderResourceView* g_pLightBIHBufferSRV = 0;
	g_pd3dDevice->CreateShaderResourceView(g_pLightBIHBuffer, &bihsrvd, &g_pLightBIHBufferSRV);






	D3D11_BUFFER_DESC cbd;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.ByteWidth = sizeof(CameraViewDesc);
	cbd.CPUAccessFlags = 0;
	cbd.MiscFlags = 0;
	cbd.StructureByteStride = 0;
	cbd.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Buffer* g_pCameraCBuffer = 0;
	g_pd3dDevice->CreateBuffer(&cbd, 0, &g_pCameraCBuffer);


	// OUTPUT INFO CBUFFER
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.ByteWidth = sizeof(OutputInfo);
	cbd.CPUAccessFlags = 0;
	cbd.MiscFlags = 0;
	cbd.StructureByteStride = 0;
	cbd.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Buffer* g_pOutputInfoCBuffer = 0;
	g_pd3dDevice->CreateBuffer(&cbd, 0, &g_pOutputInfoCBuffer);
	
	//set c-buffers
	g_pImmediateContext->CSSetConstantBuffers(0, 1, &g_pCameraCBuffer);
	g_pImmediateContext->CSSetConstantBuffers(1, 1, &g_pOutputInfoCBuffer);
	g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pOutputInfoCBuffer);

	// INPUT AND TIMER INIT
	gInput.init(g_Window.hInstance, g_Window.hWnd);
	gTimer.init();


	MSG msg;
	g_Done = false;
	g_Started = true;
	g_pOnWindowResize = Demo_OnWindowResize;

	gScene.cam.setPosition(XMVectorSet(25, 35, 25, 0));
	gScene.cam.lookAt(XMVectorSet(30, 30, 30, 0));


	D3D11_QUERY_DESC pQueryDesc;
	pQueryDesc.Query = D3D11_QUERY_EVENT;
	pQueryDesc.MiscFlags = 0;
	ID3D11Query *pEventQuery;
	g_pd3dDevice->CreateQuery(&pQueryDesc, &pEventQuery);

	LARGE_INTEGER start, stop, freq;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);
	stop = start;

	while (!g_Done)									
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				g_Done = true;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			start = stop;
			QueryPerformanceCounter(&stop);
			char str[512];
			sprintf(str, "Raytracer [dt: %.2fms] [NumSample: %.3f] [Cam pos: %.1f, %.1f, %.1f] [Cam dir: %.1f, %.1f, %.1f]", 1000.0f*(float)(stop.QuadPart-start.QuadPart)/(float)freq.QuadPart, g_numSample,
				gScene.cam.getPos().x, gScene.cam.getPos().y, gScene.cam.getPos().z, XMVectorGetX(gScene.cam.m_view_desc.dir), XMVectorGetY(gScene.cam.m_view_desc.dir), XMVectorGetZ(gScene.cam.m_view_desc.dir));
			SetWindowTextA(g_Window.hWnd, str);

			if (GetFocus() == g_Window.hWnd)
				update(g_Window.Width, g_Window.Height);

			//update camera c-buffer
			g_pImmediateContext->UpdateSubresource(g_pCameraCBuffer, 0, 0, &gScene.cam.m_view_desc, 0, 0);

			//update output info c-buffer
			OutputInfo outputInfo;
			outputInfo.res[0] = g_Window.Width;
			outputInfo.res[1] = g_Window.Height;
			outputInfo.aspectRatio = (float)g_Window.Width / (float)g_Window.Height;
			outputInfo.seedA = (1<<0)*(rand()%256) + (1<<8)*(rand()%256) + (1<<16)*(rand()%256) + (1<<24)*(rand()%256);
			outputInfo.seedB = (1<<0)*(rand()%256) + (1<<8)*(rand()%256) + (1<<16)*(rand()%256) + (1<<24)*(rand()%256);
			outputInfo.numSamples = g_numSample;
			g_pImmediateContext->UpdateSubresource(g_pOutputInfoCBuffer, 0, 0, &outputInfo, 0, 0);

			g_pImmediateContext->End(pEventQuery); // insert a fence into the pushbuffer
			while(g_pImmediateContext->GetData( pEventQuery, NULL, 0, 0 ) == S_FALSE ) {} // spin until event is finished

			//set resources
			// VERTEX, TRIANGLE, MATERIALS, LIGHTS BUFFERS AND BIH BUFFERS
			ID3D11ShaderResourceView *g_pSRViews[] = { g_pVertexBufferSRV, 
													   g_pTriangleBufferSRV,
													   g_pMaterialBufferSRV, 
													   g_pBIHBufferSRV, 
													   g_pLightBufferSRV,
													   g_pLightBIHBufferSRV,
													   g_pRandSeedBufferSRV,
													   g_p3dTexBufferSRV } ; 
			g_pImmediateContext->CSSetShaderResources(0, 8, g_pSRViews);

			//set uav-buffers
			ID3D11UnorderedAccessView * g_pUAViews[] = { g_pOutputUAV };
			g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, g_pUAViews, 0);

			//DISPATCH
			if(g_numSample < 1)
				g_pImmediateContext->Dispatch(g_Window.Width/(THREADS_X/2)  + 1, g_Window.Height/(THREADS_Y/2) + 1, 1);
			else
				g_pImmediateContext->Dispatch(g_Window.Width/THREADS_X + 1, g_Window.Height/THREADS_Y + 1, 1);
 
			ID3D11UnorderedAccessView* ppNullUAV[] = {0};
			g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, ppNullUAV, 0);
			
			//wait for compute shader for being executed
			g_pImmediateContext->End(pEventQuery); // insert a fence into the pushbuffer
			while(g_pImmediateContext->GetData( pEventQuery, NULL, 0, 0 ) == S_FALSE ) {} // spin until event is finished
			
			
			D3D11_VIEWPORT vp;
			vp.Width = g_Window.Width;
			vp.Height = g_Window.Height;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;
			g_pImmediateContext->RSSetViewports(1, &vp );

			g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, 0);
			g_pImmediateContext->PSSetShaderResources(0, 1, &g_pOutputSRV);
			g_pImmediateContext->Draw(6, 0);

			ID3D11ShaderResourceView* pSRV[] = {0};
			g_pImmediateContext->PSSetShaderResources(0, 1, pSRV);

			g_pSwapChain->Present(0, 0);
		}
	}

	SAFE_RELEASE(pEventQuery);

	SAFE_RELEASE(g_pVertexBufferSRV);
	SAFE_RELEASE(g_pVertexBuffer);
	SAFE_RELEASE(g_pTriangleBufferSRV);
	SAFE_RELEASE(g_pTriangleBuffer);
	SAFE_RELEASE(g_pMaterialBufferSRV);
	SAFE_RELEASE(g_pMaterialBuffer);
	SAFE_RELEASE(g_pBIHBufferSRV);
	SAFE_RELEASE(g_pBIHBuffer);

	SAFE_RELEASE(g_pVS);
	SAFE_RELEASE(g_pPS);
	SAFE_RELEASE(g_pCS);

	SAFE_RELEASE(g_pOutputSRV);
	SAFE_RELEASE(g_pOutputUAV);
	SAFE_RELEASE(g_pOutputBuffer);

	// Shutdown
	Demo_ReleaseDirect3D();
	Demo_DestroyWindow();

#ifdef _DEBUG
	_CrtDumpMemoryLeaks();
#endif

	return 0;
}
