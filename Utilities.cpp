#include "Utilities.h"
#include <string>
#include <map>

namespace SpriteLab
{
    std::string SpriteLab::ToolToString(Tools tool) {
        static const std::map<Tools, std::string> toolToString = {
            {PaintBrush, "PaintBrush"},
            {PaintBucket, "PaintBucket"},
            {Eraser, "Eraser"},
            {EyeDropper, "EyeDropper"},
            {Line, "Line"}
        };

        auto it = toolToString.find(tool);
        return (it != toolToString.end()) ? it->second : "Unknown";
    }

    Tools SpriteLab::StringToTool(const std::string& str) {
        static const std::map<std::string, Tools> stringToTool = {
            {"PaintBrush", PaintBrush},
            {"PaintBucket", PaintBucket},
            {"Eraser", Eraser},
            {"EyeDropper", EyeDropper},
            {"Line", Line}
        };

        auto it = stringToTool.find(str);
        return (it != stringToTool.end()) ? it->second : PaintBrush;
    }

    bool SpriteLab::CompareColor(SDL_Color color1, SDL_Color color2, bool ignoreAlpha)
    {
        if (ignoreAlpha) return (color1.r == color2.r && color1.g == color2.g && color1.b == color2.b);
        else return (color1.r == color2.r && color1.g == color2.g && color1.b == color2.b && color1.a == color2.a);
        return false;
    }

    void SpriteLab::SetBrushColour(Project* project, Brush* brush, SDL_Color colour)
    {
        if (colour.a < 10) colour.a = 10;
        if (CompareColor(colour, brush->colour, true))
        {
            if (colour.a != brush->colour.a) brush->colour.a = colour.a;
            return;
        }
        project->recentColours.erase(
            std::remove_if(project->recentColours.begin(), project->recentColours.end(),
                [&](const SDL_Color& recentColour) {
                    return CompareColor(colour, recentColour, true);
                }),
            project->recentColours.end());
        if (project->recentColours.size() >= 6)
        {
            for (int i = 0; i < 5; i++)
                project->recentColours[i + 1] = project->recentColours[i];
            project->recentColours[0] = colour;
        }
        else project->recentColours.insert(project->recentColours.begin(), brush->colour);
        brush->colour = colour;
    }

    void SpriteLab::SaveProjectPopup()
    {
        int result = MessageBox(nullptr, savePopupMessage, L"SpriteLab", MB_YESNOCANCEL | MB_ICONWARNING | MB_SYSTEMMODAL);

        switch (result) {
        case IDYES:
            savePopupButtonClicked = "Yes";
            break;
        case IDNO:
            savePopupButtonClicked = "No";
            break;
        case IDCANCEL:
            savePopupButtonClicked = "Cancel";
            break;
        }
    }
}