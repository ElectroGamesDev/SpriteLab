#pragma once
#ifndef PROJECT_H
#define PROJECT_H

#include <imgui.h>
#include <vector>
#include "Layer.h"
#include <string>

namespace SpriteLab
{
    struct ProjectSettings
    {
        ImVec2 size;
        float zoom = 1;
    };

    struct Brush
    {
        SDL_Color colour = { 0,0,0,255 };
        int size = 1;
    };

    enum Tools
    {
        PaintBrush,
        PaintBucket,
        Eraser,
        EyeDropper,
        Line
    };

    struct Project
    {
        std::string name = "";

        Tools selectedTool = PaintBrush;
        std::string selectedToolString = "PaintBrush";
        Brush brush;

        ProjectSettings projectSettings;

        bool bestZoomSet = false;
        bool saved = true;

        ImVec2 canvasMinPos;
        ImVec2 canvasMaxPos;

        std::vector<SDL_Color> recentColours;
        std::vector<Layer> layers;

        Layer* selectedLayer;

        std::string lastSaveLocation = "";
        std::string lastExportLocation = "";
        std::string lastSaveName = "";
        std::string lastExportName = "";


        bool Compare(Project other)
        {
            return (name == other.name && lastSaveLocation == other.lastSaveLocation);
        }

        float GetBestCanvasZoom()
        {
            int x = floor(ImGui::GetWindowSize().x / projectSettings.size.x);
            int y = floor(ImGui::GetWindowSize().y / projectSettings.size.y);
            return (x < y) ? x : y;
        }
    };
}

#endif