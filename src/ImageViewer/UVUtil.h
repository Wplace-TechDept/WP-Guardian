#pragma once
#include "imgui.h"

namespace UVUtil {
    inline void CalcUVForAspect(float imgAspect, float winAspect, ImVec2& uv0, ImVec2& uv1) {
        ImVec2 uvCenter((uv0.x + uv1.x) * 0.5f, (uv0.y + uv1.y) * 0.5f);
        float uvWidth = uv1.x - uv0.x;
        float uvHeight = uv1.y - uv0.y;

        if (imgAspect < winAspect)
        {
            float newUvWidth = (winAspect / imgAspect) * uvHeight;
            uv0.x = uvCenter.x - newUvWidth * 0.5f;
            uv1.x = uvCenter.x + newUvWidth * 0.5f;
        }
        else if (imgAspect > winAspect)
        {
            float newUvHeight = (imgAspect / winAspect) * uvWidth;
            uv0.y = uvCenter.y - newUvHeight * 0.5f;
            uv1.y = uvCenter.y + newUvHeight * 0.5f;
        }
    }
    enum class ZoomDir
    {
        ZoomIn,
        ZoomOut
    };
    inline void ApplyZoom(ImVec2& uv0, ImVec2& uv1, const ImVec2& origin, ZoomDir dir, float factor = 1.1f){
        float zoom = (dir == ZoomDir::ZoomOut) ? factor : (1.0f / factor);
        uv0.x = (uv0.x - origin.x) * zoom + origin.x;
        uv0.y = (uv0.y - origin.y) * zoom + origin.y;
        uv1.x = (uv1.x - origin.x) * zoom + origin.x;
        uv1.y = (uv1.y - origin.y) * zoom + origin.y;
    }
    inline ImVec2 ScreenPosToUV(
        ImVec2 pos,
        ImVec2 rectMin,
        ImVec2 rectSize,
        ImVec2 uv0,
        ImVec2 uv1
    )
    {
        ImVec2 rel{
            pos.x - rectMin.x,
            pos.y - rectMin.y
        };

        ImVec2 t{
            rel.x / rectSize.x,
            rel.y / rectSize.y
        };

        return ImVec2{
            uv0.x + (uv1.x - uv0.x) * t.x,
            uv0.y + (uv1.y - uv0.y) * t.y
        };
    }

}

