#pragma once
#include "../VA_FileSystem.h"

class Menu
{
	VA_FileSystem* fs;

	static std::string inputString();

public:
	void ShowFS();
	void FormatFS();
	void Copy();
	void Move();
	void Delete();

	Menu();
	~Menu();
};
