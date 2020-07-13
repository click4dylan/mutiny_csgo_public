#pragma once

class CSplitScreen {
public:
	virtual bool Init() = 0;
	virtual void Shutdown(void) = 0;
	virtual bool AddBaseUser(int) = 0;
	virtual bool AddSplitScreenUser(unsigned, int) = 0;
	virtual bool RemoveSplitScreenUser(unsigned, int) = 0;
	virtual int GetActiveSplitScreenPlayerSlot() = 0;
	virtual int SetActiveSplitScreenPlayerSlot() = 0;
	virtual int GetNumSplitScreenPlayers() = 0;
	virtual int GetSplitScreenPlayerEntity(int) = 0;
	virtual int GetSplitScreenPlayerNetChan(int) = 0;
	virtual bool IsValidSplitScreenSlot(int) = 0;
	virtual int FirstValidSplitScreenSlot() = 0;
	virtual int NextValidSplitScreenSlot(unsigned) = 0;
	virtual bool IsDisconnecting(int) = 0;
	virtual void SetDisconnecting(int, bool) = 0;
	virtual bool SetLocalPlayerIsResolvable(char const*, int, bool) = 0;
	virtual bool IsLocalPlayerResolvable() = 0;
};