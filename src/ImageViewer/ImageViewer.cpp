#include "ImageViewer.h"


ImageViewer::ImageViewer(int w,int h)
	:width(w),height(h)
{
	uv0 = ImVec2(0.0f, 0.0f);
	uv1 = ImVec2(1.0f, 1.0f);
}
ImageViewer::~ImageViewer()
{
}
void ImageViewer::BeginFrame()
{
	event = ImageViewEvent{};
}
void ImageViewer::Update(GLFWwindow* window) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 winSize = ImGui::GetContentRegionAvail();
    float imgAspect = (float)width / height;
    winAspect = winSize.x / winSize.y;
    UVUtil::CalcUVForAspect(imgAspect, winAspect, uv0, uv1);

    // ドラッグ中、またはホバー中のみ処理を続行
    if (!isHovered && !isPanning)
        return;

    ImVec2 mousePos = ImGui::GetMousePos();
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 uvMousePos = UVUtil::ScreenPosToUV(mousePos, windowPos, winSize, uv0, uv1);

    // --- 2. ズーム処理 ---
    if (isHovered && io.MouseWheel != 0.0f) {
        UVUtil::ZoomDir factor = (io.MouseWheel > 0.0f) ? UVUtil::ZoomDir::ZoomIn : UVUtil::ZoomDir::ZoomOut;
        UVUtil::ApplyZoom(uv0, uv1, uvMousePos, factor);
        event.zoomed = true;
        event.zoomOrigin = uvMousePos;
        event.zoomFactor = factor;
    }

    // --- 3. カーソル表示切り替え ---
    if (isHovered && !isPanning) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    // --- 4. ドラッグ（パニング）処理 ---
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && mousePressedOnImage) {
        if (!isPanning) {
            isPanning = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        // 「固定して移動距離だけ取る」場合は io.MouseDelta が最適
        ImVec2 delta{
            io.MouseDelta.x / winSize.x * (uv1.x - uv0.x),
            io.MouseDelta.y / winSize.y * (uv1.y - uv0.y)
        };

        uv0.x -= delta.x; uv1.x -= delta.x;
        uv0.y -= delta.y; uv1.y -= delta.y;

        event.panned = true;
        event.panDelta = delta;
    }
    else if (isPanning) {
        // 左クリックが離された瞬間
        isPanning = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    // --- 5. リセット処理 ---
    if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        ResetView();
        event.reset = true;
    }
}
void ImageViewer::Draw(ImTextureID texture)
{
	ImVec2 winSize = ImGui::GetContentRegionAvail();
	ImGui::Image(texture, winSize, uv0, uv1);
    // 左クリックされた瞬間
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        mousePressedOnImage = ImGui::IsItemHovered();
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        mousePressedOnImage = false;
    }

    isHovered = ImGui::IsItemHovered() || (isPanning && mousePressedOnImage);

}
const ImageViewEvent& ImageViewer::GetEvent() const
{
	return event;
}
void ImageViewer::ApplyZoom(const ImVec2& origin, UVUtil::ZoomDir factor)
{
	UVUtil::ApplyZoom(uv0, uv1, origin, factor);
}
void ImageViewer::ApplyPan(const ImVec2& delta)
{
	uv0.x -= delta.x; uv1.x -= delta.x;
	uv0.y -= delta.y; uv1.y -= delta.y;
}
void ImageViewer::ResetView()
{
	uv0 = ImVec2(0.0f, 0.0f);
	uv1 = ImVec2(1.0f, 1.0f);
	UVUtil::CalcUVForAspect((float)width / height, winAspect, uv0, uv1);
}
void ImageViewer::SetImageSize(int w, int h)
{
	width = w;
	height = h;
}