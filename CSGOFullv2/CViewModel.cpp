#include "CViewModel.h"
#include "BaseEntity.h"
#include "VTHook.h"
#include "Interfaces.h"
#include "NetworkedVariables.h"

void C_BaseViewModel::UpdateAllViewmodelAddons()
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(C_BaseViewModel*)>(_UpdateViewModelAddonsSub)(this);
	StaticOffsets.GetVFuncByType<void(__thiscall*)(C_BaseViewModel*, int)>(_UpdateViewModelAddonsVT, this)(this, 32);
}

void C_BaseViewModel::RemoveViewmodelArmModels()
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(C_BaseViewModel*)>(_RemoveViewModelArmModels)(this);
}

void C_BaseViewModel::RemoveViewmodelLabel()
{
	CBaseHandle m_hLabel = (CBaseHandle)*StaticOffsets.GetOffsetValueByType<DWORD*>(_ViewModelLabelHandle, this);
	IHandleEntity *m_pLabel = m_hLabel.Get();
	if (m_pLabel)
		CBaseEntity::Remove(m_pLabel);
}

void C_BaseViewModel::RemoveViewmodelStatTrak()
{
	CBaseHandle m_hLabel = (CBaseHandle)*StaticOffsets.GetOffsetValueByType<DWORD*>(_ViewModelStatTrackHandle, this);
	IHandleEntity *m_pLabel = m_hLabel.Get();
	if (m_pLabel)
		CBaseEntity::Remove(m_pLabel);
}

void C_BaseViewModel::RemoveViewmodelStickers()
{
	StaticOffsets.GetOffsetValueByType<void(__thiscall*)(C_BaseViewModel*)>(_RemoveViewModelStickers)(this);
}