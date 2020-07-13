#pragma once

class Vector;
// CPhysConvex is a single convex solid
class CPhysConvex;
// CPhysPolysoup is an abstract triangle soup mesh
class CPhysPolysoup;
class ICollisionQuery;
class IVPhysicsKeyParser;
struct convertconvexparams_t;
class CPackedPhysicsDescription;
class ISave;
class IRestore;


class IPhysicsObject;
class IPhysicsEnvironment;
class IPhysicsSurfaceProps;
class IPhysicsConstraint;
class IPhysicsConstraintGroup;
class IPhysicsFluidController;
class IPhysicsSpring;
class IPhysicsVehicleController;
class IConvexInfo;
class IPhysicsObjectPairHash;
class IPhysicsCollisionSet;
class IPhysicsPlayerController;
class IPhysicsFrictionSnapshot;

struct Ray_t;
struct constraint_ragdollparams_t;
struct constraint_hingeparams_t;
struct constraint_fixedparams_t;
struct constraint_ballsocketparams_t;
struct constraint_slidingparams_t;
struct constraint_pulleyparams_t;
struct constraint_lengthparams_t;
struct constraint_groupparams_t;

struct vehicleparams_t;
struct matrix3x4_t;

struct fluidparams_t;
struct springparams_t;
struct objectparams_t;
struct debugcollide_t;
class CGameTrace;
typedef CGameTrace trace_t;
struct physics_stats_t;
struct physics_performanceparams_t;
struct virtualmeshparams_t;

//enum PhysInterfaceId_t;
struct physsaveparams_t;
struct physrestoreparams_t;
struct physprerestoreparams_t;

class CPolyhedron;
class CPhysCollide;
struct vcollide_t;
class QAngle;
class truncatedcone_t;

enum PhysInterfaceId_t
{
	PIID_UNKNOWN,
	PIID_IPHYSICSOBJECT,
	PIID_IPHYSICSFLUIDCONTROLLER,
	PIID_IPHYSICSSPRING,
	PIID_IPHYSICSCONSTRAINTGROUP,
	PIID_IPHYSICSCONSTRAINT,
	PIID_IPHYSICSSHADOWCONTROLLER,
	PIID_IPHYSICSPLAYERCONTROLLER,
	PIID_IPHYSICSMOTIONCONTROLLER,
	PIID_IPHYSICSVEHICLECONTROLLER,
	PIID_IPHYSICSGAMETRACE,

	PIID_NUM_TYPES
};

struct vcollide_t
{
	unsigned short solidCount : 15;
	unsigned short isPacked : 1;
	unsigned short descSize;
	// VPhysicsSolids
	CPhysCollide	**solids;
	char			*pKeyValues;
	void			*pUserData;
};

class IPhysicsCollision
{
public:
	virtual ~IPhysicsCollision(void) {}

	// produce a convex element from verts (convex hull around verts)
	virtual CPhysConvex		*ConvexFromVerts(Vector **pVerts, int vertCount) = 0;
	// produce a convex element from planes (csg of planes)
	virtual CPhysConvex		*ConvexFromPlanes(float *pPlanes, int planeCount, float mergeDistance) = 0;
	// calculate volume of a convex element
	virtual float			ConvexVolume(CPhysConvex *pConvex) = 0;

	virtual float			ConvexSurfaceArea(CPhysConvex *pConvex) = 0;
	// store game-specific data in a convex solid
	virtual void			SetConvexGameData(CPhysConvex *pConvex, unsigned int gameData) = 0;
	// If not converted, free the convex elements with this call
	virtual void			ConvexFree(CPhysConvex *pConvex) = 0;
	virtual CPhysConvex		*BBoxToConvex(const Vector &mins, const Vector &maxs) = 0;
	// produce a convex element from a convex polyhedron
	virtual CPhysConvex		*ConvexFromConvexPolyhedron(const CPolyhedron &ConvexPolyhedron) = 0;
	// produce a set of convex triangles from a convex polygon, normal is assumed to be on the side with forward point ordering, which should be clockwise, output will need to be able to hold exactly (iPointCount-2) convexes
	virtual void			ConvexesFromConvexPolygon(const Vector &vPolyNormal, const Vector *pPoints, int iPointCount, CPhysConvex **pOutput) = 0;

	// concave objects
	// create a triangle soup
	virtual CPhysPolysoup	*PolysoupCreate(void) = 0;
	// destroy the container and memory
	virtual void			PolysoupDestroy(CPhysPolysoup *pSoup) = 0;
	// add a triangle to the soup
	virtual void			PolysoupAddTriangle(CPhysPolysoup *pSoup, const Vector &a, const Vector &b, const Vector &c, int materialIndex7bits) = 0;
	// convert the convex into a compiled collision model
	virtual CPhysCollide *ConvertPolysoupToCollide(CPhysPolysoup *pSoup, bool useMOPP) = 0;

	// Convert an array of convex elements to a compiled collision model (this deletes the convex elements)
	virtual CPhysCollide	*ConvertConvexToCollide(CPhysConvex **pConvex, int convexCount) = 0;
	virtual CPhysCollide	*ConvertConvexToCollideParams(CPhysConvex **pConvex, int convexCount, const convertconvexparams_t &convertParams) = 0;
	// Free a collide that was created with ConvertConvexToCollide()
	virtual void			DestroyCollide(CPhysCollide *pCollide) = 0;

