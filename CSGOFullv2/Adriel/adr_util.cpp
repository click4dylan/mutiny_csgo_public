#include "adr_util.hpp"
#include "../buildserver_chars.h"
#include "../VMProtectDefs.h"

#include <random>
#include <locale>
#include <codecvt>
#include <iomanip>

int adr_util::random_int(int i_start, int i_end)
{
	//VMP_BEGINMUTILATION("util # random_int");
	std::random_device rd;
	std::mt19937 rng(rd());
	const std::uniform_int_distribution<int> uni(i_start, i_end);
	return static_cast<int>(uni(rng));
	//VMP_END;
}

float adr_util::random_float(float i_start, float i_end)
{
	//VMP_BEGINMUTILATION("util # random_float");
	std::random_device rd;
	std::mt19937 rng(rd());
	const std::uniform_real_distribution<float> uni(i_start, i_end);
	return static_cast<float>(uni(rng));
	//VMP_END;
}

Color adr_util::get_rainbow_color(float speed)
{
	//VMP_BEGINMUTILATION("util # get_rainbow_color");
	speed = 0.002f * speed;
	const auto now = get_epoch_time();
	const auto hue = (now % static_cast<int>(1.0f / speed)) * speed;
	return Color::FromHSB(hue, 1.0f, 1.0f);
	//VMP_END;
}

Color adr_util::get_health_color(int hp)
{
	return Color(min(510 * (100 - hp) / 100, 255), min(510 * hp / 100, 255), 25);
}

std::string adr_util::get_disk()
{
	//VMP_BEGINMUTILATION("util # get_disk");
	TCHAR wind_dir[MAX_PATH];
	GetWindowsDirectory(wind_dir, MAX_PATH);
	return std::string(wind_dir).erase(1, 800) + ":\\";

	//VMP_END;
}

std::string adr_util::get_time()
{
	//VMP_BEGINMUTILATION("util # get_time");
	time_t current_time;
	time(&current_time);

	struct tm time_info {};
	localtime_s(&time_info, &current_time);

	std::ostringstream oss;
	//decrypts(0)
	oss << std::put_time(&time_info, XorStr("%d/%m/%Y %H:%M:%S"));
	//encrypts(0)
	return oss.str();
	//VMP_END;
}

std::string adr_util::date_to_string(tm date)
{
	//VMP_BEGINMUTILATION("util # date_to_string");
	std::ostringstream oss;
	//decrypts(0)
	oss << std::put_time(&date, XorStr("%d/%m/%Y"));
	//encrypts(0)
	return oss.str();
	//VMP_END;
}

std::pair<int, int> adr_util::get_window_size(HWND h_wnd)
{
	RECT desktop;
	GetWindowRect(h_wnd, &desktop);
	return std::pair<int, int>(desktop.right, desktop.bottom);
}

time_t adr_util::get_epoch_time()
{
	//VMP_BEGINMUTILATION("util # get_epoch_time");
	const auto duration = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	//VMP_END;
}

bool adr_util::check_one_instance()
{
	const auto h_start_event = CreateEventW(nullptr, TRUE, FALSE, L"int");
	if (h_start_event == nullptr)
	{
		CloseHandle(h_start_event);
		return false;
	}

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(h_start_event);
		return false;
	}
	return true;
}

