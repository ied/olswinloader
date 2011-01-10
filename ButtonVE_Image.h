//
// BUTTONVE_IMAGE.H
// Code contained in this file is hereby released to the public domain. 
// Ian Davis, 4/24/2007
// http://www.codeproject.com/KB/vista/vistathemebuttons.aspx
//

//
// Owner drawn WinXP/Vista themes aware image button implementation...
//
// Rev 1.0 - Apr 24th - Original Release
// Rev 1.1 - Apr 27th - No change to this file.
// Rev 1.2 -          - Added support for hot state images and image shadows.
//
#ifndef _BUTTONVE_IMAGE_H_
#define _BUTTONVE_IMAGE_H_
#include "ButtonVE.h"


#ifndef _VEIMAGEPLACEMENTTYPE_
#define _VEIMAGEPLACEMENTTYPE_
typedef enum {
  IMAGEPOS_LEFT=BS_LEFT, IMAGEPOS_RIGHT=BS_RIGHT, 
  IMAGEPOS_TOP=BS_TOP, IMAGEPOS_BOTTOM=BS_BOTTOM} VEImagePlacementType;
#define ROP_PSDPxax 0x00B8074AL 
#define ROP_DSna 0x00220326L    // Dest = (not SRC) and Dest
#endif _VEIMAGEPLACEMENTTYPE_


/////////////////////////////////////////////////////////////////////////////
//
// CButtonVE_Image owner-drawn button class...
//
class CButtonVE_Image : public CButtonVE
{
  DECLARE_DYNAMIC(CButtonVE_Image)

//
// Attributes...
//
public:
  COLORREF m_transparent_color;              // Background color to consider transparent.
  BOOL m_image_shadow;                       // Controls if image should have shadow.

  HBITMAP m_hot_bitmap;                      // Image to show when button hot.
  HICON m_hot_icon;                          //   "

  HBITMAP m_disabled_bitmap;                 // Image to show when button disabled.
  HICON m_disabled_icon;                     //   "

  VEImagePlacementType m_imagetext_position; // Image placement relative to text.  Ignored if no text.
  CSize m_imagetext_spacing;                 // Spacing between icon & text.


//
// Constructor/Destructor...
//
public:
  CButtonVE_Image() : CButtonVE() {
    m_transparent_color=-1;
    m_hot_bitmap=NULL;
    m_hot_icon=NULL;
    m_disabled_bitmap=NULL;
    m_disabled_icon=NULL;
    m_imagetext_position = IMAGEPOS_LEFT; 
    m_imagetext_spacing = CSize(8,1);
    m_image_shadow = FALSE;
  }
  virtual ~CButtonVE_Image() {
    if (m_hot_bitmap) ::DeleteObject(m_hot_bitmap);
    if (m_hot_icon) ::DeleteObject(m_hot_icon);
    if (m_disabled_bitmap) ::DeleteObject(m_disabled_bitmap);
    if (m_disabled_icon) ::DeleteObject(m_disabled_icon);
  }


//
// Configuration Operations...
//
public:
  void SafeInvalidate() 
    {if (m_hWnd) if (::IsWindow(m_hWnd)) Invalidate();}

  // Force background color for bitmaps.  -1=default.
  void SetTransparentColor (COLORREF color) 
    {m_transparent_color = color;}

  // Set image position relative to text.  Valid param: 
  //   IMAGEPOS_LEFT, IMAGEPOS_RIGHT, IMAGEPOS_TOP, IMAGEPOS_BOTTOM
  void SetImagePosition (VEImagePlacementType imagepos) {
    m_imagetext_position = imagepos; 
    SafeInvalidate();
  }

  // Set spacing between icon & text...
  void SetImageSpacing(CSize spacing) {
    m_imagetext_spacing = spacing;
    SafeInvalidate();
  }

  // Contrl if image has shadow...
  void SetImageShadow(BOOL enable) {
    m_image_shadow = enable;
    SafeInvalidate();
  }


  // Specify a bitmap to use for hot image...
  HBITMAP SetHotBitmap (HBITMAP bitmap) {
    HBITMAP old_bitmap = m_hot_bitmap;
    if (m_hot_icon) {::DeleteObject(m_hot_icon); m_hot_icon=NULL;}
    m_hot_bitmap=bitmap;
    SafeInvalidate();
    return old_bitmap;
  }

  // Specify an icon to use for hot image...
  HICON SetHotIcon (HICON icon) {
    HICON old_icon = m_hot_icon;
    if (m_hot_bitmap) {::DeleteObject(m_hot_bitmap); m_hot_bitmap=NULL;}
    m_hot_icon=icon;
    SafeInvalidate();
    return old_icon;
  }

  // Specify a bitmap to use for disabled image...
  HBITMAP SetDisabledBitmap (HBITMAP bitmap) {
    HBITMAP old_bitmap = m_disabled_bitmap;
    if (m_disabled_icon) {::DeleteObject(m_disabled_icon); m_disabled_icon=NULL;}
    m_disabled_bitmap=bitmap;
    SafeInvalidate();
    return old_bitmap;
  }

