#include "SpriteLab.h"
#include <filesystem>
#include <unordered_map>

using namespace std;

// TODO
// ZOOM BAR AT BOTTOM RIGHT LIKE MS WORD. Make the minimum and maximum depend on the size of it.
// Make all sizes work rather than just 100x100 and if size is bigger than canvas size, then zoom out also zoom in if small; scale to size where it fits canvas.
// I don't think SetBaseCanvasZoom will work if the scale is bigger than the canvas window size, it might set the zoom to 0.
// Switch structs to classes

namespace SpriteLab
{
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    unordered_map<string, SDL_Texture*> textures;
    bool bestZoomSet = false;

    struct ProjectSettings
    {
        ImVec2 size;
        float zoom = 1;
    }; ProjectSettings projectSettings;

    struct Brush
    {
        ImU32 colour = IM_COL32(255,255,255,255);
        int size = 1;
    }; Brush brush;

    struct PairHash {
        template <class T1, class T2>
        std::size_t operator () (const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ h2;
        }
    };

    struct Pixel
    {
        ImVec2 position;
        ImU32 colour = IM_COL32(255, 255, 255, 255);
    }; unordered_map<pair<int, int>, Pixel, PairHash> pixels;

    enum Tools
    {
        PaintBrush
    }; Tools selectedTool = PaintBrush;

    float SpriteLab::GetBestCanvasZoom()
    {
        int x = floor(ImGui::GetWindowSize().x / projectSettings.size.x);
        int y = floor(ImGui::GetWindowSize().y / projectSettings.size.y);
        return (x < y) ? x : y;
    }

    void SpriteLab::RenderMenuBar()
    {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("MenuBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBringToFrontOnFocus);

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Open Project", "Ctrl+O")) {}
                if (ImGui::MenuItem("Save Project", "Ctrl+S")) {}
                if (ImGui::MenuItem("Build Project", "")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Project Settings", "")) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Help")) {
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::End();
    }

    void SpriteLab::RenderBackground()
    {
        // Canvas background
        ImTextureID bgTextureId = (ImTextureID)textures["CanvasBackground"];
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 prevPadding = style.WindowPadding;
        style.WindowPadding = ImVec2(0, 0);
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::Image((ImTextureID)bgTextureId, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));

        style.WindowPadding = prevPadding;
    }

    void SpriteLab::RenderCanvas()
    {
        int width, height;
        SDL_GetWindowSize(window, &width, &height);
        ImGui::SetNextWindowPos(ImVec2(50, 50));
        ImGui::SetNextWindowSize(ImVec2(width - 100, height - 100));
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
        if (!bestZoomSet)
        {
            projectSettings.zoom = GetBestCanvasZoom();
            bestZoomSet = true;
        }

        RenderBackground();

        ImVec2 size = ImVec2(projectSettings.size.x * projectSettings.zoom, projectSettings.size.y * projectSettings.zoom);
        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - (projectSettings.size.x * projectSettings.zoom) / 2, ImGui::GetWindowSize().y / 2 - (projectSettings.size.y * projectSettings.zoom) / 2));
        ImGui::BeginChild("RealCanvas", size, false, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::Image((ImTextureID)textures["TransparentBackground"], size, ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));

        if (selectedTool == SpriteLab::PaintBrush)
        {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 canvasPos = ImGui::GetCursorScreenPos();

            int cellSize = projectSettings.zoom;
            int gridX = static_cast<int>((mousePos.x - canvasPos.x) / cellSize);
            int gridY = static_cast<int>((mousePos.y - canvasPos.y) / cellSize);

            ImVec2 pixelMin(canvasPos.x + gridX * cellSize, canvasPos.y + gridY * cellSize - cellSize - 5);
            ImVec2 pixelMax(canvasPos.x + (gridX + 1) * cellSize, canvasPos.y + (gridY + 1) * cellSize - cellSize - 5);

            ImGui::GetWindowDrawList()->AddRectFilled(pixelMin, pixelMax, IM_COL32(255, 0, 0, 255));

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(ImGui::GetWindowPos(), ImVec2(ImGui::GetWindowPos().x + size.x, ImGui::GetWindowPos().y + size.y)))
            {
                for (int y = 0; y < cellSize; y++)
                {
                    for (int x = 0; x < cellSize; x++)
                    {
                        ImVec2 pixelPos(canvasPos.x + gridX * cellSize + x, canvasPos.y + gridY * cellSize + y - cellSize - 5);
                        pixels[make_pair(pixelPos.x, pixelPos.y)] = (Pixel{ pixelPos, IM_COL32(255, 0, 0, 255) });
                    }
                }
            }
        }

        for (const auto& pixel : pixels)
        {
            ImGui::GetWindowDrawList()->AddRectFilled(pixel.second.position, ImVec2(pixel.second.position.x+1, pixel.second.position.y+1), IM_COL32(255, 0, 0, 255));
        }

        ImGui::EndChild();

        ImGui::End();
    }

    void SpriteLab::Render()
    {
        ImGui_ImplSDL2_NewFrame(window);
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui::NewFrame();

        RenderMenuBar();
        RenderCanvas();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    void SpriteLab::InitImages()
    {
        int imgFlags = IMG_INIT_PNG | IMG_INIT_JPG | IMG_INIT_WEBP | IMG_INIT_AVIF | IMG_INIT_JXL | IMG_INIT_TIF;
        if (!(IMG_Init(imgFlags) & imgFlags)) {}

        std::filesystem::path imagesPath = std::filesystem::path(__FILE__).parent_path() / "Images";

        for (const filesystem::path& entry : filesystem::directory_iterator(std::filesystem::path(__FILE__).parent_path() / "Images"))
        {
            if (!filesystem::is_regular_file(entry)) continue;
            SDL_Surface* surface = IMG_Load(entry.string().c_str());
            textures[entry.stem().string()] = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_FreeSurface(surface);
            cout << "Image Loaded - " + entry.stem().string() << endl;
        }


        // Canvas Grey Background
        SDL_Surface* surface = SDL_CreateRGBSurface(0, 1920, 1080, 32, 0, 0, 0, 0);
        Uint32 blueColor = SDL_MapRGB(surface->format, 40, 40, 40);
        SDL_FillRect(surface, NULL, blueColor);
        textures["CanvasBackground"] = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);
    }

    void SpriteLab::InitSDL()
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
            printf("SDL_Init Error: %s\n", SDL_GetError());
            exit(1);
        }

        SDL_DisplayMode dm;
        SDL_GetDesktopDisplayMode(0, &dm);
        int width = dm.w;
        int height = dm.h - 50;

        window = SDL_CreateWindow("SpriteLab v0.1",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width, height,
            SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
        //SDL_SetWindowResizable(window, SDL_FALSE);
        SDL_SetWindowMinimumSize(window, 960, 540);

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

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigWindowsMoveFromTitleBarOnly = true;
        //InitFonts();
        ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer_Init(renderer);

        ImGui::StyleColorsDark();
    }

    void SpriteLab::Cleanup()
    {
        ImGui_ImplSDLRenderer_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }

    void SpriteLab::ProcessEvents()
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                exit(0);
            }
        }
    }
}

int main(int argc, char* argv[])
{
    SpriteLab::InitSDL();
    SpriteLab::InitImages();

    SpriteLab::projectSettings.size = ImVec2(10,10);

    while (true) {
        SpriteLab::ProcessEvents();
        SpriteLab::Render();
    }

    SpriteLab::Cleanup();
    return 0;
}
