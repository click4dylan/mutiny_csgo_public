#pragma once

class CTempEnts
{
public:
	virtual void Func1();
	virtual void Func2();
	virtual void Func3();
	virtual void Func4();
	virtual void Update();
};

extern CTempEnts* tempents;

using TempEntsUpdateFn = void(__thiscall*)(CTempEnts*);
extern TempEntsUpdateFn oTempEntsUpdate;