std::string adr_util::get_key_name_by_id(int id)
{
	//VMP_BEGINMUTILATION("util # get_key_name_by_id");

	//decrypts(0)
	static std::unordered_map< int, std::string > key_names =
	{
		{ 0, XorStr("None") },
		{ VK_LBUTTON, XorStr("Mouse 1") },
		{ VK_RBUTTON, XorStr("Mouse 2") },
		{ VK_MBUTTON, XorStr("Mouse 3") },
		{ VK_XBUTTON1, XorStr("Mouse 4") },
		{ VK_XBUTTON2, XorStr("Mouse 5") },
		{ VK_BACK, XorStr("Back") },
		{ VK_TAB, XorStr("Tab") },
		{ VK_CLEAR, XorStr("Clear") },
		{ VK_RETURN, XorStr("Enter") },
		{ VK_SHIFT, XorStr("Shift") },
		{ VK_CONTROL, XorStr("Ctrl") },
		{ VK_MENU, XorStr("Alt") },
		{ VK_PAUSE, XorStr("Pause") },
		{ VK_CAPITAL, XorStr("Caps Lock") },
		{ VK_ESCAPE, XorStr("Escape") },
		{ VK_SPACE, XorStr("Space") },
		{ VK_PRIOR, XorStr("Page Up") },
		{ VK_NEXT, XorStr("Page Down") },
		{ VK_END, XorStr("End") },
		{ VK_HOME, XorStr("Home") },
		{ VK_LEFT, XorStr("Left Key") },
		{ VK_UP, XorStr("Up Key") },
		{ VK_RIGHT, XorStr("Right Key") },
		{ VK_DOWN, XorStr("Down Key") },
		{ VK_SELECT, XorStr("Select") },
		{ VK_PRINT, XorStr("Print Screen") },
		{ VK_INSERT, XorStr("Insert") },
		{ VK_DELETE, XorStr("Delete") },
		{ VK_HELP, XorStr("Help") },
		{ VK_SLEEP, XorStr("Sleep") },
		{ VK_MULTIPLY, XorStr("*") },
		{ VK_ADD, XorStr("+") },
		{ VK_SUBTRACT, XorStr("-") },
		{ VK_DECIMAL, XorStr(".") },
		{ VK_DIVIDE, XorStr("/") },
		{ VK_NUMLOCK, XorStr("Num Lock") },
		{ VK_SCROLL, XorStr("Scroll") },
		{ VK_LSHIFT, XorStr("Left Shift") },
		{ VK_RSHIFT, XorStr("Right Shift") },
		{ VK_LCONTROL, XorStr("Left Ctrl") },
		{ VK_RCONTROL, XorStr("Right Ctrl") },
		{ VK_LMENU, XorStr("Left Alt") },
		{ VK_RMENU, XorStr("Right Alt") },
	};
	//encrypts(0)

	// check for 0-9, A-Z
	if (id >= 0x30 && id <= 0x5A)
	{
		return std::string(1, (char)id);
	}
	// check for numpad
	else if (id >= 0x60 && id <= 0x69)
	{
		//decrypts(0)
		std::string ret = XorStr("Num ") + std::to_string(id - 0x60);
		//encrypts(0)
		return ret;
	}
	// check for function keys
	else if (id >= 0x70 && id <= 0x87)
	{
		return std::string(std::to_string((id - 0x70) + 1));
	}

	return key_names[id];
	//VMP_END;
}

bool adr_util::string::is_number(const std::string& s)
{
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !isdigit(c); }) == s.end();
}

void adr_util::string::split(const std::string& s, char delim, std::vector<std::string>& elems)
{
	std::stringstream ss;
	ss.str(s);
	std::string item;

	while (getline(ss, item, delim))
		if (!item.empty())
			elems.push_back(item);
}

std::vector<std::string> adr_util::string::split(const std::string& s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, elems);

	return elems;
}

bool adr_util::string::contains(const std::string& word, const std::string& sentence)
{
	if (word == "" || sentence == "")
		return true;

	return sentence.find(word) != std::string::npos;
}

std::string adr_util::string::replace(std::string text, const std::string& find, const std::string& replace)
{
	std::string::size_type found;
	do
	{
		found = text.find(find);
		if (found != std::string::npos)
		{
			text.erase(found, find.length());
			text.insert(found, replace);
		}
	} while (found != std::string::npos);

	return text;
}

std::string adr_util::string::pad_right(std::string text, size_t value)
{
	text.insert(text.length(), value - text.length(), ' ');
	return text;
}

