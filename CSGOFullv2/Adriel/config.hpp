#pragma once
#include "stdafx.hpp"

class config_item
{
public:
	std::string str_name {};
	std::string str_catg {};

	std::type_index ti_type = std::type_index(typeid(int));
	void* p_option;

	config_item(const std::string& catg, const std::string& name, std::type_index type, void* pointer)
	{
		ti_type = type;
		str_name = name;
		str_catg = catg;
		p_option = pointer;
	}
};

class config : public singleton<config>
{
private:
	std::string str_file_end{};
	std::string str_dir{};

	std::vector<config_item> item{};
	config_item* find(std::string str_catg, std::string str_name);

	template<typename T>
	void setup_var(T* ptr, T val, std::string catg, std::string name)
	{
		item.push_back(config_item(catg, name, std::type_index(typeid(T)), ptr));
		*ptr = val;
	}

public:
	config();
	~config();

	void initialize();
	void refresh();

	bool create(std::string str_cfg);
	bool load(std::string str_cfg);
	bool save(std::string str_cfg);

	bool remove(std::string str_cfg) const;
	std::vector<std::string> get_configs() const;
	const std::string get_config_directory() const;
};