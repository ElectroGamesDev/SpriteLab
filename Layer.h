#pragma once
#ifndef LAYER_H
#define LAYER_H
#include "Pixel.h"
#include <unordered_map>

namespace SpriteLab
{
    struct PairHash {
        template <class T1, class T2>
        std::size_t operator () (const std::pair<T1, T2>& p) const {
            auto h1 = std::hash<T1>{}(p.first);
            auto h2 = std::hash<T2>{}(p.second);
            return h1 ^ h2;
        }
    };

    struct Layer
    {
        std::unordered_map<std::pair<int, int>, Pixel, PairHash> pixels;
        bool visible = true;
    };
}

#endif