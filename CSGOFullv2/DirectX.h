#pragma once
#include "Overlay.h"
#include <d3d9.h>

typedef long(__stdcall* ResetFn)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
typedef long(__stdcall* EndSceneFn)(IDirect3DDevice9*);

extern HRESULT(WINAPI* oReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
extern HRESULT(WINAPI* oEndScene)(IDirect3DDevice9*);

HRESULT WINAPI Hooked_Reset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters);
HRESULT WINAPI Hooked_EndScene(IDirect3DDevice9* pDevice);