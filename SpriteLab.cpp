#include "SpriteLab.h"
#include <filesystem>
#include <unordered_map>
#define __STDC_LIB_EXT1__
#define STB_IMAGE_WRITE_IMPLEMENTATION    
#include "ProjectFileManager.h"

using namespace std;

namespace SpriteLab
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    bool resetToolbar = true;
    bool resetTopToolbar = true;
    bool resetLayersMenu = true;
    bool savePopupOpened = false;
    bool colourPickerOpened = false;
    bool resetColourPicker = true;
    bool renderProjectsMenu = false;
    bool renderCreateProjectMenu = false; 
    bool createProjectShowTitleBar = false;

    int tempWidth = 100;
    int tempHeight = 100;

    ImVec4 colourPickerColour;

    vector<SDL_Texture*> destroyTexture;

    unordered_map<string, SDL_Texture*> textures;

    vector<Project> projects;
    Project* selectedProject;

    set<SDL_Keycode> keysPressed;

    Pixel selectedPixel = {}; // Used for tools like Line Tool. This should be moved.

    void OpenCreateProjectMenu(bool showTitleBar)
    {
        if (userSettings.preferedCanvasSize.x != -1)
        {
            tempWidth = userSettings.preferedCanvasSize.x;
            tempHeight = userSettings.preferedCanvasSize.x;
        }
        createProjectShowTitleBar = showTitleBar;
        renderCreateProjectMenu = true;
    }

    void SpriteLab::RenderMenuBar()
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("MenuBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "CTRL+N"))
                {
                    OpenCreateProjectMenu(true);
                    //if (selectedProject->changesSinceSave)
                    //{
                    //    savePopupMessage = L"Do you want to save before creating a new project?";
                    //    savePopupOpened = true;
                    //    SaveProjectPopup();
                    //    while (savePopupOpened)
                    //    {
                    //        if (savePopupButtonClicked == "") continue;
                    //        savePopupOpened = false;
                    //        if (savePopupButtonClicked == "Yes") {}
                    //        else if (savePopupButtonClicked == "No") {}
                    //        savePopupButtonClicked = "";
                    //        OpenCreateProjectMenu(true);
                    //    }
                    //}
                }
                if (ImGui::MenuItem("Open", "CTRL+O"))
                {
                    string path = LoadProjectDialog(window, renderer);
                    if (path != "null")
                    {
                        Project project = LoadProject(path);
                        bool alreadyOpen = false;

                        for (Project& project2 : projects)
                        {
                            if (!project2.Compare(project)) continue;
                            alreadyOpen = true;
                            break;
                        }

                        if (!alreadyOpen)
                        {
                            projects.push_back(project);
                            selectedProject = &projects.back();
                            SpriteLab::selectedProject->projectSettings.size = ImVec2(10, 10);
                            SpriteLab::selectedProject->layers.push_back({});
                            SpriteLab::selectedProject->selectedLayer = &SpriteLab::selectedProject->layers[0];
                        }
                    }
                }
                if (ImGui::BeginMenu("Open Recent"))
                {
                    for (string projectPath : userSettings.recentProjects)
                    {
                        if (!filesystem::exists(projectPath))
                        {
                            userSettings.recentProjects.erase(std::remove(userSettings.recentProjects.begin(), userSettings.recentProjects.end(), projectPath), userSettings.recentProjects.end());
                            continue;
                        }
                        if (ImGui::MenuItem(filesystem::path(projectPath).filename().string().c_str(), ""))
                        {
                            Project project = LoadProject(projectPath);
                            bool alreadyOpen = false;

                            for (Project& project2 : projects)
                            {
                                if (!project2.Compare(project)) continue;
                                alreadyOpen = true;
                                break;
                            }

                            if (!alreadyOpen)
                            {
                                projects.push_back(project);
                                selectedProject = &projects.back();
                                SpriteLab::selectedProject->projectSettings.size = ImVec2(10, 10);
                                SpriteLab::selectedProject->layers.push_back({});
                                SpriteLab::selectedProject->selectedLayer = &SpriteLab::selectedProject->layers[0];
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "CTRL+S"))
                {
                    ProjectSave(window, renderer, selectedProject);
                }
                if (ImGui::MenuItem("Save As", "CTRL+SHIFT+S"))
                {
                    ProjectSaveAs(window, renderer, selectedProject);
                }
                if (ImGui::MenuItem("Close", "CTRL+W")) {}
                if (ImGui::MenuItem("Close All", "CTRL+SHIFT+W")) {}
                ImGui::Separator();
                if (ImGui::MenuItem("Export", "CTRL+E"))
                {
                    ProjectExport(window, renderer, selectedProject);
                }
                if (ImGui::MenuItem("Export As", "CTRL+SHIFT+E"))
                {
                    ProjectExportAs(window, renderer, selectedProject);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Exit", "CTRL+Q")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Project Settings", "")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help"))
            {
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }

        ImGui::End();
    }

    void SpriteLab::RenderBackground()
    {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        SDL_Rect dstrect = { 50, 90, width - 50, height - 140};
        //SDL_RenderClear(renderer);
        //SDL_Rect dstrect = { 0, 0, 1920 , 1080 };
        SDL_RenderCopy(renderer, textures["CanvasBackground"], NULL, &dstrect);
    }

    void BucketToolFill(Pixel& pixel, SDL_Color color, ImVec2 size, int mouseX, int mouseY)
    {
        //ImVec2 canvasPos = selectedProject->canvasMinPos;
        if (pixel.relativePos.x < 0 || pixel.relativePos.x >(selectedProject->projectSettings.size.x - 1) || pixel.relativePos.y < 0 || pixel.relativePos.y >(selectedProject->projectSettings.size.y - 1)) return;
        //if (pixel.rect.x < selectedProject->canvasMinPos.x || pixel.rect.y < selectedProject->canvasMinPos.y || pixel.rect.x > selectedProject->canvasMaxPos.x || pixel.rect.y > selectedProject->canvasMaxPos.y) return;
        SDL_Color oldColor = pixel.colour;
        pixel.colour = color;
        if (!pixel.exists) selectedProject->selectedLayer->pixels[make_pair(pixel.relativePos.x, pixel.relativePos.y)] = Pixel{ pixel.rect, pixel.relativePos, selectedProject->brush.colour };

        for (int i = 0; i < 4; i++)
        {
            int xDifference = ((selectedProject->canvasMaxPos.x - selectedProject->canvasMinPos.x) / selectedProject->projectSettings.size.x);
            int yDifference = (selectedProject->canvasMaxPos.y - selectedProject->canvasMinPos.y) / selectedProject->projectSettings.size.y;
            int x = (i == 2) ? 1 : (i == 3) ? -1 : 0;
            int y = (i == 0) ? 1 : (i == 1) ? -1 : 0;


            pair pixelPair = make_pair(pixel.relativePos.x + x, pixel.relativePos.y + y);
            if (selectedProject->selectedLayer->pixels.find(pixelPair) != selectedProject->selectedLayer->pixels.end())
            {
                if (CompareColor(oldColor, selectedProject->selectedLayer->pixels[pixelPair].colour)) BucketToolFill(selectedProject->selectedLayer->pixels[pixelPair], color, size, mouseX, mouseY);
            }
            else if (!pixel.exists)
            {
                Pixel newPixel = Pixel{ {pixel.rect.x + (x*xDifference), pixel.rect.y + (y * yDifference), pixel.rect.w, pixel.rect.h},{pixel.relativePos.x + x, pixel.relativePos.y + y}, {0,0,0,0}, false };
                BucketToolFill(newPixel, color, size, mouseX, mouseY);
            }
        }
    }

    void PaintBrushTool(int mouseX, int mouseY, int width)
    {
        //ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        //SDL_Point canvasPos;
        //SDL_GetWindowPosition(window, &canvasPos.x, &canvasPos.y);
        ImVec2 canvasPos = selectedProject->canvasMinPos;

        int cellSize = selectedProject->projectSettings.zoom;
        int gridX = static_cast<int>((mouseX - canvasPos.x) / cellSize);
        int gridY = static_cast<int>((mouseY - canvasPos.y) / cellSize);

        //int xOffset = (width - ImGui::GetWindowSize().x - ((ImGui::GetWindowSize().x - size.x) / 2)) / (size.x/gridX);
        //int yOffset = (height - ImGui::GetWindowSize().y - ((ImGui::GetWindowSize().y - size.y) / 2)) / (size.y / gridY);
        //int xOffset = (width - size.x)/2;
        //int xOffset = abs(((ImGui::GetWindowSize().x - size.x) / 2) - (canvasPos.x + gridX * cellSize));

        //int realCanvasPosX = ((ImGui::GetWindowSize().x - size.x) / 2);
        //realCanvasPosX = realCanvasPosX - ((width - ImGui::GetWindowSize().x) / 2);
        int xOffset = 0;
        int yOffset = 0;

        ////bool foundClosestXCell = false;
        ////int currentCell = -1;
        ////while (!foundClosestXCell)
        ////{
        ////    currentCell++;
        ////    if (currentCell * cellSize < (canvasPos.x + abs(((ImGui::GetWindowSize().x - size.x) / 2) - (currentCell * cellSize)))) continue;
        ////    foundClosestXCell = true;
        ////}

        //cout << "Found closest cell at " + to_string(currentCell * cellSize) << endl;
        //cout << "Canvas starts at " + to_string(realCanvasPosX) << endl;
        //cout << "Mouse X Pos " + to_string(mouseX) << endl;

        //xOffset = abs((realCanvasPosX) - (currentCell * cellSize));

        //cout << to_string(xOffset) << endl;

        SDL_Rect pixelRect;
        pixelRect.x = canvasPos.x - xOffset + gridX * cellSize;
        pixelRect.y = canvasPos.y - yOffset + gridY * cellSize;
        pixelRect.w = cellSize;
        pixelRect.h = cellSize;

        SDL_SetRenderDrawColor(renderer, selectedProject->brush.colour.r, selectedProject->brush.colour.g, selectedProject->brush.colour.b, selectedProject->brush.colour.a);
        SDL_RenderFillRect(renderer, &pixelRect);

        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            // This saves individual pixels from an SDL_Rect
            //for (int y = 0; y < cellSize; y++)
            //{
            //    for (int x = 0; x < cellSize; x++)
            //    {
            //        SDL_Rect pixelPos;
            //        pixelPos.x = canvasPos.x + gridX * cellSize + x;
            //        pixelPos.y = canvasPos.y + gridY * cellSize + y - 8;
            //        pixelPos.w = 1;
            //        pixelPos.h = 1;

            //        pixels[std::make_pair(pixelPos.x, pixelPos.y)] = Pixel{ pixelPos, SDL_Color{255, 0, 0, 255} };
            //    }
            //}
            selectedProject->selectedLayer->pixels[std::make_pair(gridX, gridY)] = Pixel{ pixelRect, {gridX, gridY}, selectedProject->brush.colour };
            selectedProject->saved = false;
        }
    } // Pencil tool

    void EraserTool(int mouseX, int mouseY, int width)
    {
        ImVec2 canvasPos = selectedProject->canvasMinPos;

        int cellSize = selectedProject->projectSettings.zoom;
        int gridX = static_cast<int>((mouseX - canvasPos.x) / cellSize);
        int gridY = static_cast<int>((mouseY - canvasPos.y) / cellSize);

        int xOffset = 0;
        int yOffset = 0;

        SDL_Rect pixelRect;
        pixelRect.x = canvasPos.x - xOffset + gridX * cellSize;
        pixelRect.y = canvasPos.y - yOffset + gridY * cellSize;
        pixelRect.w = cellSize;
        pixelRect.h = cellSize;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &pixelRect);

        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT) && selectedProject->selectedLayer->pixels.find(std::make_pair(gridX, gridY)) != selectedProject->selectedLayer->pixels.end())
        {
            selectedProject->selectedLayer->pixels.erase(make_pair(gridX, gridY));
            selectedProject->saved = false;
        }
    }// Eraser Tool

    void BucketTool(int mouseX, int mouseY, int width, ImVec2 size) // Fill tool
    {
        ImVec2 canvasPos = selectedProject->canvasMinPos;

        int cellSize = selectedProject->projectSettings.zoom;
        int gridX = static_cast<int>((mouseX - canvasPos.x) / cellSize);
        int gridY = static_cast<int>((mouseY - canvasPos.y) / cellSize);

        int xOffset = 0;
        int yOffset = 0;

        SDL_Rect pixelRect;
        pixelRect.x = canvasPos.x - xOffset + gridX * cellSize;
        pixelRect.y = canvasPos.y - yOffset + gridY * cellSize;
        pixelRect.w = cellSize;
        pixelRect.h = cellSize;

        SDL_SetRenderDrawColor(renderer, selectedProject->brush.colour.r, selectedProject->brush.colour.g, selectedProject->brush.colour.b, selectedProject->brush.colour.a);
        SDL_RenderFillRect(renderer, &pixelRect);

        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            SDL_Point pixelPos = { static_cast<int>((mouseX - selectedProject->canvasMinPos.x) / cellSize), static_cast<int>((mouseY - selectedProject->canvasMinPos.y) / cellSize) };
            pair pixelPair = make_pair(pixelPos.x, pixelPos.y);
            if (selectedProject->selectedLayer->pixels.find(pixelPair) != selectedProject->selectedLayer->pixels.end())
            {
                if (CompareColor(selectedProject->selectedLayer->pixels[pixelPair].colour, selectedProject->brush.colour)) return;
                BucketToolFill(selectedProject->selectedLayer->pixels[pixelPair], selectedProject->brush.colour, size, mouseX, mouseY);
                selectedProject->saved = false;
            }
            else
            {
                Pixel pixel = Pixel{ {pixelRect}, pixelPos, {0,0,0,0}, false };
                BucketToolFill(pixel, selectedProject->brush.colour, size, mouseX, mouseY);
                selectedProject->saved = false;
            }
        }
    }

    void EyeDropperTool(int mouseX, int mouseY, int width)
    {
        ImVec2 canvasPos = selectedProject->canvasMinPos;

        int cellSize = selectedProject->projectSettings.zoom;
        int gridX = static_cast<int>((mouseX - canvasPos.x) / cellSize);
        int gridY = static_cast<int>((mouseY - canvasPos.y) / cellSize);


        SDL_Rect pixelRect;
        pixelRect.x = canvasPos.x + gridX * cellSize;
        pixelRect.y = canvasPos.y + gridY * cellSize;
        pixelRect.w = cellSize;
        pixelRect.h = cellSize;

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &pixelRect);

        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT) && selectedProject->selectedLayer->pixels.find(std::make_pair(gridX, gridY)) != selectedProject->selectedLayer->pixels.end())
            SetBrushColour(selectedProject, &selectedProject->brush, selectedProject->selectedLayer->pixels[make_pair(gridX, gridY)].colour);
    }

    void LineToolFill(Pixel pixel, SDL_Point endPoint, SDL_Color color)
    { 
        if (pixel.relativePos.x == endPoint.x && pixel.relativePos.y == endPoint.y)
        {
            SDL_SetRenderDrawColor(renderer, selectedProject->brush.colour.r, selectedProject->brush.colour.g, selectedProject->brush.colour.b, selectedProject->brush.colour.a);
            SDL_RenderFillRect(renderer, &pixel.rect);
            return;
        }
        vector<Pixel> pixels;
        for (int i = 0; i < 8; i++)
        {
            int xDifference = ((selectedProject->canvasMaxPos.x - selectedProject->canvasMinPos.x) / selectedProject->projectSettings.size.x);
            int yDifference = (selectedProject->canvasMaxPos.y - selectedProject->canvasMinPos.y) / selectedProject->projectSettings.size.y;
            int x = (i == 2 || i == 4 || i == 6) ? 1 : (i == 3 || i == 5 || i == 7) ? -1 : 0;
            int y = (i == 0 || i == 4 || i == 7) ? 1 : (i == 1 || i == 5 || i == 6) ? -1 : 0;

            pair pixelPair = make_pair(pixel.relativePos.x + x, pixel.relativePos.y + y);
            if (selectedProject->selectedLayer->pixels.find(pixelPair) != selectedProject->selectedLayer->pixels.end())
            {
                pixels.push_back(selectedProject->selectedLayer->pixels[pixelPair]);
            }
            else if (!pixel.exists)
            {
                pixels.push_back({ {pixel.rect.x + (x * xDifference), pixel.rect.y + (y * yDifference), pixel.rect.w, pixel.rect.h},{pixel.relativePos.x + x, pixel.relativePos.y + y}, {0,0,0,0}, false });
            }
        }

        int closestDistance = 99999;
        Pixel closestPixel;
        for (const Pixel& pixel : pixels) // ---------------------- Todo: Calculate this in the loop above ----------------------
        {
            int dx = pixel.relativePos.x - endPoint.x;
            int dy = pixel.relativePos.y - endPoint.y;
            int distance = dx * dx + dy * dy;
            if (distance < closestDistance)
            {
                closestDistance = distance;
                closestPixel = pixel;
            }
        }
        LineToolFill(closestPixel, endPoint, selectedProject->brush.colour);

        SDL_SetRenderDrawColor(renderer, selectedProject->brush.colour.r, selectedProject->brush.colour.g, selectedProject->brush.colour.b, selectedProject->brush.colour.a);
        SDL_RenderFillRect(renderer, &pixel.rect);
    }

    void LineTool(int mouseX, int mouseY, int width, ImVec2 size)
    {
        ImVec2 canvasPos = selectedProject->canvasMinPos;

        int cellSize = selectedProject->projectSettings.zoom;
        int gridX = static_cast<int>((mouseX - canvasPos.x) / cellSize);
        int gridY = static_cast<int>((mouseY - canvasPos.y) / cellSize);

        SDL_Rect pixelRect;
        pixelRect.x = canvasPos.x + gridX * cellSize;
        pixelRect.y = canvasPos.y + gridY * cellSize;
        pixelRect.w = cellSize;
        pixelRect.h = cellSize;

        SDL_SetRenderDrawColor(renderer, selectedProject->brush.colour.r, selectedProject->brush.colour.g, selectedProject->brush.colour.b, selectedProject->brush.colour.a);
        SDL_RenderFillRect(renderer, &pixelRect);

        if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))
        {
            SDL_Point pixelPos = { static_cast<int>((mouseX - selectedProject->canvasMinPos.x) / cellSize), static_cast<int>((mouseY - selectedProject->canvasMinPos.y) / cellSize) };
            if (!selectedPixel.relativePos.x || selectedPixel.relativePos.x == -999)
            {
                pair pixelPair = make_pair(pixelPos.x, pixelPos.y);
                if (selectedProject->selectedLayer->pixels.find(pixelPair) != selectedProject->selectedLayer->pixels.end())
                    selectedPixel = selectedProject->selectedLayer->pixels[pixelPair];
                else selectedPixel = Pixel{ {pixelRect}, pixelPos, {0,0,0,0}, false };
            }
            LineToolFill(selectedPixel, pixelPos, selectedProject->brush.colour);
        }
        else if (!selectedPixel.relativePos.x || selectedPixel.relativePos.x != -999)
            selectedPixel = { -999, -999 };
    }

    void SpriteLab::RenderLayersMenu()
    {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        if (resetLayersMenu)
        {
            ImGui::SetNextWindowPos(ImVec2(width-200, 90));
            ImGui::SetNextWindowSize(ImVec2(200, height-140));
            resetLayersMenu = false;
        }
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::Begin("##LayersMenu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);

        int nextYPos = 20;
        int index = 0;
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        for (Layer& layer : selectedProject->layers)
        {
            ImGui::SetCursorPos(ImVec2(25, nextYPos));
            SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, selectedProject->projectSettings.size.x, selectedProject->projectSettings.size.y, 32, SDL_PIXELFORMAT_RGBA8888);

            SDL_Rect rect;
            for (const auto& pixel : layer.pixels)
            {
                rect = {pixel.second.relativePos.x, pixel.second.relativePos.y, 1, 1};
                SDL_FillRect(surface, &rect, SDL_MapRGBA(surface->format, pixel.second.colour.r, pixel.second.colour.g, pixel.second.colour.b, pixel.second.colour.a));
            }
            destroyTexture.push_back(SDL_CreateTextureFromSurface(renderer, surface));
            SDL_FreeSurface(surface);

            ImGui::Image(textures["TransparentBackground"], ImVec2(150, 150));
            ImGui::SetCursorPos(ImVec2(25, nextYPos));

            ImGui::Image(destroyTexture.back(), ImVec2(150, 150));
            bool layerClicked = ImGui::IsItemClicked(0);
            bool layerHovered = ImGui::IsItemHovered();

            ImGui::SetCursorPos(ImVec2(25, nextYPos));
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(0, 0, 0, 255), 0.0f, 0, 3.0f); // Todo: Maybe use frame padding instead

            bool anyLayerButtonClicked = false;
            if (selectedProject->selectedLayer == &layer)
            {
                ImGui::SetCursorPos(ImVec2(25, nextYPos));
                ImGui::GetWindowDrawList()->AddRect(ImVec2(ImGui::GetItemRectMin().x - 5, ImGui::GetItemRectMin().y - 5), ImVec2(ImGui::GetItemRectMax().x + 5, ImGui::GetItemRectMax().y + 5), ImColor(255, 255, 255, 255), 0.0f, 0, 3.0f);

                if (selectedProject->selectedLayer->visible)
                {
                    ImGui::SetCursorPos(ImVec2(145, nextYPos + 125));
                    ImGui::Image(textures["EyeIcon"], ImVec2(25, 25));
                    if (ImGui::IsItemClicked(0))
                    {
                        anyLayerButtonClicked = true;
                        selectedProject->selectedLayer->visible = !selectedProject->selectedLayer->visible;
                    }
                }
            }
            if (!layer.visible && !anyLayerButtonClicked)
            {
                if (selectedProject->selectedLayer != &layer)
                {
                    ImGui::SetCursorPos(ImVec2(25, nextYPos));
                    ImGui::Image(textures["FadedLayerCover"], ImVec2(150, 150));
                }
                ImGui::SetCursorPos(ImVec2(145, nextYPos + 125));
                ImGui::Image(textures["EyeStrikedIcon"], ImVec2(25, 25));
                if (ImGui::IsItemClicked(0))
                {
                    layer.visible = true;
                    anyLayerButtonClicked = true;
                }
            }

            bool breakLoop = false;
            if (layerHovered && selectedProject->layers.size() > 1)
            {
                ImGui::SetCursorPos(ImVec2(30, nextYPos + 5));
                ImGui::Image(textures["DeleteIcon"], ImVec2(25, 25));
                if (ImGui::IsItemClicked(0))
                {
                    for (auto it = selectedProject->layers.begin(); it != selectedProject->layers.end(); ++it) {
                        if (&(*it) == &layer) {
                            selectedProject->layers.erase(it);
                            if (selectedProject->selectedLayer == &layer) selectedProject->selectedLayer = &selectedProject->layers[0];
                            else selectedProject->selectedLayer = &selectedProject->layers[0];
                            breakLoop = true;
                            break;
                        }
                    }
                    anyLayerButtonClicked = true;
                }
            }
            if (breakLoop) break;

            if (layerClicked && !anyLayerButtonClicked) selectedProject->selectedLayer = &SpriteLab::selectedProject->layers[index];

            nextYPos += 175;
            index++;
        }

        ImGui::SetCursorPos(ImVec2(50, nextYPos));
        if (ImGui::ImageButton(textures["NewLayerIcon"], ImVec2(100, 100), ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 0, 0, 0)))
        {
            selectedProject->layers.push_back({});
            selectedProject->selectedLayer = &selectedProject->layers.back();
        }

        ImGui::SetCursorPos(ImVec2(25, nextYPos));
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(0, 0, 0, 255), 0.0f, 0, 3.0f); // Todo: Maybe use frame padding instead

        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleColor();
    }

    void SpriteLab::RenderCanvas()
    {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        ImGui::SetNextWindowPos(ImVec2(50, 90));
        ImGui::SetNextWindowSize(ImVec2(width - 100, height - 140));
        int diff = 20;
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        if (!selectedProject->bestZoomSet)
        {
            selectedProject->projectSettings.zoom = selectedProject->GetBestCanvasZoom();
            selectedProject->bestZoomSet = true;
            ImVec2 size = ImVec2(selectedProject->projectSettings.size.x * selectedProject->projectSettings.zoom, selectedProject->projectSettings.size.y * selectedProject->projectSettings.zoom);
            selectedProject->canvasMinPos = ImVec2(ImGui::GetWindowSize().x / 2 - (size.x / 2) + ((width - ImGui::GetWindowSize().x) / 2), ((ImGui::GetWindowSize().y) / 2 - (size.y / 2) + ((height - (ImGui::GetWindowSize().y)) / 2))+ diff);
            selectedProject->canvasMaxPos = ImVec2(selectedProject->canvasMinPos.x + size.x, selectedProject->canvasMinPos.y + size.y);
            //cout << "MIN X: " << selectedProject->canvasMinPos.x << " MAX X: " << selectedProject->canvasMaxPos.x << endl;
            //cout << "MIN Y: " << selectedProject->canvasMinPos.y << " MAX Y: " << selectedProject->canvasMaxPos.y << endl;
        }

        // ------------------------------------ Change code below to use selectedProject->canvasMinPos, and selectedProject->canvasMaxPos ------------------------------------

        ImVec2 size = ImVec2(selectedProject->projectSettings.size.x * selectedProject->projectSettings.zoom, selectedProject->projectSettings.size.y * selectedProject->projectSettings.zoom);
        //ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - (selectedProject->projectSettings.size.x * selectedProject->projectSettings.zoom) / 2, ImGui::GetWindowSize().y / 2 - (selectedProject->projectSettings.size.y * selectedProject->projectSettings.zoom) / 2));
        //ImGui::BeginChild("RealCanvas", size, false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ////ImGui::Image((ImTextureID)textures["TransparentBackground"], size, ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
        ImVec2 pos = ImVec2(ImGui::GetWindowSize().x / 2 - (size.x / 2) + ((width - ImGui::GetWindowSize().x)/2), (ImGui::GetWindowSize().y+ diff) / 2 - (size.y / 2) + ((height - (ImGui::GetWindowSize().y)+ diff)/2));
        //cout << "Pos x: " + std::to_string(pos.x) + " Pos y: " + std::to_string(pos.y)<< endl;
        //cout << "Size x: " + std::to_string(size.x) + " Size y: " + std::to_string(size.y) << endl;
        SDL_Rect dstrect = { static_cast<int>(pos.x), static_cast<int>(pos.y), static_cast<int>(size.x), static_cast<int>(size.y) };
        SDL_RenderCopy(renderer, textures["TransparentBackground"], NULL, &dstrect);

        for (int i = selectedProject->layers.size() - 1; i >= 0; i--) // Todo: Consider only rendering visible pixels. If the layer above has the pixel set and the alpha is set to 255, then don't render pixel
        {
            if (!selectedProject->layers[i].visible) continue;
            for (const auto& pixel : selectedProject->layers[i].pixels)
            {
                SDL_SetRenderDrawColor(renderer, pixel.second.colour.r, pixel.second.colour.g, pixel.second.colour.b, pixel.second.colour.a);
                SDL_RenderFillRect(renderer, &pixel.second.rect);
            }
        }
        int mouseX, mouseY;
        SDL_GetMouseState(&mouseX, &mouseY);

        SDL_FPoint mouse = { (mouseX - selectedProject->canvasMinPos.x) / selectedProject->projectSettings.zoom, (mouseY - selectedProject->canvasMinPos.y) / selectedProject->projectSettings.zoom };
        if (mouse.x >= 0 && mouse.x < selectedProject->projectSettings.size.x && mouse.y >= 0 && mouse.y < selectedProject->projectSettings.size.y)
        {
            if (selectedProject->selectedTool == SpriteLab::PaintBrush) PaintBrushTool(mouseX, mouseY, width);
            else if (selectedProject->selectedTool == SpriteLab::Eraser) EraserTool(mouseX, mouseY, width);
            else if (selectedProject->selectedTool == SpriteLab::PaintBucket) BucketTool(mouseX, mouseY, width, size);
            else if (selectedProject->selectedTool == SpriteLab::EyeDropper) EyeDropperTool(mouseX, mouseY, width);
            else if (selectedProject->selectedTool == SpriteLab::Line) LineTool(mouseX, mouseY, width, size);
        }

        //cout << "Pixels to render: " + to_string(selectedProject->selectedLayer->pixels.size()) << endl;

        //ImGui::EndChild();

        ImGui::End();
    }

    void SpriteLab::RenderProjectTabs()
    {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        ImGui::SetNextWindowPos(ImVec2(0, 20));
        ImGui::SetNextWindowSize(ImVec2(width, 30));
        ImGui::Begin("##ProjectTabs", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

        int index = -1;
        int nextX = 5;
        for (Project& project : projects)
        {
            string projectName;
            if (project.name.size() > 17) projectName = project.name.substr(0, 17) + "...";
            else projectName = project.name;
            if (!project.saved) projectName += " *";
            index++;
            if (!project.Compare(*selectedProject))
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.17f, 0.17f, 0.17f, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.20f, 0.20f, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.20f, 0.20f, 1));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.22f, 0.22f, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.22f, 0.22f, 1));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.22f, 0.22f, 1));
            }
            ImGui::SetCursorPos(ImVec2(nextX + 155, 5));
            if (ImGui::Button(("X##" + to_string(index) + "").c_str(), ImVec2(20, 20)))
            {
                for (auto it = projects.begin(); it != projects.end(); ++it) {
                    if (&(*it) == &project) {
                        projects.erase(it);
                        if (selectedProject == &project) selectedProject = &projects[0];
                        else selectedProject = &projects[0];
                        break;
                    }
                }
            }
            ImGui::SetCursorPos(ImVec2(nextX, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0, 0.5f));
            if (ImGui::Button((projectName + "##" + to_string(index) + "").c_str(), ImVec2(175, 30))) // Detects clicks on the X button
            {
                SpriteLab::selectedProject = &project;
            }
            ImGui::PopStyleVar();
            ImGui::SetCursorPos(ImVec2(nextX + 155, 5));
            if (ImGui::Button(("X##" + to_string(index) + "").c_str(), ImVec2(20, 20))) {} // Displays the X button
            ImGui::PopStyleColor(3);
            nextX += 180;
        }

        ImGui::End();
    }

    void SpriteLab::RenderTopToolBar()
    {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        if (resetTopToolbar)
        {
            ImGui::SetNextWindowPos(ImVec2(0, 50));
            ImGui::SetNextWindowSize(ImVec2(width, 40));
            ImGui::SetNextWindowBgAlpha(0.0f);
            resetTopToolbar = false;
        }
        ImGui::Begin("##TopToolbar", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);

        ImGui::End();
    }

    void SpriteLab::RenderToolBar()
    {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        int recentColoursCount = (selectedProject->recentColours.size() < 3) ? selectedProject->recentColours.size() : 3;
        vector<string> toolButtons = { "PaintBrush", "PaintBucket", "Eraser", "EyeDropper", "Line" };
        ImGui::SetNextWindowSize(ImVec2(40, 60 + (toolButtons.size() * 35) + (recentColoursCount * 31)));
        if (resetToolbar)
        {
            ImGui::SetNextWindowPos(ImVec2(60, height/2 - 83));
            ImGui::SetNextWindowBgAlpha(100.0f);
            resetToolbar = false;
        }
        ImGui::Begin("##Toolbar", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize);

        int nextYPos = 25;
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));
        for (const string& tool : toolButtons)
        {
            string active = "";
            if (selectedProject->selectedToolString == tool) active = "Active";
            ImGui::SetCursorPos(ImVec2(5, nextYPos));
            if (ImGui::ImageButton(textures[tool + active + "Icon"], ImVec2(30, 30), ImVec2(0, 0), ImVec2(1, 1), 0, ImVec4(0, 0, 0, 0)))
            {
                selectedProject->selectedTool = StringToTool(tool);
                selectedProject->selectedToolString = tool;
            } 
            nextYPos += 35;
        }
        ImGui::PopStyleColor(2);

        ImGui::SetCursorPos(ImVec2(5, nextYPos));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(static_cast<float>(selectedProject->brush.colour.r) / 255, static_cast<float>(selectedProject->brush.colour.g) / 255, static_cast<float>(selectedProject->brush.colour.b) / 255, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(static_cast<float>(selectedProject->brush.colour.r) / 255, static_cast<float>(selectedProject->brush.colour.g) / 255, static_cast<float>(selectedProject->brush.colour.b) / 255, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(static_cast<float>(selectedProject->brush.colour.r) / 255, static_cast<float>(selectedProject->brush.colour.g) / 255, static_cast<float>(selectedProject->brush.colour.b) / 255, 1));
        if (ImGui::Button("##ColourSelector", ImVec2(30,30)))
        {
            if (!colourPickerOpened)
            {
                colourPickerColour = ImVec4(static_cast<float>(selectedProject->brush.colour.r) / 255, static_cast<float>(selectedProject->brush.colour.g) / 255,
                    static_cast<float>(selectedProject->brush.colour.b) / 255, static_cast<float>(selectedProject->brush.colour.a) / 255);
                colourPickerOpened = true;
            }
            else colourPickerOpened = false;
        }
        ImGui::PopStyleColor(3);
        ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(255, 255, 255, 255), 0.0f, 0, 0.25f);
        nextYPos += 4;
        for (int i = 0; i < recentColoursCount; i++)
        {
            nextYPos += 31;
            ImGui::SetCursorPos(ImVec2(7, nextYPos));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(static_cast<float>(selectedProject->recentColours[i].r) / 255, static_cast<float>(selectedProject->recentColours[i].g) / 255, static_cast<float>(selectedProject->recentColours[i].b) / 255, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(static_cast<float>(selectedProject->recentColours[i].r) / 255, static_cast<float>(selectedProject->recentColours[i].g) / 255, static_cast<float>(selectedProject->recentColours[i].b) / 255, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(static_cast<float>(selectedProject->recentColours[i].r) / 255, static_cast<float>(selectedProject->recentColours[i].g) / 255, static_cast<float>(selectedProject->recentColours[i].b) / 255, 1));
            if (ImGui::Button(("##RecentColour" + to_string(i)).c_str(), ImVec2(26, 26))) 
            {
                SetBrushColour(selectedProject, &selectedProject->brush, { selectedProject->recentColours[i].r, selectedProject->recentColours[i].g, selectedProject->recentColours[i].b, selectedProject->brush.colour.a });
            }
            ImGui::PopStyleColor(3);
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), ImColor(255, 255, 255, 255), 0.0f, 0, 0.25f);
        }

        RenderColourPicker(ImVec2(ImGui::GetWindowPos().x * 2 - 10, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y));

        ImGui::End();
    }

    void SpriteLab::RenderColourPicker(ImVec2 pos)
    {
        if (!colourPickerOpened) return;
        if (resetColourPicker)
        {
            resetColourPicker = false;
            ImGui::SetNextWindowSize(ImVec2(250, 230));
            ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y - 125));
        }
        ImGui::Begin("Color Picker", &colourPickerOpened, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize);

        ImVec4 previousColor = ImVec4(static_cast<float>(selectedProject->brush.colour.r) / 255, static_cast<float>(selectedProject->brush.colour.g) / 255,
                static_cast<float>(selectedProject->brush.colour.b) / 255, static_cast<float>(selectedProject->brush.colour.a) / 255);

        ImGui::ColorPicker4("##ColourPicker", (float*)&colourPickerColour, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoLabel, (float*)&previousColor);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.16f, 0.16f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.40f, 0.40f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.40f, 0.40f, 0.40f, 1));
        ImGui::SetCursorPos(ImVec2(175, 162));
        if (ImGui::Button("Select", ImVec2(60, 20)))
        {
            SetBrushColour(selectedProject, &selectedProject->brush, { static_cast<unsigned char>(colourPickerColour.x * 255), static_cast<unsigned char>(colourPickerColour.y * 255), static_cast<unsigned char>(colourPickerColour.z * 255), static_cast<unsigned char>(colourPickerColour.w * 255) });
            colourPickerOpened = false;
        }

        ImGui::SetCursorPos(ImVec2(175, 192));
        if (ImGui::Button("Cancel", ImVec2(60, 20)))
        {
            colourPickerOpened = false;
        }
        ImGui::PopStyleColor(3);
        ImGui::End();
    }

    void SpriteLab::RenderProjectsMenu()
    {
        if (!renderProjectsMenu) return;
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        ImGui::SetNextWindowSize(ImVec2(300,400));
        //ImGui::SetNextWindowPos(ImVec2((float)(width - 300) / 2, (float)(height - 400) / 2));
        ImGui::SetNextWindowPos(ImVec2(0,0));

        ImGui::Begin("Projects Menu", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

        ImGui::SetCursorPos(ImVec2(75, 30));
        if (ImGui::Button("Create Project", ImVec2(150, 25)))
        {
            renderProjectsMenu = false;
            OpenCreateProjectMenu(false);
            SDL_SetWindowSize(window, 250, 250);
        }

        ImGui::SetCursorPos(ImVec2(75, 75));
        if (ImGui::Button("Open Project", ImVec2(150, 25)))
        {
            string path = LoadProjectDialog(window, renderer);
            if (path != "null")
            {
                renderProjectsMenu = false;
                projects.push_back(LoadProject(path));
                selectedProject = &projects.back();

                SDL_DisplayMode dm;
                SDL_GetDesktopDisplayMode(0, &dm);
                int width = dm.w;
                int height = dm.h - 50;
                SDL_SetWindowMinimumSize(window, 960, 540);
                SDL_SetWindowSize(window, width, height);
                SDL_SetWindowPosition(window, 0, 25);
            }
        }

        ImGui::SetCursorPos(ImVec2(0, 120));

        ImGui::Separator();

        ImGui::SetCursorPos(ImVec2(100, 125));

        ImGui::Text("Recent Projects");

        int nextY = 150;
        int recentProjectCount = 0;
        for (string projectPath : userSettings.recentProjects)
        {
            if (recentProjectCount >= 6) break;
            ImGui::SetCursorPos(ImVec2(20, nextY));
            if (!filesystem::exists(projectPath))
            {
                userSettings.recentProjects.erase(std::remove(userSettings.recentProjects.begin(), userSettings.recentProjects.end(), projectPath), userSettings.recentProjects.end());
                continue;
            }
            if (ImGui::Button(filesystem::path(projectPath).filename().string().c_str(), ImVec2(260, 30)))
            {
                Project project = LoadProject(projectPath);
                bool alreadyOpen = false;

                for (Project& project2 : projects)
                {
                    if (!project2.Compare(project)) continue;
                    alreadyOpen = true;
                    break;
                }

                if (!alreadyOpen)
                {
                    renderProjectsMenu = false;
                    projects.push_back(project);
                    selectedProject = &projects.back();
                    SpriteLab::selectedProject->projectSettings.size = ImVec2(10, 10);
                    SpriteLab::selectedProject->layers.push_back({});
                    SpriteLab::selectedProject->selectedLayer = &SpriteLab::selectedProject->layers[0];

                    SDL_DisplayMode dm;
                    SDL_GetDesktopDisplayMode(0, &dm);
                    int width = dm.w;
                    int height = dm.h - 50;
                    SDL_SetWindowMinimumSize(window, 960, 540);
                    SDL_SetWindowSize(window, width, height);
                    SDL_SetWindowPosition(window, 0, 25);
                }
            }
            nextY += 40;
            recentProjectCount++;
        }

        ImGui::End();
    }

    void SpriteLab::RenderCreateProjectMenu()
    {
        if (!renderCreateProjectMenu) return;
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        ImGui::SetNextWindowSize(ImVec2(250, 250));
        ImGui::SetNextWindowPos(ImVec2((float)(width - 250) / 2, (float)(height - 250) / 2));
        //ImGui::SetNextWindowPos(ImVec2(0, 0));

        ImGuiWindowFlags flags;
        if (createProjectShowTitleBar) flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
        else flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

        ImGui::Begin("Create Project", nullptr, flags);

        ImGui::SetCursorPos(ImVec2(50, 30));
        ImGui::Text("Name");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::SetCursorPosY(27);
        static char nameBuffer[256];
        ImGui::InputText("##Name", nameBuffer, sizeof(nameBuffer));

        ImGui::SetCursorPos(ImVec2(70, 65));
        ImGui::Text("Width");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        ImGui::SetCursorPosY(62);
        ImGui::InputInt("##Width", &tempWidth, 0, 0);

        ImGui::SetCursorPos(ImVec2(70, 100));
        ImGui::Text("Height");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(50);
        ImGui::SetCursorPosY(97);
        ImGui::InputInt("##Height", &tempHeight, 0, 0);

        ImGui::SetCursorPos(ImVec2(50, 150));
        if (ImGui::Button("Create", ImVec2(150, 25)) && tempWidth > 1 && tempHeight < 5000 && tempWidth > 1 && tempHeight < 5000)
        {
            string name = nameBuffer;
            
            auto isLegalCharacter = [](char c) {
                return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == ' ';
            };

            name.erase(std::remove_if(name.begin(), name.end(),
                [&isLegalCharacter](char c) { return !isLegalCharacter(c); }),
                name.end());

            string newName = name;
            newName.erase(std::remove_if(newName.begin(), newName.end(), ::isspace), newName.end());

            if (newName != "")
            {
                renderCreateProjectMenu = false;
                nameBuffer[0] = '\0';

                SpriteLab::projects.push_back({});
                SpriteLab::selectedProject = &SpriteLab::projects.back();

                SpriteLab::selectedProject->name = name;
                SpriteLab::selectedProject->projectSettings.size = ImVec2(tempWidth, tempHeight);
                SpriteLab::selectedProject->layers.push_back({});
                SpriteLab::selectedProject->selectedLayer = &SpriteLab::selectedProject->layers[0];

                userSettings.preferedCanvasSize = { tempWidth , tempHeight };
                SaveSettings();

                tempWidth = 100;
                tempHeight = 100;

                SDL_DisplayMode dm;
                SDL_GetDesktopDisplayMode(0, &dm);
                int width = dm.w;
                int height = dm.h - 50;
                SDL_SetWindowMinimumSize(window, 960, 540);
                SDL_SetWindowSize(window, width, height);
                SDL_SetWindowPosition(window, 0, 25);
            }
        }

        ImGui::SetCursorPos(ImVec2(50, 195));
        if (ImGui::Button("Cancel", ImVec2(150, 25)))
        {
            renderCreateProjectMenu = false;
            nameBuffer[0] = '\0';
            tempWidth = 100;
            tempHeight = 100;

            if (!selectedProject)
            {
                renderProjectsMenu = true;
                SDL_SetWindowSize(window, 300, 400);
            }
        }

        ImGui::End();
    }

    void SpriteLab::Render()
    {
        SDL_RenderClear(renderer);
        ImGui_ImplSDL2_NewFrame(window);
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui::NewFrame();

        if (selectedProject)
        {
            RenderBackground();
            RenderMenuBar();
            RenderCanvas();
            RenderLayersMenu();
            RenderToolBar();
            RenderTopToolBar();
            RenderProjectTabs();
        }

        RenderProjectsMenu();
        RenderCreateProjectMenu();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);

        for (SDL_Texture* texture : destroyTexture)
            SDL_DestroyTexture(texture);
        destroyTexture.clear();
    }

    void SpriteLab::InitImages()
    {
        int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_WEBP | IMG_INIT_AVIF | IMG_INIT_JXL | IMG_INIT_TIF;
        if (!(IMG_Init(imgFlags) & imgFlags)) {}

        //std::filesystem::path imagesPath = std::filesystem::path(__FILE__).parent_path() / "Images";

        for (const filesystem::path& entry : filesystem::directory_iterator(std::filesystem::path(__FILE__).parent_path() / "Images"))
        {
            if (!filesystem::is_regular_file(entry)) continue;
            SDL_Surface* surface = IMG_Load(entry.string().c_str());
            textures[entry.stem().string()] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            cout << "Image Loaded - " + entry.stem().string() << endl;
        }


        // Canvas Grey Background
        SDL_Surface* surface = SDL_CreateRGBSurface(0, 100, 100, 32, 0, 0, 0, 0);
        SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 40, 40, 40));
        textures["CanvasBackground"] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        // The texture that goes over invisible layers so they look faded.
        surface = SDL_CreateRGBSurfaceWithFormat(0, 100, 100, 32, SDL_PIXELFORMAT_RGBA8888);
        SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, 0, 0, 0, 100));
        textures["FadedLayerCover"] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    void SpriteLab::InitSDL()
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            printf("SDL_Init Error: %s\n", SDL_GetError());
            exit(1);
        }

        //SDL_DisplayMode dm;
        //SDL_GetDesktopDisplayMode(0, &dm);
        //int width = dm.w;
        //int height = dm.h - 50;

        window = SDL_CreateWindow("SpriteLab v0.1",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            300, 400,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        SDL_SetWindowResizable(window, SDL_FALSE);
        //SDL_SetWindowMinimumSize(window, 960, 540);

        if (!window) {
            printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
            SDL_Quit();
            exit(1);
        }

        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_ACCELERATED |
            SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            printf("SDL_CreateRenderer Error: %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            SDL_Quit();
            exit(1);
        }

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        //InitFonts();
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer_Init(renderer);

        ImGuiStyle& style = ImGui::GetStyle();

        //style.Colors[ImGuiCol_TitleBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);
        style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
        style.Colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.0f);

        ImVec4* colors = style.Colors;

        ImGui::StyleColorsDark();

        colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.23f, 0.23f, 0.23f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.13f, 0.13f, 0.13f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.55f, 0.55f, 0.55f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.65f, 0.65f, 0.65f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

        colors[ImGuiCol_Header] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];

        colors[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.22f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);

        //colors[ImGuiCol_DropdownBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        //colors[ImGuiCol_DropdownHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        //colors[ImGuiCol_DropdownActive] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);

        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);

        colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    }

    void SpriteLab::Cleanup()
    {
        for (const filesystem::path& entry : filesystem::directory_iterator(std::filesystem::path(__FILE__).parent_path() / "Images"))
            SDL_DestroyTexture(textures[entry.stem().string()]);
        SDL_DestroyTexture(textures["CanvasBackground"]);
        SDL_DestroyTexture(textures["FadedLayerCover"]);

        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void SpriteLab::ShortcutKeys()
    {
        if (savePopupOpened || renderCreateProjectMenu || renderProjectsMenu) return;
        if (keysPressed.count(SDLK_LCTRL))
        {
            if (keysPressed.count(SDLK_LSHIFT))
            {
                if (keysPressed.count(SDLK_s)) ProjectSaveAs(window, renderer, selectedProject);
                else if (keysPressed.count(SDLK_e)) ProjectExportAs(window, renderer, selectedProject);
            }
            else if (keysPressed.count(SDLK_s)) ProjectSave(window, renderer, selectedProject);
            else if (keysPressed.count(SDLK_e)) ProjectExport(window, renderer, selectedProject);
            else if (keysPressed.count(SDLK_n)) OpenCreateProjectMenu(true);
        }
    }

    void SpriteLab::ProcessEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT && !savePopupOpened)
            {
                SaveSettings();
                exit(0);
            }
            else if (event.type == SDL_KEYUP) keysPressed.erase(event.key.keysym.sym);
            else if (event.type == SDL_KEYDOWN)
            {
                keysPressed.insert(event.key.keysym.sym);
                ShortcutKeys();
            }
        }
    }
}

int main(int argc, char* argv[])
{
    SpriteLab::InitSDL();
    SpriteLab::InitImages();

    SpriteLab::renderProjectsMenu = true;
    SpriteLab::LoadSettings();

    while (true)
    {
        SpriteLab::ProcessEvents();
        SpriteLab::Render();
    }

    SpriteLab::Cleanup();
    return 0;
}
