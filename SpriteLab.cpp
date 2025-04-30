#include "SpriteLab.h"
#include <filesystem>
#include <unordered_map>
#define __STDC_LIB_EXT1__
#define STB_IMAGE_WRITE_IMPLEMENTATION    
#include "ProjectFileManager.h"
#include "FontManager.h"
#include <queue>

using namespace std;

// TODO
// Make sure if brush size is greater than one and if a pixel drawn is off screen, dont draw it
// Properly maximize window
// Add support for resizing
// Redo Project Create menu on Projects Menu Screen. Maybe put the Project Start menu in the editor, but block/hide things like the toolbar, and in the canvas area make it
//  so thats where the buttons to open projec, create project, and recent projects are.
// ZOOM BAR AT BOTTOM RIGHT LIKE MS WORD. Make the minimum and maximum depend on the size of it.
// Make all sizes work rather than just 100x100 and if size is bigger than canvas size, then zoom out also zoom in if small; scale to size where it fits canvas.
// I don't think SetBaseCanvasZoom will work if the scale is bigger than the canvas window size, it might set the zoom to 0.
// Switch structs to classes
// Change everything from ImGui to SDL other than gui such as ImVec2 to SDL_Point
// If pixel is too dark, change outline of tools like eraser to white
// Add shortcut keys to tools
// When exporting to JPG with a transparent image, pop up saying that JPG doesn't support transparent images. And recommend switching to PNG, BMP, or TGA 
//  and say the transparent pixels will be replaced with white. and say with 3 buttons, "Continue", "Cancel", "Export to PNG"
// Option to reduce quality of exported images like JPG (stb_image has parameter for it)
// Add CTRL Z, CTRL A, CTRL C, SHIFT CTRL Z, CTRL D, CTRL V, CTRL S, SHIFT CTRL S, etc
// FOR LAYERS HAVE PIXEL VARIABLE FOR EACH ONE, THEN ANOTHER PIXEL VARIABLE FOR PIXELS THAT ARE VISIBBLE. DONT RENDER PIXELS THAT CANT BE SEEN.
// Support for .spl drag and drop & double clicking
// Up and down arrow key for selecting layer
// Add alpha blending to exported image
// Support importing files from other sprite editors like aseprite, photoshop, etc (make sure to add double click file and drag & drop too)
// Make it so user can have multiple projects open at once.
// Store if the file was already saved and if it was, then dont open dialog to save. Save this cross session in project settings
// when closing, it will need to check all projects opened and say "Do you want to save xxxxx before closing"
// In the installer program, make it so it sets .spl file application to SpriteLab. Maybe others like .ase but unchecked by default
// Make a confirmation popup if user is trying to open project that is already open. Maybe just take them to the project.
// When rendering pixel to canvas, see if I should use relativePos instead of rect
// Remove lastSaveName and lastExportName, and make lastExportLocation and lastSaveLocation paths.
// Make colour picker look nice like https://github.com/ocornut/imgui/issues/346
// Change toolbar button colours to use ColorButton
// When a project is created, save the width and height, so when creating new projects, the width and height will default to what they used previously
// In .spl files, at the top, add SPL save version so incase if the user is trying to open a older spl file, it will try to convert if it cant, it will tell them which version
//  the file is for
// Instead of defining the SDL_WindowSize is a lot of the Render Functions, just define it in the Render() and then pass the values through parameters
// Bucket tool crashes program if there are a lot of pixels like 100x100
// If the save json is unable to parse, send error saying file got corrupted or was saved on a different version (although if we add version checks then not) rather than letting it crash
// Remove selectedProject->lastSaveName
// Make it so SetSetting takes a json instead of 2 strings so things like setting last width and height doesn't need to open and close the file twice & consider encoding data
// Load .settings into ram, maybe remove GetSetting() & SetSetting() (unless I still want to Set settings to files realtime), and LoadSettings() and SaveSettings(); 
// If there are too many tabs opened, some will go off screen.
// Fix canvas from being off centered with correct numbers. I think its because it assumes the top and bottom borders are the same height
// Multi-threading
// When hovering over tool, popup information about it
// With EyeDropper tool and hovering over canvas, popup colour w/ RGBA
// Dragable layers & projects tab so the order of them can be changed
// Add close project & add the functionality to Menu, Shortcut, and Tab X
// When right click layer, option to delete, duplicate, copy, etc. Also if you click on a layer and do shortcut keys, it will copy, paste, duplicate, delete, etc. Make sure to have popups saying it cant delete the only layer and stuff
// Add groups for layers
// Export spritesheet
// Potentially add layer names under tha layer preview in Layer Menu and maybe a button at the bottom of the layer menu to enable and disable layer names
// Todo: Add pixel blending for same layer. Example: if a 50% black alpha is drawn on the pixel it will draw 50% black. If its drawn again, it will go to 100% black
// Maybe instead of having to click "Select" in the colour picker, when the user clicks out of the colour picker, it will select that color
// Todo: Fix the recent projects to remove projects with different caps. Currently it only deletes one with the exact same string (including caps)
//  also, it seems like the path from recent and open menu are formatted differently or something which also gives duplicates it in recent project.
// When clicking UI, make sure it doesn't detect a click on the canvas
// Prefered Canvas saving seems to be saving or loading the Height as Width
// Duplicate, delete, copy, etc layers
// Doesnt have garbage collection. "When you allocate memory dynamically using new, you are responsible for freeing that memory when you're done with it using delete. Failing to do so can lead to memory leaks, where memory is allocated but not released, resulting in your program using more and more memory over time."

