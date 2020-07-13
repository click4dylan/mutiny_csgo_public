#include "precompiled.h"
#include "threadtools.h"
#include "utlvectorsimple.h"
#include "IModelInfoClient.h"
#include "VTHook.h"

int CStudioHdr::GetNumSeq(void) const
{
	if (m_pVModel == NULL)
	{
		return _m_pStudioHdr->numlocalseq;
	}

	return m_pVModel->m_seq.Count();
}

int CStudioHdr::GetActivityListVersion()
{
	return StaticOffsets.GetOffsetValueByType<int(__thiscall*)(CStudioHdr*)>(_GetActivityListVersion)(this);
}

const studiohdr_t* virtualgroup_t::GetStudioHdr(void) const
{
	return Interfaces::MDLCache->GetStudioHdr((MDLHandle_t)cache);
}

bool GetModelKeyValue(const model_t *model, std::string &buf)
{
	if (!model || model->type != mod_studio)
		return false;

	studiohdr_t* pStudioHdr = Interfaces::MDLCache->GetStudioHdr(model->studio);
	if (!pStudioHdr)
		return false;

	if (pStudioHdr->numincludemodels == 0)
	{
		buf += pStudioHdr->KeyValueText();
		return true;
	}

	virtualmodel_t *pVM = Interfaces::ModelInfoClient->GetVirtualModel(pStudioHdr);

	if (pVM)
	{
		for (int i = 0; i < pVM->m_group.Count(); i++)
		{
			virtualgroup_t* vG = (virtualgroup_t*)pVM->m_group.Retrieve(i, 0x90);
			const studiohdr_t* pSubStudioHdr = vG->GetStudioHdr();
			if (pSubStudioHdr && pSubStudioHdr->KeyValueText())
			{
				buf += pSubStudioHdr->KeyValueText();
			}
		}
	}
	return true;
}

mstudioattachment_t& CStudioHdr::pAttachment(int i)
{
	CStudioHdr *hdr; // edi
	virtualmodel_t *vmodel; // ecx
	const studiohdr_t *pStudioHdr; // eax

	hdr = this;
	vmodel = this->m_pVModel;
	if (!vmodel)
		return *(mstudioattachment_t *)((char *)hdr->_m_pStudioHdr + ((92 * i) + hdr->_m_pStudioHdr->localattachmentindex));
	pStudioHdr = (const studiohdr_t *)GroupStudioHdr(*(DWORD *)(vmodel->m_attachment.memory + 8 * i));
	//return *(mstudioattachment_t *)((char *)pStudioHdr
	//	+ 92 * *(DWORD *)(hdr->m_pVModel->m_attachment.memory + 8 * i + 4)
	//	+ pStudioHdr->localattachmentindex);

	DWORD v11 = 92 * *(DWORD *)(vmodel->m_attachment.memory + 8 * i + 4);
	DWORD v10 = pStudioHdr->localattachmentindex + v11;
	return *(mstudioattachment_t*)((char*)pStudioHdr + v10);
}

int CStudioHdr::GetNumAttachments()
{
	virtualmodel_t *v1; // eax
	int result; // eax

	v1 = this->m_pVModel;
	if (v1)
		result = v1->m_attachment.count;
	else
		result = this->_m_pStudioHdr->numlocalattachments;
	return result;
}

bool CStudioHdr::SequencesAvailable()
{
	//56  8B  F1  8B  16  83  BA  ??  ??  ??  ??  ??  74  3B  83  7E  04  00  75  35  83  BA  ??  ??  ??  ??  ??  75  10  33  C0  50  E8  ??  ??  ??  ??  F7  D8  5E  1B  C0  F7  D8  C3  8B  0D  ??  ??  ??  ??  52  8B  01  FF  50  68  
	int v29;
	DWORD hdr = (DWORD)this;
	if (!hdr
		|| *(DWORD*)hdr == 0
		|| *(DWORD*)(*(DWORD*)hdr + 0x150)
		&& !*(DWORD*)(hdr + 4)
		&& (*(DWORD*)(*(DWORD*)hdr + 0x150) ? (v29 = (*(int(__thiscall*)(DWORD, DWORD))(*(DWORD*)SequencesAvailableVMT + 0x68))(*(DWORD*)SequencesAvailableVMT, *(DWORD*)hdr)) : (v29 = 0),
			SequencesAvailableCall((studiohdr_t*)hdr, v29) == 0))
	{
		return false;
	}
	return true;
}

const studiohdr_t* CStudioHdr::GroupStudioHdr(DWORD group)
{
	static GroupStudioHdrFn oGroupStudioHdr = StaticOffsets.GetOffsetValueByType<GroupStudioHdrFn>(_GroupStudioHdr);
	return oGroupStudioHdr(this, group);
}

int CStudioHdr::GetAttachmentBone(int i)
{
	virtualmodel_t *vmodel = m_pVModel;
	int iBone;

	if (vmodel)
	{
		DWORD v14 = vmodel->m_group.memory + 144 * *(DWORD *)(vmodel->m_attachment.memory + 8 * i);
		iBone = *(DWORD *)(*(DWORD *)(v14 + 24) + 4 * pAttachment(i).localbone);
		if (iBone == -1)
			iBone = 0;
	}
	else
	{
		iBone = *(int *)((char *)&_m_pStudioHdr->checksum + (92 * i) + _m_pStudioHdr->localattachmentindex);
	}
	return iBone;
}

int	CStudioHdr::GetNumPoseParameters(void) const
{
	if (m_pVModel == NULL)
	{
		return _m_pStudioHdr->numlocalposeparameters;
	}

	//Assert(m_pVModel);

	return m_pVModel->m_pose.Count();
}

mstudioseqdesc_t &CStudioHdr::pSeqdesc(int seq)
{
	return *opSeqdesc((studiohdr_t*)this, seq);
}

const mstudioposeparamdesc_t & CStudioHdr::pPoseParameter(int i)
{
	if (m_pVModel == NULL)
	{
		return *_m_pStudioHdr->pLocalPoseParameter(i);
	}

	auto pose = (virtualgeneric_t*)m_pVModel->m_pose.Retrieve(i, sizeof(virtualgeneric_t));

	if (pose->group == 0)
		return *_m_pStudioHdr->pLocalPoseParameter(pose->index);

	const studiohdr_t* pStudioHdr = GroupStudioHdr(pose->group);

	return *pStudioHdr->pLocalPoseParameter(pose->index);
}