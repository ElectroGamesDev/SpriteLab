#include "ProjectFileManager.h"
#include <algorithm>
#include "json.hpp"
#include <fstream>

using json = nlohmann::json;

namespace SpriteLab
{
    UserSettings userSettings;

    inline std::string encode(std::string const& data) {
        int counter = 0;
        uint32_t bit_stream = 0;
        const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        std::string encoded;
        int offset = 0;
        for (unsigned char c : data) {
            auto num_val = static_cast<unsigned int>(c);
            offset = 16 - counter % 3 * 8;
            bit_stream += num_val << offset;
            if (offset == 16) {
                encoded += base64_chars.at(bit_stream >> 18 & 0x3f);
            }
            if (offset == 8) {
                encoded += base64_chars.at(bit_stream >> 12 & 0x3f);
            }
            if (offset == 0 && counter != 3) {
                encoded += base64_chars.at(bit_stream >> 6 & 0x3f);
                encoded += base64_chars.at(bit_stream & 0x3f);
                bit_stream = 0;
            }
            counter++;
        }
        if (offset == 16) {
            encoded += base64_chars.at(bit_stream >> 12 & 0x3f);
            encoded += "==";
        }
        if (offset == 8) {
            encoded += base64_chars.at(bit_stream >> 6 & 0x3f);
            encoded += '=';
        }
        return encoded;
    }

    inline std::string decode(std::string const& data) {
        int counter = 0;
        uint32_t bit_stream = 0;
        std::string decoded;
        int offset = 0;
        const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789+/";
        for (unsigned char c : data) {
            auto num_val = base64_chars.find(c);
            if (num_val != std::string::npos) {
                offset = 18 - counter % 4 * 6;
                bit_stream += num_val << offset;
                if (offset == 12) {
                    decoded += static_cast<char>(bit_stream >> 16 & 0xff);
                }
                if (offset == 6) {
                    decoded += static_cast<char>(bit_stream >> 8 & 0xff);
                }
                if (offset == 0 && counter != 4) {
                    decoded += static_cast<char>(bit_stream & 0xff);
                    bit_stream = 0;
                }
            }
            else if (c != '=') {
                return std::string();
            }
            counter++;
        }
        return decoded;
    }

    void SaveProject(Project* project, std::filesystem::path path)
    {
        userSettings.recentProjects.erase(std::remove(userSettings.recentProjects.begin(), userSettings.recentProjects.end(), path.string()), userSettings.recentProjects.end());
        userSettings.recentProjects.insert(userSettings.recentProjects.begin(), path.string());
        SaveSettings();

        json projectJson;
        json serializedLayers;
        for (const Layer& layer : project->layers)
        {
            json serializedLayer;
            serializedLayer["Pixels"] = {};

            for (const auto& pixelEntry : layer.pixels)
            {
                const std::pair<int, int>& coords = pixelEntry.first;
                const Pixel& pixel = pixelEntry.second;

                json serializedPixel =
                {
                    {"rect", {pixel.rect.x, pixel.rect.y, pixel.rect.w, pixel.rect.h}},
                    {"relativePos", {pixel.relativePos.x, pixel.relativePos.y}},
                    {"colour", {pixel.colour.r, pixel.colour.g, pixel.colour.b, pixel.colour.a}},
                    {"exists", pixel.exists}
                };

                serializedLayer["Pixels"][std::to_string(coords.first) + "-" + std::to_string(coords.second)] = serializedPixel;
            }
            serializedLayer["Visible"] = layer.visible;
            serializedLayers.push_back(serializedLayer);
        }

        projectJson["Settings"] =
        {
            {"Size", {project->projectSettings.size.x, project->projectSettings.size.y}},
            {"Zoom", project->projectSettings.zoom},
        };

        projectJson["Brush"] =
        {
            {"Colour", {project->brush.colour.r, project->brush.colour.g, project->brush.colour.b, project->brush.colour.a}},
            {"Size", project->brush.size},
        };

        int selectedLayer = 0;
        for (const Layer& layer : project->layers)
        {
            if (project->selectedLayer == &layer) break;
            else selectedLayer++;
        }

        json serializedRecentColours = json::array();
        for (const auto& colour : project->recentColours) {
            json serializedColor = {
                {"r", colour.r},
                {"g", colour.g},
                {"b", colour.b},
                {"a", colour.a}
            };
            serializedRecentColours.push_back(serializedColor);
        }
        projectJson["RecentColours"] = serializedRecentColours;

        projectJson["Other"] =
        {
            {"Name", project->name},
            {"SelectedLayer", selectedLayer},
            {"LastSaveLocation", project->lastSaveLocation},
            {"LastExportLocation", project->lastExportLocation},
            {"LastSaveName", project->lastSaveName},
            {"LastExportName", project->lastExportLocation}
        };

        projectJson["Layers"] = serializedLayers;

        std::ofstream outFile(path);
        if (outFile.is_open())
        {
            outFile << encode(projectJson.dump(4));
            outFile.close();
            //std::cout << "Project saved to: " << path.string() << std::endl;
        }
        else std::cerr << "Failed to save project to: " << path.string() << std::endl;
        project->saved = true;
    }