std::string adr_util::string::wstring_to_string(std::wstring wstr)
{
	if (wstr.empty())
        return std::string();

#ifdef WIN32
	const auto size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wstr[0], wstr.size(), nullptr, 0, nullptr, nullptr);
	auto ret = std::string(size, 0);
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, &wstr[0], wstr.size(), &ret[0], size, nullptr, nullptr);
#else
    size_t size = 0;
    _locale_t lc = _create_locale(LC_ALL, "en_US.UTF-8");
    errno_t err = _wcstombs_s_l(&size, NULL, 0, &wstr[0], _TRUNCATE, lc);
    std::string ret = std::string(size, 0);
    err = _wcstombs_s_l(&size, &ret[0], size, &wstr[0], _TRUNCATE, lc);
    _free_locale(lc);
    ret.resize(size - 1);
#endif

    return ret;
}

std::wstring adr_util::string::string_to_wstring(std::string str)
{
	if (str.empty())
        return std::wstring();

	const auto len = str.length() + 1;
    auto ret = std::wstring(len, 0);

#ifdef WIN32
	const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, &str[0], str.size(), &ret[0], len);
    ret.resize(size);
#else
    size_t size = 0;
    _locale_t lc = _create_locale(LC_ALL, "en_US.UTF-8");
    errno_t retval = _mbstowcs_s_l(&size, &ret[0], len, &str[0], _TRUNCATE, lc);
    _free_locale(lc);
    ret.resize(size - 1);
#endif

	return ret;
}

std::string adr_util::string::random(int size)
{
	//VMP_BEGINMUTILATION("string # random");

	//decrypts(0)
	static auto& chrs = XorStr("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
	//encrypts(0)

	thread_local static std::mt19937 rg{ std::random_device{}() };
	thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

	std::string str;
	str.reserve(size);

	while (size--)
		str += chrs[pick(rg)];

	return str;
	//VMP_END;
}

std::string adr_util::string::to_lower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](char c) {return static_cast<char>(::tolower(c)); });
	return str;
}

std::string adr_util::string::to_upper(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](char c) {return static_cast<char>(::toupper(c)); });
	return str;
}

std::string adr_util::get_weapon_name(ItemDefinitionIndex index)
{
	//decrypts(0)
	static std::unordered_map<int, std::string> weapon_names = 
	{
		{ 0, XorStr("Default")},
		{ WEAPON_DEAGLE, XorStr("Deagle")},
		{ WEAPON_ELITE, XorStr("DBerettas")},
		{ WEAPON_FIVESEVEN, XorStr("Five-SeveN")},
		{ WEAPON_GLOCK, XorStr("Glock-18")},
		{ WEAPON_AK47, XorStr("AK-47")},
		{ WEAPON_AUG, XorStr("AUG")},
		{ WEAPON_AWP, XorStr("AWP")},
		{ WEAPON_FAMAS, XorStr("FAMAS")},
		{ WEAPON_G3SG1, XorStr("G3SG1")},
		{ WEAPON_GALILAR, XorStr("Galil")},
		{ WEAPON_M249, XorStr("M249")},
		{ WEAPON_M4A1, XorStr("M4A4")},
		{ WEAPON_MAC10, XorStr("MAC-10")},
		{ WEAPON_P90, XorStr("P90")},
		{ WEAPON_MP5SD, XorStr("MP5-SD")},
		{ WEAPON_UMP45, XorStr("UMP-45")},
		{ WEAPON_XM1014, XorStr("XM1014")},
		{ WEAPON_BIZON, XorStr("PP-Bizon")},
		{ WEAPON_MAG7, XorStr("MAG-7")},
		{ WEAPON_NEGEV, XorStr("Negev")},
		{ WEAPON_SAWEDOFF, XorStr("Sawed-Off")},
		{ WEAPON_TEC9, XorStr("Tec-9")},
		{ WEAPON_TASER, XorStr("Taser")},
		{ WEAPON_HKP2000, XorStr("P2000")},
		{ WEAPON_MP7, XorStr("MP7")},
		{ WEAPON_MP9, XorStr("MP9")},
		{ WEAPON_NOVA, XorStr("Nova")},
		{ WEAPON_P250, XorStr("P250")},
		{ WEAPON_SCAR20, XorStr("SCAR-20")},
		{ WEAPON_SG556, XorStr("SG-553")},
		{ WEAPON_SSG08, XorStr("SSG-08")},
		{ WEAPON_KNIFE, XorStr("Knife")},
		{ WEAPON_FLASHBANG, XorStr("Flashbang")},
		{ WEAPON_HEGRENADE, XorStr("Grenade")},
		{ WEAPON_SMOKEGRENADE, XorStr("Smoke")},
		{ WEAPON_MOLOTOV, XorStr("Molotov")},
		{ WEAPON_INCGRENADE, XorStr("Incendiary")},
		{ WEAPON_C4, XorStr("C4")},
		{ WEAPON_KNIFE_T, XorStr("Knife")},
		{ WEAPON_USP_SILENCER, XorStr("USP-S")},
		{ WEAPON_CZ75A, XorStr("CZ75-A")},
		{ WEAPON_REVOLVER, XorStr("R8")},
	};
	//encrypts(0)

	if (weapon_names.find(index) == weapon_names.end())
		return weapon_names[0];

	return weapon_names[index];
}