// When a project is SaveAs & Opened, it will save it as a recent project. Create a OpenProject() ALSO LIMIT IT TO 10

/*
------ STB IMAGE SAYS THIS -----
The PNG output is not optimal; it is 20-50% larger than the file
   written by a decent optimizing implementation; though providing a custom
   zlib compress function (see STBIW_ZLIB_COMPRESS) can mitigate that.
   This library is designed for source code compactness and simplicity,
   not optimal image file size or run-time performance.
*/

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
                if (ImGui::BeginMenu("Open Recent")) // Todo: Add name length limit and maybe don't load each recent project to reduce resource usage.
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
        // Canvas background
        //ImTextureID bgTextureId = (ImTextureID)textures["CanvasBackground"];
        //ImVec2 windowPos = ImGui::GetWindowPos();
        //ImVec2 windowSize = ImGui::GetWindowSize();
        //ImGuiStyle& style = ImGui::GetStyle();
        //ImVec2 prevPadding = style.WindowPadding;
        //style.WindowPadding = ImVec2(0, 0);
        //ImGui::SetCursorPos(ImVec2(0, 0));
        //ImGui::Image((ImTextureID)bgTextureId, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
        //style.WindowPadding = prevPadding;
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        SDL_Rect dstrect = { 50, 90, width - 50, height - 140};
        //SDL_RenderClear(renderer);
        //SDL_Rect dstrect = { 0, 0, 1920 , 1080 };
        SDL_RenderCopy(renderer, textures["CanvasBackground"], NULL, &dstrect);
    }

    void BucketToolFill(Pixel& startPixel, SDL_Color fillColor, ImVec2 size, int mouseX, int mouseY) {
        // Bounds check for the starting pixel
        if (startPixel.relativePos.x < 0 ||
            startPixel.relativePos.x >(selectedProject->projectSettings.size.x - 1) ||
            startPixel.relativePos.y < 0 ||
            startPixel.relativePos.y >(selectedProject->projectSettings.size.y - 1)) {
            return;
        }

        // Get the color we're replacing
        SDL_Color targetColor;

        if (startPixel.exists) {
            targetColor = startPixel.colour;
        }
        else {
            targetColor = { 0, 0, 0, 0 }; // Assuming transparent color
        }

        // If target color is already the fill color, no need to fill
        if (CompareColor(targetColor, fillColor)) {
            return;
        }

        // Use a queue for breadth-first fill approach
        std::queue<std::pair<int, int>> pixelsToFill;
        pixelsToFill.push(make_pair(startPixel.relativePos.x, startPixel.relativePos.y));

        // Calculate pixel size
        int xDifference = ((selectedProject->canvasMaxPos.x - selectedProject->canvasMinPos.x) /
            selectedProject->projectSettings.size.x);
        int yDifference = ((selectedProject->canvasMaxPos.y - selectedProject->canvasMinPos.y) /
            selectedProject->projectSettings.size.y);

        // Directions: up, down, right, left
        const int dx[4] = { 0, 0, 1, -1 };
        const int dy[4] = { -1, 1, 0, 0 };

        // Process pixels until queue is empty
        while (!pixelsToFill.empty()) {
            // Get the next pixel to process
            auto currentPos = pixelsToFill.front();
            pixelsToFill.pop();

            int x = currentPos.first;
            int y = currentPos.second;

            // Skip if outside bounds
            if (x < 0 || x >(selectedProject->projectSettings.size.x - 1) ||
                y < 0 || y >(selectedProject->projectSettings.size.y - 1)) {
                continue;
            }

            // Check if this pixel needs to be filled
            bool needsFill = false;
            auto pixelPair = make_pair(x, y);

            if (selectedProject->selectedLayer->pixels.find(pixelPair) !=
                selectedProject->selectedLayer->pixels.end()) {
                // Existing pixel - check if it's the target color
                if (CompareColor(targetColor, selectedProject->selectedLayer->pixels[pixelPair].colour)) {
                    needsFill = true;
                }
            }
            else {
                // Non-existing pixel - should be filled if target is transparent
                if (!startPixel.exists) {
                    needsFill = true;
                }
            }

            if (needsFill) {
                // Fill this pixel
                if (selectedProject->selectedLayer->pixels.find(pixelPair) !=
                    selectedProject->selectedLayer->pixels.end()) {
                    // Update existing pixel
                    selectedProject->selectedLayer->pixels[pixelPair].colour = fillColor;
                }
                else {
                    // Create new pixel
                    Pixel newPixel = {
                        {selectedProject->canvasMinPos.x + (x * xDifference),
                         selectedProject->canvasMinPos.y + (y * yDifference),
                         xDifference, yDifference},
                        {x, y},
                        fillColor,
                        true
                    };
                    selectedProject->selectedLayer->pixels[pixelPair] = newPixel;
                }

                // Add neighboring pixels to the queue
                for (int i = 0; i < 4; i++) {
                    int newX = x + dx[i];
                    int newY = y + dy[i];
                    pixelsToFill.push(make_pair(newX, newY));
                }
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

    void LineToolFill(Pixel pixel, SDL_Point endPoint, SDL_Color color) // Todo: Fix it so its more accurate if the line isn't straight or 45 degree angle.
    { 
        //I think I should rely on math rather than for every pixel, check which is closest to the end point and keep doing that to get line since it doesn't make the line straight.
            //So what I think I will need to do is come up with a math formula that will take 2 SDL_Point's and then do calculations to see of all numbers in between to comeup with a straight line.
        // I think this is trigonometry/Pythagorean theorem. SOH-CAH-TOA. Find the slope. Maybe use Opposite and Adjacent to find Hypotenuse. Then divide that by the pixels between or something. Each pixel it will add the value together.
        // http://www.softwareandfinance.com/Visual_CPP/VCPP_Equation_of_a_line.html
        // https://www.google.com/search?client=opera-gx&q=cpp+straight+line+from+2+points+math&sourceid=opera&ie=UTF-8&oe=UTF-8
        // https://www.tutorialspoint.com/program-to-find-line-passing-through-2-points-in-cplusplus
        // https://math.stackexchange.com/questions/2158172/formula-of-the-straight-line-through-two-points
        // https://www.codeproject.com/Questions/224182/Get-all-points-in-a-Line
        // https://www.google.com/search?client=opera-gx&q=cpp+straight+line+from+2+points+math&sourceid=opera&ie=UTF-8&oe=UTF-8

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

        // The below would also work. Check what is more efficent
        //for (int y = -1; y <= 1; y++)
        //{
        //    for (int x = -1; x <= 1; x++)
        //    {
        //        if (x == 0 && y == 0)
        //        {
        //            continue;
        //        }

        //        int xDifference = (selectedProject->canvasMaxPos.x - selectedProject->canvasMinPos.x) / selectedProject->projectSettings.size.x;
        //        int yDifference = (selectedProject->canvasMaxPos.y - selectedProject->canvasMinPos.y) / selectedProject->projectSettings.size.y;

        //        pair pixelPair = make_pair(pixel.relativePos.x + x, pixel.relativePos.y + y);
        //        if (selectedProject->selectedLayer->pixels.find(pixelPair) != selectedProject->selectedLayer->pixels.end())
        //        {
        //            pixels.push_back(selectedProject->selectedLayer->pixels[pixelPair]);
        //        }
        //        else if (!pixel.exists)
        //        {
        //            pixels.push_back({ {pixel.rect.x + (x * xDifference), pixel.rect.y + (y * yDifference), pixel.rect.w, pixel.rect.h}, {pixel.relativePos.x + x, pixel.relativePos.y + y}, {0, 0, 0, 0}, false });
        //        }
        //    }
        //}

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

    // -------------- Todo: Doesn't let the user start the line on the very left pixels --------------
    void LineTool(int mouseX, int mouseY, int width, ImVec2 size) // Todo: Check all 6 directions and determine which 2-3 to use so then the other ones wont be calculated as distance since we know its not in that direciton
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
        // ------- Either I can save the selectedProject->layers as textures then load them on to buttons, or render them pixel by pixel in child windows -------
        // Texture may be best although I should update texture only after not making any changes for 1 second. Although check performance difference because it seems fine rn
        //  but photoshop waits until the user lifts mouse button, maybe that is how I should do it. Once the user lifts mouse button from painting, render the new canvas
        // todo: Create the texture in RenderCanvas() since pixels are already being looped through, no need to do it twice

        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        if (resetLayersMenu)
        {
            // Todo: Check if toolbar is in same place and if it is, recalculate position to acount for recent colour buttons (or maybe if when its
            //  adding the recent colors buttons check if its the same position and store in a variable if it has been changed.
            ImGui::SetNextWindowPos(ImVec2(width-200, 90));
            ImGui::SetNextWindowSize(ImVec2(200, height-140));
            resetLayersMenu = false;
        }
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
        ImGui::Begin("##LayersMenu", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);

        int nextYPos = 20;
        int index = 0;
        //if (!selectedProject)
        //{
        //    ImGui::End();
        //    ImGui::PopStyleColor();
        //    return;
        //}
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
            if (layerHovered && selectedProject->layers.size() > 1) // Todo: Delete button won't show up for selected project
            {
                ImGui::SetCursorPos(ImVec2(30, nextYPos + 5));
                ImGui::Image(textures["DeleteIcon"], ImVec2(25, 25));
                if (ImGui::IsItemClicked(0)) // Todo: Add a popup asking if the user is sure they want to delete the layer
                {
                    // Todo: Fix this so when the user deletes a layer and its not the project the user selected, keep it at that project. Currently I am setting it to 0 to fix an issue.
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
        //if (!selectedProject) return;
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        ImGui::SetNextWindowPos(ImVec2(50, 90));
        ImGui::SetNextWindowSize(ImVec2(width - 100, height - 140));
        int diff = 20; // The difference between the top of the canvas to the top of the window, and the bottom of the canvas to the bottom of the window. Should do math rather than hardcode this. Will need to fix this in other places too.
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
        for (Project& project : projects) // Make the X highlight when hovered
        {
            // ---------------------- Todo: Make selected project high lighted ----------------------
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
                // ------------------ Todo: Ask if user wants to save before closing also do something if the project is the only one opened ------------------
                // Todo: Fix this so when the user deletes a project and its not the project the user selected, keep it at that project. Currently I am setting it to 0 to fix an issue.
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

        if (selectedProject->selectedTool == PaintBrush || selectedProject->selectedTool == PaintBucket || selectedProject->selectedTool == Line )
        {
            int pushNextItem = 0;
            if (selectedProject->selectedTool != PaintBucket)
            {
                //totalWidth += 200;
                ImGui::SetCursorPos(ImVec2((width / 2) - 100 - 90, 10));
                ImGui::Text("Size");
                ImGui::SameLine();
                ImGui::PushTextWrapPos(0.0f);
                char xBuffer[256] = {};
                sprintf_s(xBuffer, sizeof(xBuffer), "%d", selectedProject->brush.size);
                ImGui::SetNextItemWidth(50);
                if (ImGui::InputText("##brushSize", xBuffer, sizeof(xBuffer), ImGuiInputTextFlags_EnterReturnsTrue) && round(static_cast<unsigned char>(stof(xBuffer))) > 0) // Todo: Can probably remove round here
                    selectedProject->brush.size = round(static_cast<unsigned char>(stof(xBuffer)));
                ImGui::PopTextWrapPos();
                pushNextItem = 120;
            }
            ImGui::SetCursorPos(ImVec2((width / 2) - 110 + pushNextItem, 10));
            ImGui::Text("Opacity");
            ImGui::SameLine();
            ImGui::PushTextWrapPos(0.0f);
            char xBuffer[256] = {};
            sprintf_s(xBuffer, sizeof(xBuffer), "%d", selectedProject->brush.colour.a);
            ImGui::SetNextItemWidth(50);
            if (ImGui::InputText("##brushOpacity", xBuffer, sizeof(xBuffer), ImGuiInputTextFlags_EnterReturnsTrue) && round(static_cast<unsigned char>(stof(xBuffer))) >= 0 && static_cast<unsigned char>(stof(xBuffer)) <= 255)
                SetBrushColour(selectedProject, &selectedProject->brush, { selectedProject->brush.colour.a, selectedProject->brush.colour.g, selectedProject->brush.colour.b, static_cast<unsigned char>(stof(xBuffer)) });
            ImGui::PopTextWrapPos();
        }

        ImGui::End();
    }

    void SpriteLab::RenderToolBar()
    {
        //if (!selectedProject) return;
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        int recentColoursCount = (selectedProject->recentColours.size() < 3) ? selectedProject->recentColours.size() : 3;
        vector<string> toolButtons = { "PaintBrush", "PaintBucket", "Eraser", "EyeDropper", "Line" };
        ImGui::SetNextWindowSize(ImVec2(40, 60 + (toolButtons.size() * 35) + (recentColoursCount * 31)));
        if (resetToolbar)
        {
            // Todo: Check if toolbar is in same place and if it is, recalculate position
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
        for (string projectPath : userSettings.recentProjects) // Todo: Add name length limit
        {
            if (recentProjectCount >= 6) break;
            ImGui::SetCursorPos(ImVec2(20, nextY));
            if (!filesystem::exists(projectPath))
            {
                userSettings.recentProjects.erase(std::remove(userSettings.recentProjects.begin(), userSettings.recentProjects.end(), projectPath), userSettings.recentProjects.end());
                continue;
            }
            if (ImGui::Button(filesystem::path(projectPath).filename().string().c_str(), ImVec2(260, 30))) // Todo: Maybe don't load each recent project to reduce resource usage.
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

    void SpriteLab::RenderNewProjectsMenu()
    {
        if (!renderProjectsMenu) return;
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(800, 600));
        ImGui::Begin("Projects Menu", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::SetCursorPos(ImVec2(230, 30));
        ImGui::PushFont(FontManager::GetFont("BoldMarker", 90, false));
        ImGui::Text("SpriteLab");
        ImGui::PopFont();

        ImGui::PushFont(FontManager::GetFont("Familiar-Pro-Bold", 30, false));
        ImGui::SetCursorPos(ImVec2(150, 150));
        if (ImGui::Button("Create Project", ImVec2(200, 75)))
        {
            renderProjectsMenu = false;
            OpenCreateProjectMenu(false);
            SDL_SetWindowSize(window, 250, 250);
        }

        ImGui::SetCursorPos(ImVec2(440, 150));
        if (ImGui::Button("Open Project", ImVec2(200, 75)))
        {
            string path = LoadProjectDialog(window, renderer);
            if (path != "null")
            {
                SDL_MaximizeWindow(window);
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
        ImGui::PopFont();

        ImGui::NewLine();
        ImGui::Separator();

        ImGui::SetCursorPos(ImVec2(275, 262));
        ImGui::PushFont(FontManager::GetFont("Familiar-Pro-Bold", 40, false));
        ImGui::Text("Recent Projects");
        ImGui::PopFont();


        int nextY = 315;
        int recentProjectCount = 0;
        for (string projectPath : userSettings.recentProjects) // Todo: Add name length limit
        {
            if (recentProjectCount >= 7) break;
            ImGui::SetCursorPos(ImVec2(272, nextY));
            if (!filesystem::exists(projectPath))
            {
                userSettings.recentProjects.erase(std::remove(userSettings.recentProjects.begin(), userSettings.recentProjects.end(), projectPath), userSettings.recentProjects.end());
                continue;
            }
            if (ImGui::Button(filesystem::path(projectPath).filename().string().c_str(), ImVec2(260, 30))) // Todo: Maybe don't load each recent project to reduce resource usage.
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
        ImGui::SetNextWindowBgAlpha(0.0f);
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
                if (projects.empty())
                    SDL_MaximizeWindow(window);

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
                SDL_SetWindowSize(window, 800, 600);
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

        //RenderProjectsMenu();
        RenderNewProjectsMenu();
        RenderCreateProjectMenu();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);

        for (SDL_Texture* texture : destroyTexture)
            SDL_DestroyTexture(texture);
        destroyTexture.clear();
    }

    void InitFonts()
    {
        FontManager::InitFontManager();
        // Todo: Make sure each of the fonts below are being used.
        FontManager::LoadFonts("Familiar-Pro-Bold", { 15, 18, 20, 25, 30, 40 });
        FontManager::LoadFont("BoldMarker", 90);
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
            //cout << "Image Loaded - " + entry.stem().string() << endl;
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
            800, 600,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        SDL_SetWindowResizable(window, SDL_FALSE);
        // 300, 400 W and H for old design
        // 
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
        InitFonts();
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
                // Todo: Check if changes were made since last save, if there were, then open save popup
                SaveSettings();
                exit(0);
            }
            else if (event.type == SDL_KEYUP) keysPressed.erase(event.key.keysym.sym);
            else if (event.type == SDL_KEYDOWN)
            {
                keysPressed.insert(event.key.keysym.sym);
                ShortcutKeys();
            }
            //else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) leftMouseButtonPressed = true;
            //else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) leftMouseButtonPressed = false;
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
