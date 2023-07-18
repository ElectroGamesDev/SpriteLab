#pragma once
#ifndef SPRITELAB_H
#define SPRITELAB_H

#include <iostream>
#include <SDL.h>
#include <SDL_render.h>
#include <SDL_image.h>
#include <stdio.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer.h"
#include <ImGuiFileDialog/ImGuiFileDialog.h>
#include <imgui_internal.h>

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

namespace SpriteLab
{
    void Render();
    void InitImages();
    void InitSDL();
    void Cleanup();
    void ProcessEvents();

    void RenderMenuBar();
    void RenderCanvas();
    void RenderBackground();

    float GetBestCanvasZoom();
}
int main(int argc, char* argv[]);

#endif // SPRITELAB_H