bool adr_util::is_knife(ItemDefinitionIndex index)
{
	return index >= WEAPON_KNIFE_BAYONET && index < GLOVE_STUDDED_BLOODHOUND || index == WEAPON_KNIFE_T || index == WEAPON_KNIFE || index == WEAPON_KNIFEGG || index == WEAPON_FISTS || index == WEAPON_KNIFE_GHOST || index == WEAPON_SPANNER || index == WEAPON_HAMMER || index == WEAPON_AXE || index == WEAPON_MELEE;
}

bool adr_util::is_glove(ItemDefinitionIndex index)
{
	return index >= GLOVE_STUDDED_BLOODHOUND && index <= GLOVE_HYDRA;
}

bool adr_util::is_rifle(ItemDefinitionIndex index)
{
	switch (index)
	{
	case WEAPON_FAMAS:
	case WEAPON_GALILAR:
	case WEAPON_M4A1:
	case WEAPON_M4A1_SILENCER:
	case WEAPON_AK47:
	case WEAPON_AUG:
	case WEAPON_SG556:
		return true;
	default:
		return false;
	}
}

bool adr_util::is_smg(ItemDefinitionIndex index)
{
	switch (index)
	{
	case WEAPON_MAC10:
	case WEAPON_MP9:
	case WEAPON_MP7:
	case WEAPON_P90:
	case WEAPON_BIZON:
	case WEAPON_MP5SD:
		return true;
	default:
		return false;
	}
}

bool adr_util::is_shotgun(ItemDefinitionIndex index)
{
	switch (index)
	{
	case WEAPON_XM1014:
	case WEAPON_NOVA:
	case WEAPON_SAWEDOFF:
	case WEAPON_MAG7:
		return true;
	default:
		return false;
	}
}

bool adr_util::is_sniper(ItemDefinitionIndex index)
{
	switch (index)
	{
	case WEAPON_AWP:
	case WEAPON_SSG08:
	case WEAPON_SCAR20:
	case WEAPON_G3SG1:
		return true;
	default:
		return false;
	}
}

bool adr_util::is_pistol(ItemDefinitionIndex index)
{
	switch (index)
	{
	case WEAPON_HKP2000:
	case WEAPON_USP_SILENCER:
	case WEAPON_P250:
	case WEAPON_ELITE:
	case WEAPON_REVOLVER:
	case WEAPON_DEAGLE:
	case WEAPON_FIVESEVEN:
	case WEAPON_CZ75A:
		return true;
	default:
		return false;
	}
}