  // Specify an icon to use for disabled image...
  HICON SetDisabledIcon (HICON icon) {
    HICON old_icon = m_disabled_icon;
    if (m_disabled_bitmap) {::DeleteObject(m_disabled_bitmap); m_disabled_bitmap=NULL;}
    m_disabled_icon=icon;
    SafeInvalidate();
    return old_icon;
  }

  inline HBITMAP SetHotImage (HBITMAP bitmap) {return SetHotBitmap(bitmap);}
  inline HICON SetHotImage (HICON icon) {return SetHotIcon(icon);}
  inline HBITMAP SetDisabledImage (HBITMAP bitmap) {return SetDisabledBitmap(bitmap);}
  inline HICON SetDisabledImage (HICON icon) {return SetDisabledIcon(icon);}
  inline HBITMAP SetImage (HBITMAP bitmap) {return SetBitmap(bitmap);}
  inline HICON SetImage (HICON icon) {return SetIcon(icon);}


//
// Overrideable...
//
private:
  // Draw image button content (after background prepared)...
  virtual void DrawContent (
    CDC &dc,         // Drawing context
    HTHEME hTheme,   // XP/Vista theme (if available).  Use g_xpStyle global var for drawing.
    int uistate,     // Windows keyboard/mouse ui styles.  If UISF_HIDEACCEL set, should hide underscores.
    CRect rclient,   // Button outline rectangle.
    CRect border,    // Content rectangle (specified by theme API).
    CRect textrc,    // Text rectangle.
    int text_format, // DrawText API formatting.
    BOOL enabled)    // Set if button enabled.
  {
    DWORD style = GetStyle();
    CSize imagesize(0,0);
    BITMAP bmp;

    // Identify image to draw...
    HBITMAP use_bitmap = (style & BS_BITMAP) ? GetBitmap() : NULL;
    HICON use_icon = (style & BS_ICON) ? GetIcon() : NULL;

    // If disabled, and "disabled" bitmap/icon given, use that...
    if (style & WS_DISABLED) {
      if (m_disabled_bitmap||m_disabled_icon) {
        use_bitmap = m_disabled_bitmap; 
        use_icon = m_disabled_icon;
        style &= ~WS_DISABLED;
      }
    }
    // If button hot, and "hot" bitmap/icon provided, use that...
    else if (m_button_hot)
      if (m_hot_bitmap||m_hot_icon) {
        use_bitmap = m_hot_bitmap;
        use_icon = m_hot_icon;
      }  

    // Get size of image...
    if (use_bitmap) {
      if (GetObject(use_bitmap, sizeof(bmp), &bmp)) {
        imagesize.cx = bmp.bmWidth; 
        imagesize.cy = bmp.bmHeight;
      }
    }
    else if (use_icon) {
      ICONINFO info;
      GetIconInfo(use_icon, &info);
      if (info.hbmColor) { // got color icon...
        if (GetObject(info.hbmColor, sizeof(bmp), &bmp)) {
          imagesize.cx = bmp.bmWidth; 
          imagesize.cy = bmp.bmHeight;
        }
      }
      else // only black-and-white 2X high mask given...
        if (GetObject(info.hbmMask, sizeof(bmp), &bmp)) {
          imagesize.cx = bmp.bmWidth; 
          imagesize.cy = bmp.bmHeight/2;
        }
      if (info.hbmColor) ::DeleteObject(info.hbmColor);
      if (info.hbmMask) ::DeleteObject(info.hbmMask);
    }

    // Get size of text...
    CSize textsize(0,0);
    CString button_text;
    GetWindowText(button_text);

    CFont *oldfont = (CFont*)dc.SelectObject(GetFont());
    if (!button_text.IsEmpty())
      textsize = GetTextSize(dc, button_text, textrc, text_format);

    // Compute coords for text & image...
    CPoint icoord(0,0);
    if (imagesize.cx>0)
      if (textsize.cx==0) { // no text - all image - just use button style...
        if (text_format & DT_CENTER) 
          icoord.x = textrc.left + (textrc.Width()-imagesize.cx)/2;
        else if (text_format & DT_RIGHT)
          icoord.x = textrc.right - imagesize.cx;
        else icoord.x = textrc.left;

        if (text_format & DT_VCENTER) 
          icoord.y = textrc.top + (textrc.Height()-imagesize.cy)/2;
        else if (text_format & DT_BOTTOM)
          icoord.y = textrc.bottom - imagesize.cy;
        else icoord.y = textrc.top;
      }
      else { // compute image position relative to text...
        switch (m_imagetext_position) {
          case IMAGEPOS_LEFT : { // put image to left of text
            int width = imagesize.cx + textsize.cx + m_imagetext_spacing.cx;
            if (text_format & DT_CENTER)
              icoord.x = textrc.left + (textrc.Width()-width)/2;
            else if (text_format & DT_RIGHT)
              icoord.x = textrc.right - width;
            else icoord.x = textrc.left;
            textrc.left = icoord.x+imagesize.cx+m_imagetext_spacing.cx;
            text_format = (text_format & 0xFFFFFFFC) | DT_LEFT;
            break;
          }

          case IMAGEPOS_RIGHT : { // put image to right of text
            int width = imagesize.cx + textsize.cx + m_imagetext_spacing.cx;
            if (text_format & DT_CENTER)
              icoord.x = textrc.right - (textrc.Width()-width)/2 - imagesize.cx;
            else if (text_format & DT_RIGHT)
              icoord.x = textrc.right - imagesize.cx;
            else icoord.x = textrc.left + textsize.cx + m_imagetext_spacing.cx;
            textrc.right = icoord.x-m_imagetext_spacing.cx;
            text_format = (text_format & 0xFFFFFFFC) | DT_RIGHT;
            break;
          }

          case IMAGEPOS_TOP : { // put image above text
            int height = imagesize.cy + textsize.cy + m_imagetext_spacing.cy;
            if (text_format & DT_VCENTER)
              icoord.y = textrc.top + (textrc.Height()-height)/2;
            else if (text_format & DT_BOTTOM)
              icoord.y = textrc.bottom - height;
            else icoord.y = textrc.top;
            textrc.top = icoord.y + imagesize.cy + m_imagetext_spacing.cy;
            text_format = (text_format & 0xFFFFFFF3) | DT_TOP;
            break;
          }

          case IMAGEPOS_BOTTOM : { // put image below text
            int height = imagesize.cy + textsize.cy + m_imagetext_spacing.cy;
            if (text_format & DT_VCENTER)
              icoord.y = textrc.bottom - (textrc.Height()-height)/2 - imagesize.cy;
            else if (text_format & DT_BOTTOM)
              icoord.y = textrc.bottom - imagesize.cy;
            else icoord.y = textrc.top + textsize.cy + m_imagetext_spacing.cy;
            textrc.bottom = icoord.y - m_imagetext_spacing.cy;
            text_format = (text_format & 0xFFFFFFF3) | DT_BOTTOM;
            break;
          }
        }
        switch (m_imagetext_position) {
          case IMAGEPOS_LEFT :
          case IMAGEPOS_RIGHT :
            if (text_format & DT_VCENTER) 
              icoord.y = textrc.top + (textrc.Height()-imagesize.cy)/2;
            else if (text_format & DT_BOTTOM)
              icoord.y = textrc.bottom - imagesize.cy;
            else icoord.y = textrc.top;
            break;

          case IMAGEPOS_TOP :
          case IMAGEPOS_BOTTOM :
            if (text_format & DT_CENTER)
              icoord.x = textrc.left + (textrc.Width()-imagesize.cx)/2;
            else if (text_format & DT_RIGHT)
              icoord.x = textrc.right - imagesize.cx;
            else icoord.x = textrc.left;
            break;
        }
      }

    // Draw text...
    dc.IntersectClipRect(border);
    if (textsize.cx>0)
      DrawTextContent (dc, hTheme, uistate, rclient, border, textrc, text_format, enabled, button_text);
    dc.SelectObject(oldfont);

    // Create temp dc/bitmap...
    CBitmap tBitmap;
    tBitmap.CreateCompatibleBitmap(&dc,imagesize.cx,imagesize.cy);
    CDCBitmap tempDC(dc,tBitmap);

    // Draw default background color...
    COLORREF use_transparent_color = (m_transparent_color==-1) ? m_bgcolor : m_transparent_color;
    tempDC.FillSolidRect(0,0,imagesize.cx,imagesize.cy, use_transparent_color);

    // Draw image into tempDC...
    if (use_bitmap) tempDC.DrawState(CPoint(0,0), imagesize, use_bitmap, DSS_NORMAL, HBRUSH(NULL));
    if (use_icon) tempDC.DrawState(CPoint(0,0), imagesize, use_icon, DSS_NORMAL, HBRUSH(NULL)); 

    // Draw image disabled or with transparent background...
    use_transparent_color = (m_transparent_color==-1) ? tempDC.GetPixel(0,0) : m_transparent_color;
    if (use_bitmap||use_icon)
      if (style & WS_DISABLED)
        DrawDisabledImage(dc, tempDC, icoord, imagesize, use_transparent_color);
      else {
        if (m_image_shadow) DrawBlurredShadowImage(dc, tempDC, icoord-CPoint(1,1), imagesize, use_transparent_color);
        DrawTransparentImage(dc, tempDC, icoord, imagesize, use_transparent_color);
      }
  }

//
// Message map functions...
//
protected:
  DECLARE_MESSAGE_MAP()
};

#endif // define _BUTTONVE_IMAGE_H_
  
