#pragma once

#include "Common/d3dApp.h"
#include "pcH.h"
#include <DirectXColors.h>

class Source : public D3DApp
{
public:
	Source(HINSTANCE hInstance);
	~Source();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	//Enable run-time memory check for debug builds
#if defined(DEBUG)| defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	try
	{
		Source App(hInstance);
		if(!App.Initialize())
		{
			return 0;
		}
		return App.Run();
	}
	catch(DxException &e)
	{
		MessageBox(nullptr, reinterpret_cast<LPCSTR>(e.ToString().c_str()), "HR FAILED", MB_OK);
		return 0;
	}
}

Source::Source(HINSTANCE hInstance) : D3DApp(hInstance)
{
}

Source::~Source()
= default;

bool Source::Initialize()
{
	return D3DApp::Initialize();
}

void Source::OnResize()
{
	D3DApp::OnResize();
}

void Source::Update(const GameTimer& gt)
{
	
}


void Source::Draw(const GameTimer& gt)
{
	//Reuse memory associated with command recording
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	//Indicate a state transition on the resource usage
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//Set the viewport and scissor rect. Needs to be reset when command list is reset
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	//clear back buffer and depth buffer
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::Black, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//specify the buffers to render to
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	// Indicate a state transition on the resource usage
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//done recording commands
	ThrowIfFailed(mCommandList->Close());

	//add the command list to queue
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	// swap back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	//inefficient waiting for the next frame
	FlushCommandQueue();
}