bool adr_util::is_heavy(ItemDefinitionIndex index)
{
	switch (index)
	{
	case WEAPON_M249:
	case WEAPON_NEGEV:
		return true;
	default:
		return false;
	}
}

bool adr_util::is_utility(ItemDefinitionIndex index)
{
	switch (index)
	{
	case WEAPON_FLASHBANG:
	case WEAPON_HEGRENADE:
	case WEAPON_INCGRENADE:
	case WEAPON_MOLOTOV:
	case WEAPON_SMOKEGRENADE:
	case WEAPON_DECOY:
	case WEAPON_FRAG_GRENADE:
	case WEAPON_FIREBOMB:
	case WEAPON_TAGRENADE:
	case WEAPON_BREACHCHARGE:
	case WEAPON_C4:
	case WEAPON_TASER:
	case WEAPON_DIVERSION:
	case WEAPON_TABLET:
	case WEAPON_HEALTHSHOT:
		return true;
	default:
		return false;
	}
}

std::string adr_util::sanitize_name(char* name)
{
	char buf[128];
	auto c = 0;
	for (auto i = 0; name[i]; ++i)
	{
		if (c >= sizeof(buf) - 1)
			break;

		switch (name[i])
		{
			case '"':
			case '\\':
			case ';':
			case '\n':
			case '%':
				break;
			default:
			buf[c++] = name[i];
		}
	}
	buf[c] = '\0';

	std::string tmp(buf);
	if (tmp.length() > 20)
	{
		tmp.erase(20, (tmp.length() - 20));
		//decrypts(0)
		tmp.append(XorStr("..."));
		//encrypts(0)
	}

	return tmp;
}

//std::vector< adr_util::weapon_name_t > adr_util::aim_weapon_names =
//{
//	{ WEAPON_NONE, ("Default") },
//	{ WEAPON_AK47, ("AK-47") },
//	{ WEAPON_AUG, ("AUG") },
//	{ WEAPON_AWP, ("AWP") },
//	{ WEAPON_CZ75A, ("CZ75 Auto") },
//	{ WEAPON_DEAGLE, ("Desert Eagle") },
//	{ WEAPON_ELITE, ("Dual Berettas") },
//	{ WEAPON_FAMAS, ("FAMAS") },
//	{ WEAPON_FIVESEVEN, ("Five-SeveN") },
//	{ WEAPON_G3SG1, ("G3SG1") },
//	{ WEAPON_GALILAR, ("Galil AR") },
//	{ WEAPON_GLOCK, ("Glock-18") },
//	{ WEAPON_M249, ("M249") },
//	{ WEAPON_M4A1_SILENCER, ("M4A1-S") },
//	{ WEAPON_M4A1, ("M4A4") },
//	{ WEAPON_MAC10, ("MAC-10") },
//	{ WEAPON_MAG7, ("MAG-7") },
//	{ WEAPON_MP5SD, ("MP5-SD") },
//	{ WEAPON_MP7, ("MP7") },
//	{ WEAPON_MP9, ("MP9") },
//	{ WEAPON_NEGEV, ("Negev") },
//	{ WEAPON_NOVA, ("Nova") },
//	{ WEAPON_HKP2000, ("P2000") },
//	{ WEAPON_P250, ("P250") },
//	{ WEAPON_P90, ("P90") },
//	{ WEAPON_BIZON, ("PP-Bizon") },
//	{ WEAPON_REVOLVER, ("R8 Revolver") },
//	{ WEAPON_SAWEDOFF, ("Sawed-Off") },
//	{ WEAPON_SCAR20, ("SCAR-20") },
//	{ WEAPON_SSG08, ("SSG 08") },
//	{ WEAPON_SG556, ("SG 556") },
//	{ WEAPON_TEC9, ("Tec-9") },
//	{ WEAPON_UMP45, ("UMP-45") },
//	{ WEAPON_USP_SILENCER, ("USP-S") },
//	{ WEAPON_XM1014, ("XM1014") },
//};