	// Get the memory size in bytes of the collision model for serialization
	virtual int				CollideSize(CPhysCollide *pCollide) = 0;
	// serialize the collide to a block of memory
	virtual int				CollideWrite(char *pDest, CPhysCollide *pCollide, bool bSwap = false) = 0;
	// unserialize the collide from a block of memory
	virtual CPhysCollide	*UnserializeCollide(char *pBuffer, int size, int index) = 0;

	// compute the volume of a collide
	virtual float			CollideVolume(CPhysCollide *pCollide) = 0;
	// compute surface area for tools
	virtual float			CollideSurfaceArea(CPhysCollide *pCollide) = 0;

	// Get the support map for a collide in the given direction
	virtual Vector			CollideGetExtent(const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, const Vector &direction) = 0;

	// Get an AABB for an oriented collision model
	virtual void			CollideGetAABB(Vector *pMins, Vector *pMaxs, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles) = 0;

	virtual void			CollideGetMassCenter(CPhysCollide *pCollide, Vector *pOutMassCenter) = 0;
	virtual void			CollideSetMassCenter(CPhysCollide *pCollide, const Vector &massCenter) = 0;
	// get the approximate cross-sectional area projected orthographically on the bbox of the collide
	// NOTE: These are fractional areas - unitless.  Basically this is the fraction of the OBB on each axis that
	// would be visible if the object were rendered orthographically.
	// NOTE: This has been precomputed when the collide was built or this function will return 1,1,1
	virtual Vector			CollideGetOrthographicAreas(const CPhysCollide *pCollide) = 0;
	virtual void			CollideSetOrthographicAreas(CPhysCollide *pCollide, const Vector &areas) = 0;

	// query the vcollide index in the physics model for the instance
	virtual int				CollideIndex(const CPhysCollide *pCollide) = 0;

	// Convert a bbox to a collide
	virtual CPhysCollide	*BBoxToCollide(const Vector &mins, const Vector &maxs) = 0;
	virtual int				GetConvexesUsedInCollideable(const CPhysCollide *pCollideable, CPhysConvex **pOutputArray, int iOutputArrayLimit) = 0;


	// Trace an AABB against a collide
	virtual void TraceBox(const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr) = 0;
	virtual void TraceBox(const Ray_t &ray, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr) = 0;
	virtual void TraceBox(const Ray_t &ray, unsigned int contentsMask, IConvexInfo *pConvexInfo, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr) = 0;

	// Trace one collide against another
	virtual void TraceCollide(const Vector &start, const Vector &end, const CPhysCollide *pSweepCollide, const QAngle &sweepAngles, const CPhysCollide *pCollide, const Vector &collideOrigin, const QAngle &collideAngles, trace_t *ptr) = 0;

	// relatively slow test for box vs. truncated cone
	virtual bool			IsBoxIntersectingCone(const Vector &boxAbsMins, const Vector &boxAbsMaxs, const truncatedcone_t &cone) = 0;

	// loads a set of solids into a vcollide_t
	virtual void			VCollideLoad(vcollide_t *pOutput, int solidCount, const char *pBuffer, int size, bool swap = false) = 0;
	// destroyts the set of solids created by VCollideLoad
	virtual void			VCollideUnload(vcollide_t *pVCollide) = 0;

	// begins parsing a vcollide.  NOTE: This keeps pointers to the text
	// If you free the text and call members of IVPhysicsKeyParser, it will crash
	virtual IVPhysicsKeyParser	*VPhysicsKeyParserCreate(const char *pKeyData) = 0;
	// Free the parser created by VPhysicsKeyParserCreate
	virtual void			VPhysicsKeyParserDestroy(IVPhysicsKeyParser *pParser) = 0;

	// creates a list of verts from a collision mesh
	virtual int				CreateDebugMesh(CPhysCollide const *pCollisionModel, Vector **outVerts) = 0;
	// destroy the list of verts created by CreateDebugMesh
	virtual void			DestroyDebugMesh(int vertCount, Vector *outVerts) = 0;

	// create a queryable version of the collision model
	virtual ICollisionQuery *CreateQueryModel(CPhysCollide *pCollide) = 0;
	// destroy the queryable version
	virtual void			DestroyQueryModel(ICollisionQuery *pQuery) = 0;

	virtual IPhysicsCollision *ThreadContextCreate(void) = 0;
	virtual void			ThreadContextDestroy(IPhysicsCollision *pThreadContex) = 0;

	virtual CPhysCollide	*CreateVirtualMesh(const virtualmeshparams_t &params) = 0;
	virtual bool			SupportsVirtualMesh() = 0;


	virtual bool			GetBBoxCacheSize(int *pCachedSize, int *pCachedCount) = 0;


	// extracts a polyhedron that defines a CPhysConvex's shape
	virtual CPolyhedron		*PolyhedronFromConvex(CPhysConvex * const pConvex, bool bUseTempPolyhedron) = 0;

	// dumps info about the collide to Msg()
	virtual void			OutputDebugInfo(const CPhysCollide *pCollide) = 0;
	virtual unsigned int	ReadStat(int statID) = 0;
};