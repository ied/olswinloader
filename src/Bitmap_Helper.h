//
// BITMAP_HELPER.H
// Code contained in this file is hereby released to the public domain. 
// Ian Davis, 4/24/2007
// http://www.codeproject.com/KB/vista/vistathemebuttons.aspx
//

//
// Misc routines for manipulating Windows bitmaps...
//
#ifndef _BUTTONVE_HELPER_H_
#define _BUTTONVE_HELPER_H_

#define ROP_PSDPxax 0x00B8074AL 
#define ROP_DSna 0x00220326L    // Dest = (not SRC) and Dest

#include "ols_winloader.h"

// Need library containing AlphaBlend function...
#pragma comment (lib,"msimg32.lib")

// Support 64-bit compiles on VS7/8...
#ifndef TIMER_WPARAM
#if _MSC_VER >= 1300
  #define TIMER_WPARAM UINT_PTR
#else
  #define TIMER_WPARAM UINT
#endif
#endif


class CBitmapEx : public CBitmap {
public:
  CBitmapEx() : CBitmap() {}
  BOOL LoadIcon(int resource_id, COLORREF bgcolor, int cx=32, int cy=32);
};


#define mainwnd(param) (theApp.m_pMainWnd != NULL) ? theApp.m_pMainWnd->param : 0
void AFXAPI AfxDeleteObject(HGDIOBJ* pObject);


//
// Helper class for dealing with memory drawing contexts...
//
#ifndef _CDCBITMAP_
#define _CDCBITMAP_
class CDCBitmap : public CDC
{
  HDC m_src_hDC;
  CRect m_rc;
  HBITMAP mem_bitmap;
  HBITMAP old_bitmap;
public:
  CDCBitmap(HDC hDC, HBITMAP hBitmap) {
    Attach(::CreateCompatibleDC(hDC));
    old_bitmap = (HBITMAP)SelectObject(hBitmap);
    mem_bitmap=NULL;
  }

  CDCBitmap(HDC hDC, CBitmap *bmp) {
    ASSERT(bmp);
    Attach(::CreateCompatibleDC(hDC));
    old_bitmap = (HBITMAP)SelectObject(*bmp); 
    mem_bitmap=NULL;
  }

  CDCBitmap(HDC hDC, CRect rc) {
    m_src_hDC = hDC;
    m_rc = rc;
    Attach(::CreateCompatibleDC(hDC));
    mem_bitmap = ::CreateCompatibleBitmap(hDC,m_rc.Width(),m_rc.Height());
    old_bitmap = (HBITMAP)SelectObject(mem_bitmap);
    SetWindowOrg(m_rc.left,m_rc.top);
  }

  void SetBitmap(HBITMAP hBitmap) {
    if (mem_bitmap) 
      ::BitBlt(m_src_hDC, m_rc.left, m_rc.top, m_rc.Width(), m_rc.Height(), 
        GetSafeHdc(), m_rc.left, m_rc.top, SRCCOPY);
    SelectObject(hBitmap);
    if (mem_bitmap) ::DeleteObject(mem_bitmap);
    mem_bitmap = NULL;
  }

  ~CDCBitmap() {SetBitmap(old_bitmap);}
};
#endif // _CDCBITMAP_


void DrawMonoBitmap (CDC &dc, CDC &monoDC, CPoint icoord, CSize imagesize, COLORREF color);
void DrawDisabledImage(CDC &dc, CDC &srcDC, CPoint icoord, CSize imagesize, COLORREF transparent_color);
void DrawTransparentImage(CDC &dc, CDC &srcDC, CPoint icoord, CSize imagesize, COLORREF transparent_color);
int GetDIBPixels(HDC dc, HBITMAP bmp, BITMAPINFO &bmi, DWORD **pixels);
void DrawBlurredShadowImage(CDC &dc, CDC &srcDC, CPoint icoord, CSize imagesize, COLORREF transparent_color);
void AlphaBlt(HDC destdc, CRect rdest, HDC srcdc, CRect rsrc, int alpha);
void AlphaBltFrame(HDC destdc, HDC srcdc, CRect rc, CSize border_size, int alpha);

void DrawBitmap (CDC *pDC, HBITMAP bmp, CRect dest, CPoint src);
void DrawTransparentBitmap(HDC hDC, HDC hdcSrc, short xDest, short yDest, int xSrc, int ySrc, int xSize, int ySize, COLORREF cTransparentColor);
bool GetBitmapPixels(HDC hdc, HBITMAP image, int width, int height, BOOL invert, byte **dib_ptr=NULL, byte **pixel_ptr=NULL, int *pixels_bytesperline=NULL);
HICON LoadIconResource (int resource_id, int cx=32, int cy=32);
HBITMAP LoadBitmapResource (int resource_id, int cx=32, int cy=32);
void BitmapColorMap(HBITMAP tBitmap, COLORREF target, COLORREF newcolor);

  // use DWORD instead of RGBQUAD so we can compare two RGBQUADs easily
struct REMAP_COLORMAP { 
  DWORD rgbqFrom; 
  int iSysColorTo; 
};
HBITMAP RemapBitmapColor(HINSTANCE hInst, HRSRC hRsrc, COLORMAP *sysColorMap, int sysColorMapSize);

#endif // define _BUTTONVE_HELPER_H_
  
