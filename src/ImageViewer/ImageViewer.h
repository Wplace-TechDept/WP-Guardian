#pragma once
#include "imgui.h"
#include "UVUtil.h"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

struct ImageViewEvent {
    bool zoomed = false;
    bool panned = false;
    bool reset = false;

    ImVec2 zoomOrigin{};
	UVUtil::ZoomDir zoomFactor;
    ImVec2 panDelta{};
};

class ImageViewer
{
public:
	ImageViewer(int w,int h);
	~ImageViewer();
    void BeginFrame();
    void Update(GLFWwindow* window);
    void Draw(ImTextureID texture);
    const ImageViewEvent& GetEvent() const;
	void ApplyZoom(const ImVec2& origin, UVUtil::ZoomDir factor);
	void ApplyPan(const ImVec2& delta);
	void ResetView();
	void SetImageSize(int w, int h);

private:
	ImVec2 uv0;
	ImVec2 uv1;
	float winAspect;
    bool   isPanning = false;
	bool isHovered = false;
	bool mousePressedOnImage = false;
    ImVec2 lastMouse{};
	int width, height;
    ImageViewEvent event{};
};
