#include "FontManager.h"
#include <vector>
#include <iostream>
#include <unordered_map> 
#include <utility>

static ImFont* defaultFont;
//static std::vector<std::pair<std::string, int>> unloadedFonts;
static std::unordered_map<std::string, std::unordered_map<int, ImFont*>> fonts;
//static bool updateFonts = false;

void FontManager::InitFontManager()
{
	defaultFont = ImGui::GetIO().Fonts->AddFontFromFileTTF("resources/fonts/Familiar-Pro-Bold.ttf", 16);
}

ImFont* FontManager::LoadFont(std::string font, int size)
{
	fonts[font][size] = ImGui::GetIO().Fonts->AddFontFromFileTTF(("resources/fonts/" + font +".ttf").c_str(), size);
	//unloadedFonts.push_back(std::make_pair(font, size));
	//updateFonts = true;
	//return defaultFont; // Returns the default font since the new font can't be used until the next frame
	return fonts[font][size];
}

void FontManager::LoadFonts(std::string font, std::vector<int> sizes)
{
	for (auto& size : sizes) LoadFont(font, size);
}

ImFont* FontManager::GetFont(std::string font, int size, bool checkIfExists)
{
	if (!checkIfExists)
		return fonts[font][size];

	auto fontIter = fonts.find(font);
	if (fontIter != fonts.end()) {
		auto& sizeMap = fontIter->second;
		auto sizeIter = sizeMap.find(size);

		if (sizeIter != sizeMap.end())
			return sizeIter->second;
	}

	return LoadFont(font, size);
}

//void FontManager::UpdateFonts()
//{
//	if (!updateFonts) return;
//	updateFonts = false;
//	ImGuiIO& io = ImGui::GetIO();
//
//	for (auto& font : unloadedFonts)
//	{
//		fonts[font.first][font.second] = io.Fonts->AddFontFromFileTTF(("resources/fonts/" + font.first + ".ttf").c_str(), font.second);
//	}
//	unloadedFonts.clear();
//}