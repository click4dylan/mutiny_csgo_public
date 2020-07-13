#pragma once

#ifdef VMPROTECT
//#include "C:\DeveloperFrameworkOLD\VMProtect\VMProtectSDK.h"
#include "C:\Developer\Sync\Framework\VMProtect\VMProtectSDK.h"
#define VMP_BEGIN(marker) VMProtectBegin(marker);
#define VMP_BEGINVIRTUALIZATION(marker) VMProtectBeginVirtualization(marker);
#define VMP_BEGINMUTILATION(marker) VMProtectBeginMutation(marker);
#define VMP_BEGINULTRA(marker) VMProtectBeginUltra(marker);
#define VMP_BEGINVIRTUALIZATIONLOCKBYKEY(marker) VMProtectBeginVirtualizationLockByKey(marker);
#define VMP_BEGINULTRALOCKBYKEY(marker) VMProtectBeginUltraLockByKey(marker);
#define VMP_END

#define VMP_ISPROTECTED VMProtectIsProtected()
#define VMP_ISDEBUGGERPRESENT(bcheckkernel) VMProtectIsDebuggerPresent(bcheckkernel)
#define VMP_ISVIRTUALMACHINEPRESENT() VMProtectIsVirtualMachinePresent()
#define VMP_ISVALIDIMAGECRC() VMProtectIsValidImageCRC()
#define VMP_ENCDECSTRINGA(value) VMProtectDecryptStringA(value)
#define VMP_ENCDECSTRINGW(value) VMProtectDecryptStringW(value)
#define VMP_FREESTRING(value) VMProtectFreeString(value)
#else
#define VMP_BEGIN(marker)
#define VMP_BEGINVIRTUALIZATION(marker)
#define VMP_BEGINMUTILATION(marker)
#define VMP_BEGINULTRA(marker)
#define VMP_BEGINVIRTUALIZATIONLOCKBYKEY(marker)
#define VMP_BEGINULTRALOCKBYKEY(marker)
#define VMP_END

#define VMP_ISPROTECTED false
#define VMP_ISDEBUGGERPRESENT(bcheckkernel) false
#define VMP_ISVIRTUALMACHINEPRESENT() false
#define VMP_ISVALIDIMAGECRC true
#define VMP_ENCDECSTRINGA(value) value
#define VMP_ENCDECSTRINGW(value) value
#define VMP_FREESTRING(value) true
#endif