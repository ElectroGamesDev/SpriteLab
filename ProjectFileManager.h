#pragma once
#ifndef PROJECTFILEMANAGER_H
#define PROJECTFILEMANAGER_H
#include <windows.h>
#include <shobjidl.h>
#include <iostream>
#include "stb_image_write.h"
#include "SDL.h"
#include <filesystem>
#include <SDL_syswm.h>
#include "Project.h"

namespace SpriteLab
{
    enum FileType
    {
        ExportType,
        SaveType
    };

    struct UserSettings
    {
        SDL_Point preferedCanvasSize = {-1, -1};
        std::vector<std::string> recentProjects;
    }; extern UserSettings userSettings;

    void ExportProject(SDL_Renderer* renderer, Project* project, std::filesystem::path path, SDL_Point size);
    std::string SaveProjectDialog(SDL_Window* window, SDL_Renderer* renderer, FileType fileType, std::string fileName);
    std::string LoadProjectDialog(SDL_Window* window, SDL_Renderer* renderer);
    void SaveProject(Project* project, std::filesystem::path path);
    Project LoadProject(std::filesystem::path path);
    void SetSetting(std::string setting, std::string value, std::string value2 = "null");
    std::string GetSetting(std::string setting);
    void LoadSettings();
    void SaveSettings();
    void ProjectSaveAs(SDL_Window* window, SDL_Renderer* renderer, Project* project);
    void ProjectSave(SDL_Window* window, SDL_Renderer* renderer, Project* project);
    void ProjectExportAs(SDL_Window* window, SDL_Renderer* renderer, Project* project);
    void ProjectExport(SDL_Window* window, SDL_Renderer* renderer, Project* project);
}

#endif