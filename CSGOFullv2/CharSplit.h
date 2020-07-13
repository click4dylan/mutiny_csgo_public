#ifndef charsplit_h
#define charsplit_h

#include <vector> //Required by SplitChar3

//Noel's String Explode

class StringList{
	std::vector<const char*> strings;
public:
	StringList(int Length, int initialmemreserve);
	~StringList();

	inline unsigned StringList::GetSize(){
		return strings.size();
	}

	inline void StringList::Add(const char* str){ strings.push_back(str); }

	inline void StringList::AddAt(unsigned pos, const char * str){ strings.insert(strings.begin() + pos, str); }

	/*
	inline char *& StringList::operator[](int pos){
	return strings.at(pos);
	}
	*/
	inline void StringList::Remove(){ strings.pop_back(); return; }

	inline void StringList::RemoveAt(unsigned pos){ strings.erase(strings.begin() + pos); }

	inline const char * StringList::Get(unsigned pos) { return strings.at(pos); }

	inline void StringList::Set(unsigned pos, const char * replacement){ strings.at(pos) = replacement; }
	/*inline unsigned int GetSize();
	inline void Add(const char * str); // Add A String To The End Of The String List
	//char *& operator[](int pos); // testing on this later
	inline void AddAt(unsigned pos, const char * str); // Add A String And Store It At A Current Position In The String List
	inline void Remove(); // Remove A String From The End Of The String List And Returns The String
	inline void RemoveAt(unsigned pos); // Remove A String At A Current Position In The String List
	inline const char * Get(unsigned pos); // Get A String At A Current Position In The String List
	inline void Set(unsigned pos, const char * replacement); // Set A String At A Current Position In The String List With A Replacement Value*/
};

StringList * SplitChar(const char * text, char delimiters[]);

#endif