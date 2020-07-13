#pragma once

class CBeams
{
public:
	virtual void Func1();
	virtual void Func2();
	virtual	void Func3();
	virtual void UpdateTempEntBeams();
};

extern CBeams *beams;
using UpdateTempEntBeamsFn = void(__thiscall*)(CBeams*);
extern UpdateTempEntBeamsFn oUpdateTempEntBeams;