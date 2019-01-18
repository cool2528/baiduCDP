#include "shadow_window.h"
#include <iostream>
#include <map>

//////////////////////////////////////////////////////////////////////
// LayeredWin Support
// 使用自绘阴影窗口
//////////////////////////////////////////////////////////////////////

class LayeredWin : public ShadowWnd {
protected:
	COLORREF _color;
	int _shadow_size;
	int _sharpness;
	int _darkness;
	int _offset;
	HWND _hwnd;
	HWND _dest_hwnd;
	WNDPROC _dest_wndproc;
public:
	LayeredWin();
	virtual ~LayeredWin();

	virtual void Setup(int size, int sharpness, int darkness, int offset);
	virtual void Update();

	virtual int Create(HWND dest);

	// 处理目标窗口消息
	LRESULT HandDestWndMsg(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

public:
	// 阴影(自身)窗口消息处理
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	// 目标窗口消息钩子, 转发到 LayeredWin->MsgHookProc()
	static LRESULT CALLBACK DestWndMsgRoute(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};


// 生命周期管理,所有创建的阴影窗口都放在此处
// 如果消息异常，程序结束时检查并释放内存
struct WndManager {
	std::map<HWND, LayeredWin*> wnds;
	~WndManager() {
		for (auto it = wnds.begin(); it != wnds.end(); ++it) {
			if (it->second) {
				delete it->second;
			}
		}
		wnds.clear();
	}
	LayeredWin* get(HWND hwnd) {
		auto it = wnds.find(hwnd);
		return (it == wnds.end()) ? 0 : it->second;
	}
}__manager;


LayeredWin::LayeredWin()
{
	_dest_hwnd = 0;
	_color = 0;
	_shadow_size = 15;
	_sharpness = 15;
	_darkness = 30;
	_offset = 0;
	_hwnd = 0;
	_dest_wndproc = 0;
}

LayeredWin::~LayeredWin() 
{
	if (_hwnd) {
		DestroyWindow(_hwnd);
		_hwnd = 0;
	}
	if (_dest_hwnd && _dest_wndproc) {	// 在目标消息NCDESTROY时置0
		SetWindowLongPtr(_dest_hwnd, GWLP_WNDPROC, (LONG_PTR) _dest_wndproc);
		_dest_hwnd = 0;
		_dest_wndproc = 0;
	}
	// printf("~LayeredWin()\n");
}

void LayeredWin::Setup(int size, int sharpness, int darkness, int offset)
{
	_shadow_size = size;
	_sharpness = sharpness;
	_darkness = darkness;
	_offset = offset;
	Update();
}

int LayeredWin::Create(HWND dest) 
{
	if (!IsWindow(dest)) {
		return 0;
	}

	static ATOM atom = 0;
	HINSTANCE hinst = GetModuleHandle(0);

	if (!atom) {
		WNDCLASSEX wc = {
			sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, // | CS_DROPSHADOW,
			WndProc, 0, 0, hinst, 
			0, 0, 0, 0, L"Shadow Window", 0
		};
		atom = RegisterClassEx(&wc);
	}
	if (!atom) {
		return 0;
	}

	_hwnd = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
		L"Shadow Window", L"Shadow Window",
		WS_POPUP |  WS_CLIPSIBLINGS | WS_DISABLED | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, 100, 100,
		NULL, 0, hinst, this);
	if (!_hwnd) {
		return 0;
	}
	_dest_hwnd = dest;
	// _dest_wndproc = (WNDPROC)GetWindowLongPtr(dest, GWLP_WNDPROC);
	_dest_wndproc = (WNDPROC)SetWindowLongPtr(dest, GWLP_WNDPROC, (LONG_PTR)DestWndMsgRoute);
	SetWindowPos(dest, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_DRAWFRAME);

	Update();
	return 1;
}

void LayeredWin::Update()
{
	RECT rc;
	GetWindowRect(_dest_hwnd, &rc);
	// printf("rc=%d,%d,%d,%d\n", rc.left, rc.top, rc.right, rc.bottom);
	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;
	HRGN rgn = 0;
	GetWindowRgn(_dest_hwnd, rgn);
	// 创建阴影位图
	HBITMAP bitmap = MakeShadowImage(width, height, _shadow_size, _sharpness, _darkness, _color, _offset, rgn);

	// 绘制阴影
	HDC hdc = GetWindowDC(_hwnd);
	HDC hdc_mem = CreateCompatibleDC(hdc);
	HBITMAP orig_bitmap = (HBITMAP)SelectObject(hdc_mem, bitmap);

	BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

	POINT pt_src = { 0, 0 };
	POINT pt_dest = { rc.left + _offset - _shadow_size, rc.top + _offset - _shadow_size };
	SIZE sz = { width + _shadow_size * 2, height + _shadow_size * 2 };

	UpdateLayeredWindow(_hwnd, hdc, &pt_dest, &sz, hdc_mem, &pt_src, 0, &bf, ULW_ALPHA);

	SelectObject(hdc_mem, orig_bitmap);
	DeleteObject(bitmap);
	DeleteDC(hdc_mem);
}


LRESULT CALLBACK LayeredWin::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK LayeredWin::DestWndMsgRoute(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	auto window = __manager.get(hwnd);
	if (!window) {
		printf("hook msg error: %d(0x%08x)\n", msg, msg);
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	auto result = window->HandDestWndMsg(hwnd, msg, wparam, lparam);

	if (WM_NCDESTROY == msg) {
		delete window;
		__manager.wnds.erase(hwnd);
	}
	return result;
}

// 目标窗口消息钩子处理
LRESULT LayeredWin::HandDestWndMsg(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
		// 此时应该销毁本对象，执行到return后，在上层 DestWndMsgRoute 进行销毁。
		case WM_NCDESTROY:
			_dest_hwnd = 0;
			break;
		case WM_NCCALCSIZE:
			return 0;
		case WM_DESTROY:
			DestroyWindow(_hwnd);
			_hwnd = 0;
			break;
		case WM_MOVE:
		{
			RECT rc;
			GetWindowRect(hwnd, &rc);
			SetWindowPos(_hwnd, 0,
				rc.left + _offset - _shadow_size, rc.top + _offset - _shadow_size,
				0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
			break;
		}
		case WM_SETFOCUS:
			SetWindowPos(_hwnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
			break;
		case WM_SIZE:
			if (SIZE_MINIMIZED == wparam) {
				//ShowWindow(_hwnd, wparam);
				ShowWindow(_hwnd, SW_HIDE);
			}
			else if (SIZE_MAXIMIZED == wparam) {
				ShowWindow(_hwnd, SW_HIDE);
			}
			else {
				Update();
				ShowWindow(_hwnd, SW_SHOW);
			}
			break;

	}
	return _dest_wndproc(hwnd, msg, wparam, lparam);
}




ShadowWnd* BindShadowWindow(HWND dest)
{
	LayeredWin* window = __manager.get(dest);
	if (window) {
		return window;
	}
	if (IsWindow(dest)) {
		window = new LayeredWin();
		__manager.wnds[dest] = window;
		if (!window->Create(dest)) {
			delete window;
			__manager.wnds.erase(dest);
			return 0;
		}
	}	
	return window;
}

void UnbindShadowWindow(HWND dest)
{
	LayeredWin* window = __manager.get(dest);
	if (window) {
		delete window;
		__manager.wnds.erase(dest);
	}
}

ShadowWnd* GetShadowWindow(HWND hwnd_dest)
{
	return __manager.get(hwnd_dest);
}



//////////////////////////////////////////////////////////////////////
// WDM Shadow Support
// 使用WDM窗口管理器
//////////////////////////////////////////////////////////////////////

namespace WdmShadow {

// 兼容XP,不加载dwmapi库
LRESULT(WINAPI* DwmExtendFrameIntoClientArea)(HWND hWnd, const int* pMarInset) = 0;
BOOL(WINAPI* IsCompositionActive)() = 0;

int InitDwmFunc()
{
	HMODULE dll = 0;
	if (dll = LoadLibrary(L"dwmapi.dll")) {
		*(FARPROC*)&DwmExtendFrameIntoClientArea = GetProcAddress(dll, "DwmExtendFrameIntoClientArea");
	}
	if (dll = LoadLibrary(L"UxTheme.dll")) {
		*(FARPROC*)&IsCompositionActive = GetProcAddress(dll, "IsCompositionActive");
	}
	return 0;	// must 0
}

int IsSupported() {
	static int once = InitDwmFunc();
	return DwmExtendFrameIntoClientArea ? IsCompositionActive() : once;
}


// 已挂接的窗口信息,用于恢复
struct BindInfo {
	WNDPROC wndproc;
	DWORD style;
};

std::map<HWND, BindInfo> binds;

LRESULT CALLBACK MsgHooker(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	auto orig = binds.find(hwnd);
	if (orig == binds.end()) {
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	// hook it 
	if (msg == WM_NCCALCSIZE) {
		return 0;
	}

	auto result = orig->second.wndproc(hwnd, msg, wparam, lparam);
	if (WM_NCDESTROY == msg) {
		binds.erase(orig);
	}
	return result;
}

// 修改Style, HookMessage
void HookMsg(HWND dest, int state) {
	if (state) {
		BindInfo info;
		info.style = (DWORD) GetWindowLongPtr(dest, GWL_STYLE);
		info.wndproc = (WNDPROC)GetWindowLongPtr(dest, GWLP_WNDPROC);
		binds[dest] = info;

		SetWindowLongPtr(dest, GWL_STYLE, (LONG_PTR)(info.style | WS_THICKFRAME));
		SetWindowLongPtr(dest, GWLP_WNDPROC, (LONG_PTR)MsgHooker);
	}
	else {
		auto it = binds.find(dest);
		if (it != binds.end()) {
			SetWindowLongPtr(dest, GWL_STYLE, (LONG_PTR)it->second.style);
			SetWindowLongPtr(dest, GWLP_WNDPROC, (LONG_PTR)it->second.wndproc);
			binds.erase(it);
		}
	}
}

};


//////////////////////////////////////////////////////////////////////
// WdmShadow 对外接口
//////////////////////////////////////////////////////////////////////

int IsSupportDwm() 
{
	return WdmShadow::IsSupported();
}

int EnableDwmShadow(HWND hwnd, int enable, int force_hook)
{
	if (!IsSupportDwm()) {
		return 0;
	}

	if (enable && !IsWindow(hwnd)) {
		return 0;
	}

	if (force_hook) {
		WdmShadow::HookMsg(hwnd, enable);
	}

	int margins[2][4] = { {0,0,0,0}, { 1, 1, 1, 1 } };
	WdmShadow::DwmExtendFrameIntoClientArea(hwnd, enable ? margins[1] : margins[0]);
	SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
	return 1;
}

int EnableStyleShadow(HWND hwnd, int enable)
{
	DWORD style = (DWORD) GetClassLongPtr(hwnd, GCL_STYLE);
	style = enable ? (style | CS_DROPSHADOW) : (style & ~CS_DROPSHADOW);
	SetClassLongPtr(hwnd, GCL_STYLE, style);
	return 1;
}



//////////////////////////////////////////////////////////////////////
// Common APIs for Shadow Window
//////////////////////////////////////////////////////////////////////

// 预乘
inline DWORD PreMultiply(COLORREF cl, unsigned char alpha)
{
	// It's strange that the byte order of RGB in 32b BMP is reverse to in COLORREF
	return (GetRValue(cl) * (DWORD)alpha / 255) << 16 |
		(GetGValue(cl) * (DWORD)alpha / 255) << 8 |
		(GetBValue(cl) * (DWORD)alpha / 255);
}

HBITMAP MakeShadowImage(
	int width,
	int height,
	int size,
	int sharpness,
	int darkness,
	COLORREF color,
	int offset,
	HRGN rgn)
{
	int shadow_size = size;
	if (shadow_size < -20 || shadow_size > 20) {
		shadow_size = 10;
	}
	if (sharpness < 0 || sharpness > 20) {
		sharpness = 16;
	}
	if (darkness < 0 || darkness > 255) {
		darkness = 100;
	}

	// 如果有，则为双边阴影，右下。否则为一圈
	if (offset > 20 || offset < -20) {
		offset = 0;
	}

	int offset_x = offset;
	int offset_y = offset;

	int bmp_width = width + shadow_size * 2;
	int bmp_height = height + shadow_size * 2;

	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = bmp_width;
	bmi.bmiHeader.biHeight = bmp_height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32; // four 8-bit components
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = bmp_width * bmp_height * 4;


	unsigned int* bmp_bits;
	HBITMAP hbitmap = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void **)&bmp_bits, NULL, 0);
	ZeroMemory(bmp_bits, bmi.bmiHeader.biSizeImage);


	// The shadow algorithm:
	// Get the region of parent window,
	// Apply morphologic erosion to shrink it into the size (ShadowSize - Sharpness)
	// Apply modified (with blur effect) morphologic dilation to make the blurred border
	HRGN use_rgn = rgn ? rgn : CreateRectRgn(0, 0, width, height);

	// Extra 2 lines (set to be empty) in anchors are used in dilation
	int num_anchors = max(height, bmp_height);	// # of anchor points pares
	int(*anchors)[2] = new int[num_anchors + 2][2];
	int(*anchors_orig)[2] = new int[height][2];	// anchor points, will not modify during erosion
	anchors[0][0] = width;
	anchors[0][1] = 0;
	anchors[num_anchors + 1][0] = width;
	anchors[num_anchors + 1][1] = 0;
	if (shadow_size > 0)
	{
		// Put the parent window anchors at the center
		for (int i = 0; i < shadow_size; i++)
		{
			anchors[i + 1][0] = width;
			anchors[i + 1][1] = 0;
			anchors[bmp_height - i][0] = width;
			anchors[bmp_height - i][1] = 0;
		}
		anchors += shadow_size;
	}
	for (int i = 0; i < height; i++)
	{
		// find start point
		int j;
		for (j = 0; j < width; j++)
		{
			if (PtInRegion(use_rgn, j, i))
			{
				anchors[i + 1][0] = j + shadow_size;
				anchors_orig[i][0] = j;
				break;
			}
		}
		if (j >= width)	// Start point not found
		{
			anchors[i + 1][0] = width;
			anchors_orig[i][1] = 0;
			anchors[i + 1][0] = width;
			anchors_orig[i][1] = 0;
		}
		else
		{
			// find end point
			for (j = width - 1; j >= anchors[i + 1][0]; j--)
			{
				if (PtInRegion(use_rgn, j, i))
				{
					anchors[i + 1][1] = j + 1 + shadow_size;
					anchors_orig[i][1] = j + 1;
					break;
				}
			}
		}
		// if(0 != anchors_orig[i][1])
		// _RPTF2(_CRT_WARN, "%d %d\n", anchors_orig[i][0], anchors_orig[i][1]);
	}
	if (shadow_size > 0)
		anchors -= shadow_size;	// Restore pos of anchors for erosion
	int(*tmp_anchors)[2] = new int[num_anchors + 2][2];	// Store the result of erosion
	// First and last line should be empty
	tmp_anchors[0][0] = width;
	tmp_anchors[0][1] = 0;
	tmp_anchors[num_anchors + 1][0] = width;
	tmp_anchors[num_anchors + 1][1] = 0;
	int num_ero_times = 0;
	// morphologic erosion
	for (int i = 0; i < sharpness - shadow_size; i++)
	{
		num_ero_times++;
		//tmp_anchors[1][0] = width;
		//tmp_anchors[1][1] = 0;
		//tmp_anchors[height + 1][0] = width;
		//tmp_anchors[height + 1][1] = 0;
		for (int j = 1; j < num_anchors + 1; j++)
		{
			tmp_anchors[j][0] = max(anchors[j - 1][0], max(anchors[j][0], anchors[j + 1][0])) + 1;
			tmp_anchors[j][1] = min(anchors[j - 1][1], min(anchors[j][1], anchors[j + 1][1])) - 1;
		}
		// Exchange anchors and tmp_anchors;
		int(*anchors_xange)[2] = tmp_anchors;
		tmp_anchors = anchors;
		anchors = anchors_xange;
	}
	// morphologic dilation
	// now coordinates in anchors are same as in shadow window
	anchors += (shadow_size < 0 ? -shadow_size : 0) + 1;

	// Generate the kernel
	int kernel_size = shadow_size > sharpness ? shadow_size : sharpness;
	int center_size = shadow_size > sharpness ? (shadow_size - sharpness) : 0;
	UINT32 *kernel_ptr = new UINT32[(2 * kernel_size + 1) * (2 * kernel_size + 1)];
	UINT32 *kernel_iter = kernel_ptr;
	for (int i = 0; i <= 2 * kernel_size; i++)
	{
		for (int j = 0; j <= 2 * kernel_size; j++)
		{
			double dLength = sqrt((i - kernel_size) * (i - kernel_size) + (j - kernel_size) * (double)(j - kernel_size));
			if (dLength < center_size)
				*kernel_iter = darkness << 24 | PreMultiply(color, darkness);
			else if (dLength <= kernel_size)
			{
				UINT32 nFactor = ((UINT32)((1 - (dLength - center_size) / (sharpness + 1)) * darkness));
				*kernel_iter = nFactor << 24 | PreMultiply(color, nFactor);
			}
			else
				*kernel_iter = 0;
			//TRACE("%d ", *kernel_iter >> 24);
			kernel_iter++;
		}
		//TRACE("\n");
	}

	// Generate blurred border
	for (int i = kernel_size; i < bmp_height - kernel_size; i++)
	{
		int j;
		if (anchors[i][0] < anchors[i][1])
		{
			// Start of line
			for (j = anchors[i][0];
				j < min(max(anchors[i - 1][0], anchors[i + 1][0]) + 1, anchors[i][1]);
				j++)
			{
				for (int k = 0; k <= 2 * kernel_size; k++)
				{
					UINT32 *pixel = bmp_bits +
						(bmp_height - i - 1 + kernel_size - k) * bmp_width + j - kernel_size;
					UINT32 *kernel_pixel = kernel_ptr + k * (2 * kernel_size + 1);
					for (int l = 0; l <= 2 * kernel_size; l++)
					{
						if (*pixel < *kernel_pixel)
							*pixel = *kernel_pixel;
						pixel++;
						kernel_pixel++;
					}
				}
			}	// for() start of line

			// End of line
			for (j = max(j, min(anchors[i - 1][1], anchors[i + 1][1]) - 1);
				j < anchors[i][1];
				j++)
			{
				for (int k = 0; k <= 2 * kernel_size; k++)
				{
					UINT32 *pixel = bmp_bits +
						(bmp_height - i - 1 + kernel_size - k) * bmp_width + j - kernel_size;
					UINT32 *kernel_pixel = kernel_ptr + k * (2 * kernel_size + 1);
					for (int l = 0; l <= 2 * kernel_size; l++)
					{
						if (*pixel < *kernel_pixel)
							*pixel = *kernel_pixel;
						pixel++;
						kernel_pixel++;
					}
				}
			}	// for() end of line
		}
	}	// for() Generate blurred border
	// Erase unwanted parts and complement missing
	UINT32 cl_center = darkness << 24 | PreMultiply(color, darkness);
	for (int i = min(kernel_size, max(shadow_size - offset_y, 0));
		i < max(bmp_height - kernel_size, min(height + shadow_size - offset_y, height + 2 * shadow_size));
		i++)
	{
		UINT32 *line_ptr = bmp_bits + (bmp_height - i - 1) * bmp_width;
		if (i - shadow_size + offset_y < 0 || i - shadow_size + offset_y >= height)	// Line is not covered by parent window
		{
			for (int j = anchors[i][0]; j < anchors[i][1]; j++)
			{
				*(line_ptr + j) = cl_center;
			}
		}
		else
		{
			for (int j = anchors[i][0];
				j < min(anchors_orig[i - shadow_size + offset_y][0] + shadow_size - offset_x, anchors[i][1]);
				j++)
				*(line_ptr + j) = cl_center;
			for (int j = max(anchors_orig[i - shadow_size + offset_y][0] + shadow_size - offset_x, 0);
				j < min(anchors_orig[i - shadow_size + offset_y][1] + shadow_size - offset_x, bmp_width);
				j++)
				*(line_ptr + j) = 0;
			for (int j = max(anchors_orig[i - shadow_size + offset_y][1] + shadow_size - offset_x, anchors[i][0]);
				j < anchors[i][1];
				j++)
				*(line_ptr + j) = cl_center;
		}
	}
	// Delete used resources
	delete[](anchors - (shadow_size < 0 ? -shadow_size : 0) - 1);
	delete[] tmp_anchors;
	delete[] anchors_orig;
	delete[] kernel_ptr;

	if (!rgn) {
		DeleteObject(use_rgn);
	}
	return hbitmap;
}