    Project LoadProject(std::filesystem::path path) // Todo: If project is already open, set project as selected
    {
        Project project;

        std::ifstream inFile(path, std::ios::binary);
        if (!inFile.is_open())
        {
            std::cerr << "Failed to open project: " << path.string() << std::endl;
            return project;
        }

        // --------------- TODO: Instead of doing all of that below to get file contents, do it like how I did it in GetSetting() ---------------

        inFile.seekg(0, std::ios::end);
        std::streampos fileSize = inFile.tellg();
        inFile.seekg(0, std::ios::beg);

        std::string projectString;
        projectString.resize(fileSize);
        inFile.read(&projectString[0], fileSize);

        projectString = decode(projectString);

        nlohmann::json projectJson = nlohmann::json::parse(projectString);

        project.projectSettings.size.x = projectJson["Settings"]["Size"][0];
        project.projectSettings.size.y = projectJson["Settings"]["Size"][1];
        project.projectSettings.zoom = projectJson["Settings"]["Zoom"];

        project.brush.colour.r = projectJson["Brush"]["Colour"][0];
        project.brush.colour.g = projectJson["Brush"]["Colour"][1];
        project.brush.colour.b = projectJson["Brush"]["Colour"][2];
        project.brush.colour.a = projectJson["Brush"]["Colour"][3];
        project.brush.size = projectJson["Brush"]["Size"];

        for (const auto& serializedColor : projectJson["RecentColours"])
        {
            SDL_Color color;
            color.r = serializedColor["r"];
            color.g = serializedColor["g"];
            color.b = serializedColor["b"];
            color.a = serializedColor["a"];

            project.recentColours.push_back(color);
        }

        const nlohmann::json& serializedLayers = projectJson["Layers"];
        for (const auto& serializedLayer : serializedLayers)
        {
            Layer layer;

            for (auto it = serializedLayer["Pixels"].begin(); it != serializedLayer["Pixels"].end(); ++it)
            {
                const std::string& key = it.key();
                const nlohmann::json& serializedPixel = it.value();

                Pixel pixel;
                pixel.rect.x = serializedPixel["rect"][0];
                pixel.rect.y = serializedPixel["rect"][1];
                pixel.rect.w = serializedPixel["rect"][2];
                pixel.rect.h = serializedPixel["rect"][3];
                pixel.relativePos.x = serializedPixel["relativePos"][0];
                pixel.relativePos.y = serializedPixel["relativePos"][1];
                pixel.colour.r = serializedPixel["colour"][0];
                pixel.colour.g = serializedPixel["colour"][1];
                pixel.colour.b = serializedPixel["colour"][2];
                pixel.colour.a = serializedPixel["colour"][3];
                pixel.exists = serializedPixel["exists"];

                int x, y;
                sscanf_s(key.c_str(), "%d-%d", &x, &y);
                layer.pixels[{x, y}] = pixel;
            }
            layer.visible = serializedLayer["Visible"];

            project.layers.push_back(layer);
        }

        project.selectedLayer = &project.layers[projectJson["Other"]["SelectedLayer"]];

        project.name = projectJson["Other"]["Name"];
        project.lastSaveLocation = projectJson["Other"]["LastSaveLocation"];
        project.lastExportLocation = projectJson["Other"]["LastExportLocation"];
        project.lastSaveName = projectJson["Other"]["LastSaveName"];
        project.lastExportName = projectJson["Other"]["LastExportName"];

        inFile.close();

        userSettings.recentProjects.erase(std::remove(userSettings.recentProjects.begin(), userSettings.recentProjects.end(), path.string()), userSettings.recentProjects.end());
        userSettings.recentProjects.insert(userSettings.recentProjects.begin(), path.string());
        SaveSettings();

        return project;
    }

