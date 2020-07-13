#include "CPhysicsEnvironment.h"
#include "VTHook.h"

CPhysicsEnvironment *physenv()
{
	return *StaticOffsets.GetOffsetValueByType<CPhysicsEnvironment**>(_physenv);
}