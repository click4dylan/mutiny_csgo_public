#pragma once
#include "IClientRenderable.h"
#include "IModelInfoClient.h"

enum RenderableModelType_t
{
	RENDERABLE_MODEL_UNKNOWN_TYPE = -1,
	RENDERABLE_MODEL_ENTITY = 0,
	RENDERABLE_MODEL_STUDIOMDL,
	RENDERABLE_MODEL_STATIC_PROP,
	RENDERABLE_MODEL_BRUSH,
};

typedef unsigned short ClientLeafShadowHandle_t;
enum { CLIENT_LEAF_SHADOW_INVALID_HANDLE = (ClientLeafShadowHandle_t)~0 };

struct RenderableInstance_t
{
	uint8_t m_nAlpha;
};

enum RenderGroup_t
{
	RENDER_GROUP_OPAQUE = 0,
	RENDER_GROUP_TRANSLUCENT,
	RENDER_GROUP_TRANSLUCENT_IGNOREZ,
	RENDER_GROUP_COUNT, // Indicates the groups above are real and used for bucketing a scene
};

struct DistanceFadeInfo_t
{
	float m_flMaxDistSqr;    // distance at which everything is faded out
	float m_flMinDistSqr;    // distance at which everything is unfaded
	float m_flFalloffFactor; // 1.0f / ( maxDistSqr - MinDistSqr )
							// opacity = ( maxDist - distSqr ) * falloffFactor
};

struct CClientRenderablesList
{
	struct CEntry
	{
		IClientRenderable* m_pRenderable;
		unsigned short m_iWorldListInfoLeaf; // NOTE: this indexes WorldListInfo_t's leaf list.
		RenderableInstance_t m_InstanceData;
		uint8_t m_nModelType : 7; // See RenderableModelType_t
		uint8_t m_TwoPass : 1;
		int pad;
	}; // size 0x000C bytes

	enum
	{
		MAX_GROUP_ENTITIES = 4096,
		MAX_BONE_SETUP_DEPENDENCY = 64,
	};

	void* vtable;
	int m_iRefs;

	// The leaves for the entries are in the order of the leaves you call CollateRenderablesInLeaf in.
	DistanceFadeInfo_t m_DetailFade;
	CEntry m_RenderGroups[RENDER_GROUP_COUNT][MAX_GROUP_ENTITIES];
	int m_RenderGroupCounts[RENDER_GROUP_COUNT];
	int m_nBoneSetupDependencyCount;
	IClientRenderable* m_pBoneSetupDependency[MAX_BONE_SETUP_DEPENDENCY];
};

class CClientLeafSubSystemData
{
public:
	virtual ~CClientLeafSubSystemData(void)
	{
	}
};

typedef unsigned short LeafIndex_t;
enum
{
	INVALID_LEAF_INDEX = (LeafIndex_t)~0
};

struct WorldListLeafData_t
{
	LeafIndex_t	leafIndex;	// 16 bits
	__int16	waterData;
	__int16 firstTranslucentSurface;	// engine-internal list index
	__int16	translucentSurfaceCount;	// count of translucent surfaces+disps
};

struct WorldListInfo_t
{
	int		m_ViewFogVolume;
	int		m_LeafCount;
	bool	m_bHasWater;
	WorldListLeafData_t* m_pLeafDataList;
};

struct SetupRenderInfo_t
{
	WorldListInfo_t* m_pWorldListInfo;
	CClientRenderablesList* m_pRenderList;
	Vector m_vecRenderOrigin;
	Vector m_vecRenderForward;
	int m_nRenderFrame;
	int m_nDetailBuildFrame;	// The "render frame" for detail objects
	float m_flRenderDistSq;
	int m_nViewID;
	bool m_bDrawDetailObjects : 1;
	bool m_bDrawTranslucentObjects : 1;

	SetupRenderInfo_t()
	{
		m_bDrawDetailObjects = true;
		m_bDrawTranslucentObjects = true;
	}
};

class IClientLeafSystem
{
public:
	virtual void CreateRenderableHandle(IClientRenderable* pRenderable, bool bRenderWithViewModels, RenderableTranslucencyType_t nType, RenderableModelType_t nModelType, UINT32 nSplitscreenEnabled = 0xFFFFFFFF) = 0; // = RENDERABLE_MODEL_UNKNOWN_TYPE ) = 0;
	virtual void RemoveRenderable(ClientRenderHandle_t handle) = 0;
	virtual void AddRenderableToLeaves(ClientRenderHandle_t renderable, int nLeafCount, unsigned short* pLeaves) = 0;
	virtual void SetTranslucencyType(ClientRenderHandle_t handle, RenderableTranslucencyType_t nType) = 0;

	/*RenderInFastReflections(unsigned short, bool)
	DisableShadowDepthRendering(unsigned short, bool)
	DisableCSMRendering(unsigned short, bool)*/
	virtual void pad0() = 0;
	virtual void pad1() = 0;
	virtual void pad2() = 0;

	virtual void AddRenderable(IClientRenderable* pRenderable, bool IsStaticProp, RenderableTranslucencyType_t Type, RenderableModelType_t nModelType, UINT32 nSplitscreenEnabled = 0xFFFFFFFF) = 0; //7
	virtual bool IsRenderableInPVS(IClientRenderable* pRenderable) = 0; //8
	virtual void SetSubSystemDataInLeaf(int leaf, int nSubSystemIdx, CClientLeafSubSystemData* pData) = 0;
	virtual CClientLeafSubSystemData* GetSubSystemDataInLeaf(int leaf, int nSubSystemIdx) = 0;
	virtual void SetDetailObjectsInLeaf(int leaf, int firstDetailObject, int detailObjectCount) = 0;
	virtual void GetDetailObjectsInLeaf(int leaf, int& firstDetailObject, int& detailObjectCount) = 0;
	virtual void DrawDetailObjectsInLeaf(int leaf, int frameNumber, int& nFirstDetailObject, int& nDetailObjectCount) = 0;
	virtual bool ShouldDrawDetailObjectsInLeaf(int leaf, int frameNumber) = 0;
	virtual void RenderableChanged(ClientRenderHandle_t handle) = 0;
	virtual void BuildRenderablesList(const SetupRenderInfo_t& info) = 0;
	virtual void CollateViewModelRenderables(void*) = 0;
	virtual void DrawStaticProps(bool enable) = 0;
	virtual void DrawSmallEntities(bool enable) = 0;
	virtual ClientLeafShadowHandle_t AddShadow(ClientShadowHandle_t userId, unsigned short flags) = 0;
	virtual void RemoveShadow(ClientLeafShadowHandle_t h) = 0;
	virtual void ProjectShadow(ClientLeafShadowHandle_t handle, int nLeafCount, const int* pLeafList) = 0;
	virtual void ProjectFlashlight(ClientLeafShadowHandle_t handle, int nLeafCount, const int* pLeafList) = 0;
};