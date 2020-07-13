#include "precompiled.h"
#include "CharSplit.h"

StringList::StringList(int Length = 0, int initialmemreserve = 5){
	strings.resize(Length);
	strings.reserve(initialmemreserve);
}

StringList::~StringList(){
	for (int c = strings.size()-1; c >= 0; c--){
		delete strings.at(c);
	}
}


StringList * SplitChar(const char * text, char delimiters[]){
	int len = strlen(text) + 1;
	char * editabletext = new char[len]; // For Const Char Strings Passed To This Function
	memcpy(editabletext, text, len);
	StringList * sl = new StringList();
	char * token = strtok(editabletext, delimiters);
	while (token != nullptr){
		int len2 = strlen(token) + 1;
		char * curstr = new char[len2];
		memcpy(curstr, token, len2);
		sl->Add(curstr);
		token = strtok(nullptr, delimiters);
	}
	delete[] editabletext;
	return sl;
}