#pragma once

struct text_animation
{
	unsigned int current_frame;
	std::vector<std::string> frames {};

	text_animation()
	{
		current_frame = 0;
		frames = std::vector<std::string>();
	}

	explicit text_animation(std::vector<std::string> vec_frames)
	{
		current_frame = 0;
		frames = vec_frames;
	}

	std::string get_current_frame()
	{
		return frames[current_frame];
	}

	void next_frame()
	{
		current_frame++;
		if (current_frame >= frames.size())
			current_frame = 0;
	}
};

class clantag_changer : public singleton<clantag_changer>
{
private:
	std::string str_old_text = "129037hfsjiadnf9812wedcazsaxc";
	int i_old_anim = -1;
	float m_next_clantag_time = FLT_MIN;
	bool b_set = false;
	bool b_was_on = false;

private:
	text_animation animation;
	void set(const char* str);

	text_animation marquee(std::string text, int width) const;
	static text_animation words(std::string text);
	static text_animation letters(std::string text);

public:
	clantag_changer();
	~clantag_changer();

	void create_move();
	void clear();
	void reset() { b_set = true; }
};