    void ExportProject(SDL_Renderer* renderer, Project* project, std::filesystem::path path, SDL_Point size)
    {
        if ("path" == "null") return;

        int channels = 4;
        if (path.extension().string() == ".jpg" || path.extension().string() == ".jpeg") channels = 3;

        uint8_t* pixelData = new uint8_t[size.x * size.y * channels];
        int index = 0;
        for (int y = 0; y < size.y; y++)
        {
            for (int x = 0; x < size.x; x++)
            {
                int r = -1, g = -1, b = -1, a = -1;
                for (int i = project->layers.size() - 1; i >= 0; i--)
                {
                    if (!project->layers[i].visible) continue;
                    auto pixel = project->layers[i].pixels.find(std::make_pair(x, y));

                    if (pixel != project->layers[i].pixels.end() && pixel->second.exists)
                    {
                        r = pixel->second.colour.r;
                        g = pixel->second.colour.g;
                        b = pixel->second.colour.b;
                        if (channels == 4) a = pixel->second.colour.a;
                    }
                }
                if (r == -1)
                {
                    if (channels == 4) pixelData[index++] = 0;
                    else pixelData[index++] = 255;
                }
                else pixelData[index++] = r;
                if (g == -1)
                {
                    if (channels == 4) pixelData[index++] = 0;
                    else pixelData[index++] = 255;
                }
                else pixelData[index++] = g;
                if (b == -1)
                {
                    if (channels == 4) pixelData[index++] = 0;
                    else pixelData[index++] = 255;
                }
                else pixelData[index++] = b;
                if (a == -1)
                {
                    if (channels == 4) pixelData[index++] = 0;
                    else pixelData[index++] = 255;
                }
                else if (channels == 4) pixelData[index++] = a;
                else pixelData[index++] = 255;
            }
        }

        /*
           You can configure it with these global variables:
int stbi_write_tga_with_rle;             // defaults to true; set to 0 to disable RLE
int stbi_write_png_compression_level;    // defaults to 8; set to higher for more compression
int stbi_write_force_png_filter;         // defaults to -1; set to 0..5 to force a filter mode
                  */

        int success = 0;
        if (path.extension().string() == ".png")
            success = stbi_write_png(path.string().c_str(), size.x, size.y, channels, pixelData, size.x * channels);
        else if (path.extension().string() == ".jpg" || path.extension().string() == ".jpeg")
            success = stbi_write_jpg(path.string().c_str(), size.x, size.y, channels, pixelData, 100);
        else if (path.extension().string() == ".bmp")
            success = stbi_write_bmp(path.string().c_str(), size.x, size.y, channels, pixelData);
        else if (path.extension().string() == ".tga")
            success = stbi_write_tga(path.string().c_str(), size.x, size.y, channels, pixelData);
        else std::cerr << "Incorrect image format selected: " << path.extension().string() << std::endl;

        if (success)
        {
            std::cout << "Exported image: " << path.filename().string() << std::endl;
        }
        else
        {
            std::cerr << "Image " << path.filename().string() << " failed to export" << std::endl;
        }
    }

