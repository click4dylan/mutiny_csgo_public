#include "precompiled.h"
#include "VTHook.h"
#include "Interfaces.h"
#include "Globals.h"
#include "LocalPlayer.h"
#include "Adriel/ui.hpp"
#include "Adriel/console.hpp"

UnlockCursorFn oUnlockCursor;
LockCursorFn oLockCursor;

void __stdcall Hooks::LockCursor()
{
	/*if( ui::get().is_visible() ) {

		using func_t = void ( __thiscall * )(void*);
		GetVFunc< func_t > ( Interfaces::Surface, 66 )( Interfaces::Surface);
		Interfaces::InputSystem->EnableInput( false );
		return;
	}

	oLockCursor(Interfaces::Surface);
	Interfaces::InputSystem->EnableInput( true );*/

	//decrypts(0)
	static auto b_once = (logger::add(LWARN, XorStr("Lock Cursor Hooked at: 0x%.8X"), oLockCursor), true);
	//encrypts(0)
	
	if (ui::get().is_visible())
	{
		//SDK::Interfaces::MatSurface()->UnlockCursor();

		using func_t = void ( __thiscall * )(void*);
		GetVFunc< func_t > ( Interfaces::Surface, 66 )( Interfaces::Surface);

		return;
	}

	oLockCursor(Interfaces::Surface);
}

void __stdcall Hooks::UnlockCursor()
{
	oUnlockCursor(Interfaces::Surface);
}