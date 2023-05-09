#include "Imgui/imgui.h"
#include "Imgui/imgui_impl_dx9.h"
#include "Imgui/imgui_impl_win32.h"

typedef struct
{
	DWORD R;
	DWORD G;
	DWORD B;
	DWORD A;
}RGBA;

std::string string_To_UTF8(const std::string& str) {
	int nwLen = ::MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, NULL, 0);
	wchar_t* pwBuf = new wchar_t[nwLen + 1];
	ZeroMemory(pwBuf, nwLen * 2 + 2);
	::MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.length(), pwBuf, nwLen);
	int nLen = ::WideCharToMultiByte(CP_UTF8, 0, pwBuf, -1, NULL, NULL, NULL, NULL);
	char* pBuf = new char[nLen + 1];
	ZeroMemory(pBuf, nLen + 1);
	::WideCharToMultiByte(CP_UTF8, 0, pwBuf, nwLen, pBuf, nLen, NULL, NULL);
	std::string retStr(pBuf);
	delete[]pwBuf;
	delete[]pBuf;
	pwBuf = NULL;
	pBuf = NULL;
	return retStr;
}

void DrawStrokeText(int x, int y, RGBA* color, const char* str) {
	ImFont a;
	std::string utf_8_1 = std::string(str);
	std::string utf_8_2 = string_To_UTF8(utf_8_1);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y - 1), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y + 1), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x - 1, y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x + 1, y), ImGui::ColorConvertFloat4ToU32(ImVec4(1 / 255.0, 1 / 255.0, 1 / 255.0, 255 / 255.0)), utf_8_2.c_str());
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), utf_8_2.c_str());
}

void DrawNewText(int x, int y, RGBA* color, const char* str) {
	ImFont a;
	std::string utf_8_1 = std::string(str);
	std::string utf_8_2 = string_To_UTF8(utf_8_1);
	ImGui::GetOverlayDrawList()->AddText(ImVec2(x, y), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), utf_8_2.c_str());
}

void DrawRect(int x, int y, int w, int h, RGBA* color, int thickness) {
	ImGui::GetOverlayDrawList()->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), 0, 0, thickness);
}

void DrawFilledRect(int x, int y, int w, int h, RGBA* color) {
	ImGui::GetOverlayDrawList()->AddRectFilled(ImVec2(x, y), ImVec2(x + w, y + h), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), 0, 0);
}

void DrawCircleFilled(int x, int y, int radius, RGBA* color) {
	ImGui::GetOverlayDrawList()->AddCircleFilled(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)));
}

auto DrawCircle(float x, float y, float radius, ImVec4 color, int segments)-> VOID
{
	auto vList = ImGui::GetOverlayDrawList();
	vList->AddCircle(ImVec2(x, y), radius, ImGui::ColorConvertFloat4ToU32(color), segments);
}

auto RectFilled(float x0, float y0, float x1, float y1, ImColor color, float rounding, int rounding_corners_flags)-> VOID
{
	auto vList = ImGui::GetOverlayDrawList();
	vList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), color, rounding, rounding_corners_flags);
}

auto HealthBar(float x, float y, float w, float h, int phealth)-> VOID
{
	auto vList = ImGui::GetOverlayDrawList();

	int healthValue = max(0, min(phealth, 100));

	int barColor = ImColor
	(min(510 * (100 - healthValue) / 100, 255), min(510 * healthValue / 100, 255), 25, 255);
	vList->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), ImColor(255.0f / 255, 255.0f, 255.0f));
	RectFilled(x, y, x + w, y + (((float)h / 100.0f) * (float)phealth), barColor, 0.0f, 0);
}

void DrawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, RGBA* color, float thickne) {
	ImGui::GetOverlayDrawList()->AddTriangle(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), thickne);
}

void DrawTriangleFilled(int x1, int y1, int x2, int y2, int x3, int y3, RGBA* color) {
	ImGui::GetOverlayDrawList()->AddTriangleFilled(ImVec2(x1, y1), ImVec2(x2, y2), ImVec2(x3, y3), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)));
}

auto DrawLine2(const ImVec2& aPoint1, const ImVec2 aPoint2, ImU32 aColor, const FLOAT aLineWidth) -> VOID
{
	auto vList = ImGui::GetOverlayDrawList();
	vList->AddLine(aPoint1, aPoint2, aColor, aLineWidth);
}

auto DrawBox(float x, float y, float w, float h, ImColor color, float thickness)-> VOID
{
	DrawLine2(ImVec2(x, y), ImVec2(x + w, y), color, thickness); // top 
	DrawLine2(ImVec2(x, y - 1.3f), ImVec2(x, y + h + 1.4f), color, thickness); // left
	DrawLine2(ImVec2(x + w, y - 1.3f), ImVec2(x + w, y + h + 1.4f), color, thickness);  // right
	DrawLine2(ImVec2(x, y + h), ImVec2(x + w, y + h), color, thickness);   // bottom 
}


void DrawLine(int x1, int y1, int x2, int y2, RGBA* color, int thickness) {
	ImGui::GetOverlayDrawList()->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), ImGui::ColorConvertFloat4ToU32(ImVec4(color->R / 255.0, color->G / 255.0, color->B / 255.0, color->A / 255.0)), thickness);
}

auto DrawCorneredBox(float X, float Y, float W, float H, const ImU32& color, float thickness) -> VOID
{
	auto vList = ImGui::GetOverlayDrawList();

	float lineW = (W / 3);
	float lineH = (H / 3);
	//black outlines
	auto col = ImGui::GetColorU32(color);

	//corners
	vList->AddLine(ImVec2(X, Y - thickness / 2), ImVec2(X, Y + lineH), col, thickness);//top left
	vList->AddLine(ImVec2(X - thickness / 2, Y), ImVec2(X + lineW, Y), col, thickness);

	vList->AddLine(ImVec2(X + W - lineW, Y), ImVec2(X + W + thickness / 2, Y), col, thickness);//top right horizontal
	vList->AddLine(ImVec2(X + W, Y - thickness / 2), ImVec2(X + W, Y + lineH), col, thickness);

	vList->AddLine(ImVec2(X, Y + H - lineH), ImVec2(X, Y + H + (thickness / 2)), col, thickness);//bot left
	vList->AddLine(ImVec2(X - thickness / 2, Y + H), ImVec2(X + lineW, Y + H), col, thickness);

	vList->AddLine(ImVec2(X + W - lineW, Y + H), ImVec2(X + W + thickness / 2, Y + H), col, thickness);//bot right
	vList->AddLine(ImVec2(X + W, Y + H - lineH), ImVec2(X + W, Y + H + (thickness / 2)), col, thickness);

}

void DrawCornerBox(int x, int y, int w, int h, int borderPx, RGBA* color) {
	DrawFilledRect(x + borderPx, y, w / 3, borderPx, color);
	DrawFilledRect(x + w - w / 3 + borderPx, y, w / 3, borderPx, color);
	DrawFilledRect(x, y, borderPx, h / 3, color);
	DrawFilledRect(x, y + h - h / 3 + borderPx * 2, borderPx, h / 3, color);
	DrawFilledRect(x + borderPx, y + h + borderPx, w / 3, borderPx, color);
	DrawFilledRect(x + w - w / 3 + borderPx, y + h + borderPx, w / 3, borderPx, color);
	DrawFilledRect(x + w + borderPx, y, borderPx, h / 3, color);
	DrawFilledRect(x + w + borderPx, y + h - h / 3 + borderPx * 2, borderPx, h / 3, color);
}