    std::string SaveProjectDialog(SDL_Window* window, SDL_Renderer* renderer, FileType fileType, std::string fileName)
    {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (!SDL_GetWindowWMInfo(window, &info)) return "null";
        HWND hwnd = info.info.win.window;
        HINSTANCE hInstance = GetModuleHandle(NULL);
        std::string path;

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileSaveDialog* pFileSave;

            hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
                IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));

            if (SUCCEEDED(hr))
            {
                std::vector<COMDLG_FILTERSPEC> fileTypes;
                if (fileType == ExportType)
                {
                    fileTypes = {
                        { L"PNG Files", L"*.png" },
                        { L"JPEG Files", L"*.jpg;*.jpeg" },
                        { L"BMP Files", L"*.bmp" },
                        { L"TGA Files", L"*.tga" }
                    };
                    pFileSave->SetDefaultExtension(L"png");
                }
                else
                {
                    fileTypes = {
                        { L"SpriteLab File", L"*.spl" }
                    };
                    pFileSave->SetDefaultExtension(L"spl");
                }

                std::wstring wFileName = std::wstring(fileName.begin(), fileName.end());

                pFileSave->SetFileTypes(static_cast<UINT>(fileTypes.size()), fileTypes.data());
                pFileSave->SetFileName(wFileName.c_str());

                hr = pFileSave->Show(NULL);

                if (SUCCEEDED(hr))
                {
                    IShellItem* pItem;
                    hr = pFileSave->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                        if (SUCCEEDED(hr))
                        {
                            int len = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                            std::string str(len, '\0');
                            WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &str[0], len, NULL, NULL);
                            path = str;

                            CoTaskMemFree(pszFilePath);
                        }
                        else path = "null";
                        pItem->Release();
                    }
                    else path = "null";
                }
                else path = "null";
                pFileSave->Release();
            }
            CoUninitialize();
        }

        path.erase(std::remove_if(path.begin(), path.end(), [](char c) {
            return !std::isprint(static_cast<unsigned char>(c));
            }), path.end());
        return path;
    }

    std::string LoadProjectDialog(SDL_Window* window, SDL_Renderer* renderer)
    {
        SDL_SysWMinfo info;
        SDL_VERSION(&info.version);
        if (!SDL_GetWindowWMInfo(window, &info)) return "null";
        HWND hwnd = info.info.win.window;
        HINSTANCE hInstance = GetModuleHandle(NULL);
        std::string path;

        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr))
        {
            IFileOpenDialog* pFileOpen;

            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
                IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

            if (SUCCEEDED(hr))
            {
                COMDLG_FILTERSPEC fileTypes[] = {
                    { L"SpriteLab File", L"*.spl" }
                };

                pFileOpen->SetFileTypes(static_cast<UINT>(std::size(fileTypes)), fileTypes);
                pFileOpen->SetDefaultExtension(L"spl");

                hr = pFileOpen->Show(NULL);

                if (SUCCEEDED(hr))
                {
                    IShellItem* pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    if (SUCCEEDED(hr))
                    {
                        PWSTR pszFilePath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                        if (SUCCEEDED(hr))
                        {
                            int len = WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, NULL, 0, NULL, NULL);
                            std::string str(len, '\0');
                            WideCharToMultiByte(CP_UTF8, 0, pszFilePath, -1, &str[0], len, NULL, NULL);
                            path = str;

                            CoTaskMemFree(pszFilePath);
                        }
                        else path = "null";
                        pItem->Release();
                    }
                    else path = "null";
                }
                else path = "null";

                pFileOpen->Release();
            }
            CoUninitialize();
        }

        // Rest of your code
        return path;
    }

    void SpriteLab::SetSetting(std::string setting, std::string value, std::string value2)
    {
        json settingsJson;
        std::filesystem::path settingsPath = (std::filesystem::path(__FILE__).parent_path() / ".settings");
        if (std::filesystem::exists(settingsPath))
        {
            std::ifstream settingsFile(settingsPath);
            if (!settingsFile.is_open()) {
                std::cerr << "Failed to open settings file." << std::endl;
                return;
            }

            try
            {
                std::string settingsString((std::istreambuf_iterator<char>(settingsFile)), std::istreambuf_iterator<char>());
                settingsJson = json::parse(settingsString);
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error parsing settings JSON: " << e.what() << std::endl;
                return;
            }
        }

        if (value2 == "null") settingsJson[setting] = value;
        else settingsJson[setting] = {value, value2};

        std::ofstream outFile(settingsPath);
        if (outFile.is_open())
        {
            outFile << settingsJson.dump(4);
            outFile.close();
        }
        else std::cerr << "Failed to save settings: " << std::endl;
    }

    std::string SpriteLab::GetSetting(std::string setting)
    {
        std::ifstream settingsFile(std::filesystem::path(__FILE__).parent_path() / ".settings");
        if (!settingsFile.is_open()) {
            std::cerr << "Failed to open settings file, may not exist." << std::endl;
            return "null";
        }

        try
        {
            std::string settingsString((std::istreambuf_iterator<char>(settingsFile)), std::istreambuf_iterator<char>());
            json settingsJson = json::parse(settingsString);
            return settingsJson[setting];
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error parsing settings JSON: " << e.what() << std::endl;
            return "null";
        }
    }

    void ProjectSave(SDL_Window* window, SDL_Renderer* renderer, Project* project)
    {
        std::string path;
        if (project->lastSaveLocation == "") return ProjectSaveAs(window, renderer, project);
        else path = project->lastSaveLocation + "/" + project->name + ".spl";
        if (path != "null" && path != "")SaveProject(project, path);
    }

    void ProjectSaveAs(SDL_Window* window, SDL_Renderer* renderer, Project* project)
    {
        std::string path = SaveProjectDialog(window, renderer, SaveType, project->name);
        if (path != "null")
        {
            std::filesystem::path fspath = path;
            project->lastSaveLocation = fspath.parent_path().string();
            project->lastSaveName = fspath.filename().string();
            project->name = fspath.stem().string();
            SaveProject(project, path);
        }
    }

    void ProjectExport(SDL_Window* window, SDL_Renderer* renderer, Project* project)
    {
        std::string path;
        if (project->lastExportLocation == "") return ProjectExportAs(window, renderer, project);
        else path = project->lastExportLocation + "/" + project->lastExportName;
        if (path != "null" && path != "") ExportProject(renderer, project, path, { static_cast<int>(project->projectSettings.size.x), static_cast<int>(project->projectSettings.size.y) });

    }

    void ProjectExportAs(SDL_Window* window, SDL_Renderer* renderer, Project* project)
    {
        std::string path = SaveProjectDialog(window, renderer, ExportType, project->name);
        if (path != "null")
        {
            std::filesystem::path fspath = path;
            project->lastExportLocation = fspath.parent_path().string();
            project->lastExportName = fspath.filename().string();
            ExportProject(renderer, project, path, { static_cast<int>(project->projectSettings.size.x), static_cast<int>(project->projectSettings.size.y) });
        }
    }

    void LoadSettings()
    {
        json settingsJson;
        std::ifstream settingsFile(std::filesystem::path(__FILE__).parent_path() / ".settings");
        if (!settingsFile.is_open()) return;

        try
        {
            std::string settingsString((std::istreambuf_iterator<char>(settingsFile)), std::istreambuf_iterator<char>());
            settingsString = decode(settingsString);
            json settingsJson = json::parse(settingsString);

            userSettings.preferedCanvasSize.x = settingsJson["PreferedWidth"];
            userSettings.preferedCanvasSize.y = settingsJson["PreferedHeight"];

            for (const auto& recentProject : settingsJson["RecentProjects"])
            {
                userSettings.recentProjects.push_back(recentProject);
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error parsing settings JSON: " << e.what() << std::endl;
            return;
        }
    }

    void SaveSettings()
    {
        json settingsJson;
        std::filesystem::path path = std::filesystem::path(__FILE__).parent_path() / ".settings";

        settingsJson["PreferedWidth"] = userSettings.preferedCanvasSize.x;
        settingsJson["PreferedHeight"] = userSettings.preferedCanvasSize.y;

        for (const std::string project : userSettings.recentProjects)
        {
            settingsJson["RecentProjects"].push_back(project);
        }

        std::ofstream outFile(path);
        if (outFile.is_open())
        {
            outFile << encode(settingsJson.dump(4));
            outFile.close();
        }
        else std::cerr << "Failed to save settings to: " << path.string() << std::endl;
    }
}
