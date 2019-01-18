
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// 阴影窗口接口
struct ShadowWnd {
	virtual void Setup(int size, int sharpness, int darkness, int offset) = 0;
	virtual void Update() = 0;
};

// 创建一个阴影窗口，对目标窗口进行绑定，返回一个接口(无须释放, 生命期与目标窗口同步)
// 注：强制拦截WM_NCCALCSIZE消息，使之不绘制边框和标题栏
ShadowWnd* BindShadowWindow(HWND hwnd_dest);

// 删除绑定的阴影窗口
void UnbindShadowWindow(HWND hwnd_dest);

// 获取绑定的阴影窗口，在运行期，应使用GetShadowWindow()获取相关的阴影对象，且判断是否为空
ShadowWnd* GetShadowWindow(HWND hwnd_dest);

// 启用DWM窗口管理器自带的阴影特效
// 前提： 样式: WS_THICKFRAME, IsCompositionActive(), 以及拦截WM_NCCALESIZE消息
// 如果指定force_hook, 则强制修改样式,以及拦截WM_NCCALESIZE消息, 否则请确保目标窗口已满足前提
int EnableDwmShadow(HWND hwnd, int enable, int force_hook = 1);


// 适用于 XP/Win7经典主题，产生一个右下方阴影特效。 适用于overlapped window
// CS_DROPSHADOW
int EnableStyleShadow(HWND hwnd, int enable);

// 如果支持DWM,则使用 EnableDwmShadow()来开启阴影。
int IsSupportDwm();



// 生成一个带alpha通道的阴影图像
// width,height 矩形大小，以此矩形往外扩展阴影
// size: -20 ~ +20 阴影宽度
// sharpness: 渐变, 0-20, 0为不渐变
// darkness: 色调，最大0-255, 越大越黑
// color: 颜色， 0为黑色
// offset: 偏移， -20 ~ +20 若指定，为右下两个方向， 否则为四周
// rgn: 形状
HBITMAP MakeShadowImage(
	int width,
	int height,
	int size = 10,
	int sharpness = 20,
	int darkness = 30,
	COLORREF color = 0,
	int offset = 0,
	HRGN rgn = 0);

