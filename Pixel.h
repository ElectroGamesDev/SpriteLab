#pragma once
#ifndef PIXEL_H
#define PIXEL_H
#include <SDL.h>

namespace SpriteLab
{
    struct Pixel
    {
        SDL_Rect rect;
        SDL_Point relativePos;
        SDL_Color colour = SDL_Color{ 255, 255, 255, 255 };
        bool exists = true;
    };
}

#endif