#include "Adriel/config.hpp"
#include "Adriel/adr_util.hpp"
#include "LocalPlayer.h"
#include "UsedConvars.h"
#include "Events.h"
#include "CreateMove.h"
#include "Eventlog.h"

// TODO: nit; add in money check/algorithm for legit players
// TODO: nit; don't buy things that we already own (ie: weapon if currentweapon is same, armor if armorvalue >= 100, frags if frag count == 1, kit if already owned)
void autobuy_logic()
{
	auto& var = variable::get();

	if (!var.misc.b_autobuy)
		return;

	LocalPlayer.Get(&LocalPlayer);

	std::stringstream cmd;

	//decrypts(0)
	static std::unordered_map< int, std::string > primaries = {
		{ 0,  XorStr("buy a 15;") },		// ak47		| m4a4, m4a1-s
		{ 1,  XorStr("buy a 14;") },		// galil	| famas
		{ 2,  XorStr("buy a 17;") },		// sg553	| aug
		{ 3,  XorStr("buy a 19;") },		// g3gs1	| scar20
		{ 4,  XorStr("buy a 8;") },		// mac10	| mp9
		{ 5,  XorStr("buy a 22;") },		// sawedoff | mag7
		{ 6,  XorStr("buy a 9;") },		// mp7, mp5sd
		{ 7,  XorStr("buy awp;") },		// awp
		{ 8,  XorStr("buy ssg08;") },		// scout
		{ 9,  XorStr("buy ump45;") },		// ump45
		{ 10, XorStr("buy bizon;") },		// bizon
		{ 11, XorStr("buy p90;") },		// p90
		{ 12, XorStr("buy nova;") },		// nova
		{ 13, XorStr("buy xm1014;") },	// xm1014
		{ 14, XorStr("buy m249;") },		// m249
		{ 15, XorStr("buy negev;") },		// negev
	};
	//encrypts(0)

	//decrypts(0)
	static std::unordered_map< int, std::string > secondaries = {
		{ 0, XorStr("buy a 5;") },			// tec9 | five-seven, cz-75
		{ 1, XorStr("buy a 6;") },			// r8	| deagle
		{ 2, XorStr("buy p250;") },		// p250
		{ 3, XorStr("buy elite;") },		// dual berettas
	};
	//encrypts(0)

	const size_t primary_t = var.misc.i_autobuy_t_primary;
	const size_t primary_ct = var.misc.i_autobuy_ct_primary;
	const size_t secondary_t = var.misc.i_autobuy_t_secondary;
	const size_t secondary_ct = var.misc.i_autobuy_ct_secondary;

	// buy primaries and secondaries
	if (LocalPlayer.Entity && LocalPlayer.Entity->GetTeam() == TEAM_T)
	{
		if (primary_t >= 1 && primary_t <= primaries.size())
			cmd << primaries[primary_t - 1].data();
		if (secondary_t >= 1 && secondary_t <= secondaries.size())
			cmd << secondaries[secondary_t - 1].data();
	}
	else if (LocalPlayer.Entity && LocalPlayer.Entity->GetTeam() == TEAM_CT)
	{
		if (primary_ct >= 1 && primary_ct <= primaries.size())
			cmd << primaries[primary_ct - 1].data();
		if (secondary_ct >= 1 && secondary_ct <= secondaries.size())
			cmd << secondaries[secondary_ct - 1].data();
	}

	// buy nades
	if (var.misc.b_autobuy_frag)
	{
		//decrypts(0)
		cmd << XorStr("buy hegrenade;");
		//encrypts(0)
	}
	if (var.misc.b_autobuy_smoke)
	{
		//decrypts(0)
		cmd << XorStr("buy smokegrenade;");
		//encrypts(0)
	}
	if (var.misc.b_autobuy_flash)
	{
		//decrypts(0)
		cmd << XorStr("buy flashbang;");
		//encrypts(0)
	}
	if (var.misc.b_autobuy_fire)
	{
		//decrypts(0)
		cmd << XorStr("buy a 26;");
		//encrypts(0)
	}
	if (var.misc.b_autobuy_decoy)
	{
		//decrypts(0)
		cmd << XorStr("buy decoy;");
		//encrypts(0)
	}

	// buy armor
	if (var.misc.b_autobuy_armor)
	{
		// buy kevlar and helmet and kevlar if needed without an if check

		//decrypts(0)
		cmd << XorStr("buy vesthelm;buy vest;");
		//encrypts(0)
	}

	// buy zeus
	if (var.misc.b_autobuy_zeus)
	{
		//decrypts(0)
		cmd << XorStr("buy a 34;");
		//encrypts(0)
	}

	// buy kit if we don't already have one
	if (var.misc.b_autobuy_kit && LocalPlayer.Entity && !LocalPlayer.Entity->HasDefuseKit())
	{
		//decrypts(0)
		cmd << XorStr("buy defuser;");
		//encrypts(0)
	}

	Interfaces::EngineClient->ExecuteClientCmd(cmd.str().data());
}