#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "imgui.h"

class FontManager
{
public:
    static ImFont* LoadFont(std::string font, int size);
    static ImFont* GetFont(std::string font, int size, bool checkIfExists = true);
    static void LoadFonts(std::string font, std::vector<int> sizes);
    //static void UpdateFonts();
    static void InitFontManager();
};
