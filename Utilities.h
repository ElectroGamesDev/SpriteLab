#pragma once
#ifndef UTILITIES_H
#define UTILITIES_H

#include <windows.h>
#include <shobjidl.h> 
#include "SDL.h"
#include "Project.h"
#include <string>

namespace SpriteLab
{
    static LPCWSTR savePopupMessage = L"";
    static std::string savePopupButtonClicked = "";

    std::string ToolToString(Tools tool);

    Tools StringToTool(const std::string& str);

    bool CompareColor(SDL_Color color1, SDL_Color color2, bool ignoreAlpha = false);

    void SetBrushColour(Project* project, Brush* brush, SDL_Color colour);

    void SaveProjectPopup();
}

#endif