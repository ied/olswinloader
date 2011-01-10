//
// BITMAP_HELPER.CPP
// Code contained in this file is hereby released to the public domain. 
// Ian Davis, 4/24/2007
// http://www.codeproject.com/KB/vista/vistathemebuttons.aspx
//

//
// Misc routines for manipulating Windows bitmaps...
//
#include "stdafx.h"
#include "Bitmap_Helper.h"
#include "2PassScaleEx.h"

#include "ols_winloader.h"


//
// Draw monochrome mask.  1's in the bitmap are converted to the color paremeter.   
// 0's are left unchanged in the destination.  Gotta love ROP's...
//
void DrawMonoBitmap (CDC &dc, CDC &monoDC, CPoint icoord, CSize imagesize, COLORREF color) 
{
  CBrush brush;
  brush.CreateSolidBrush(color);
  CBrush *old_brush = dc.SelectObject(&brush);
  dc.SetTextColor(RGB(0,0,0));
  dc.SetBkColor(RGB(255,255,255));
  dc.BitBlt(icoord.x, icoord.y, imagesize.cx, imagesize.cy, &monoDC, 0, 0, ROP_PSDPxax); 
  dc.SelectObject(old_brush); 
}



//
// Create "disabled" version of bitmap.   Replace all non-transparent pixels with shaded mask...
//
void DrawDisabledImage(CDC &dc, CDC &srcDC, CPoint icoord, CSize imagesize, COLORREF transparent_color) 
{
  // Create monochrome mask based on transparent color...
  CBitmap mBitmap; mBitmap.CreateBitmap(imagesize.cx, imagesize.cy, 1, 1, NULL);
  CDCBitmap monoDC(dc,mBitmap);
  srcDC.SetBkColor(transparent_color);
  monoDC.BitBlt(0, 0, imagesize.cx, imagesize.cy, &srcDC, 0, 0, SRCCOPY); 

  // Draw monochrome mask twice.  First in highlight color (offset by a pixel), then in shadow color.
  // Creates depth in the disabled image.
  DrawMonoBitmap(dc, monoDC, icoord+CPoint(1,1), imagesize, ::GetSysColor(COLOR_BTNHIGHLIGHT));
  DrawMonoBitmap(dc, monoDC, icoord, imagesize, ::GetSysColor(COLOR_BTNSHADOW));
}



//
// Draw image in source DC filtering transparent color...
//
void DrawTransparentImage(CDC &dc, CDC &srcDC, CPoint icoord, CSize imagesize, COLORREF transparent_color)
{
  // Create monochrome mask based on transparent color...
  CBitmap mBitmap; mBitmap.CreateBitmap(imagesize.cx, imagesize.cy, 1, 1, NULL);
  CDCBitmap monoDC(dc, mBitmap); 
  srcDC.SetBkColor(transparent_color);
  monoDC.BitBlt(0, 0, imagesize.cx, imagesize.cy, &srcDC, 0, 0, SRCCOPY);

  // Create dc with transparent colored pixels masked out...
  CBitmap tBitmap; tBitmap.CreateCompatibleBitmap(&dc, imagesize.cx, imagesize.cy);
  CDCBitmap maskDC(dc, tBitmap);
  maskDC.BitBlt(0, 0, imagesize.cx, imagesize.cy, &srcDC, 0, 0, SRCCOPY);
  maskDC.BitBlt(0, 0, imagesize.cx, imagesize.cy, &monoDC, 0, 0, ROP_DSna);

  // Mask out non-transparent pixels & combine with background on destination DC...
  dc.SetBkColor(RGB(255,255,255));
  dc.BitBlt(icoord.x, icoord.y, imagesize.cx, imagesize.cy, &monoDC, 0, 0, SRCAND);
  dc.BitBlt(icoord.x, icoord.y, imagesize.cx, imagesize.cy, &maskDC, 0, 0, SRCPAINT);
} 



//
// Get DIB pixels from bitmap.  Returns dword width of image if successful.
//
int GetDIBPixels(HDC dc, HBITMAP bmp, BITMAPINFO &bmi, DWORD **pixels)
{
  ASSERT (pixels);

  // Init BITMAPINFO & get image size...
  memset (&bmi, 0, sizeof(BITMAPINFOHEADER));
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
  bmi.bmiHeader.biCompression = BI_RGB;
  if (::GetDIBits(dc, bmp, 0, 0, NULL, &bmi, DIB_RGB_COLORS)==0) return 0;

  // Compute image size if driver didn't...
  #define WIDTHDWORD(bits) (((bits) + 31)>>5)
  int dword_width = WIDTHDWORD((DWORD)bmi.bmiHeader.biWidth * bmi.bmiHeader.biBitCount);
  if (bmi.bmiHeader.biSizeImage == 0) bmi.bmiHeader.biSizeImage = (dword_width<<2) * bmi.bmiHeader.biHeight;
  if (pixels==NULL) return dword_width;

  // Get pixels...
  DWORD *pixbuf = new DWORD[bmi.bmiHeader.biSizeImage];
  if (pixbuf==NULL) return 0;
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); 
  bmi.bmiHeader.biCompression = BI_RGB;
  if (::GetDIBits(dc, bmp, 0, bmi.bmiHeader.biHeight, pixbuf, &bmi, DIB_RGB_COLORS)==0) {
    delete [] pixbuf;
    return 0;
  }

  *pixels = pixbuf;
  return dword_width;
} 


// Draw image in source DC as blurred shadow filtering transparent color.  
// Uses 5x5 Guassian blurring on a DIB.  Output image is larger by 4 pixels in x/y directions.
void DrawBlurredShadowImage(CDC &dc, CDC &srcDC, CPoint icoord, CSize imagesize, COLORREF transparent_color)
{
  #define BORDER 2
  #define SRCPIX(xofs,yofs) ((BYTE)srcpix[xofs+yofs*src_width])

  // Create monochrome mask based on transparent color...
  CBitmap mBitmap; mBitmap.CreateBitmap(imagesize.cx, imagesize.cy, 1, 1, NULL);
  CDCBitmap monoDC(dc, mBitmap);
  srcDC.SetBkColor(transparent_color);
  monoDC.BitBlt(0, 0, imagesize.cx, imagesize.cy, &srcDC, 0, 0, SRCCOPY);

  // Draw mono bitmap into 32 bit image in gray (on white background), and get DIB pixel pointer...
  CSize newimgsize = imagesize+CSize(4*BORDER,4*BORDER);
  CBitmap tBitmap; 
  if (tBitmap.CreateBitmap(newimgsize.cx, newimgsize.cy, 1, 32, NULL)) {
    CDCBitmap tempDC(dc, tBitmap); 
    tempDC.FillSolidRect(0, 0, newimgsize.cx, newimgsize.cy, RGB(0xFF,0xFF,0xFF));
    DrawMonoBitmap(tempDC, monoDC, CPoint(2*BORDER,2*BORDER), imagesize, RGB(0x80,0x80,0x80));
  }

  BITMAPINFO src_bmi;
  DWORD *src_pixels=NULL;
  int src_width = GetDIBPixels(dc, tBitmap, src_bmi, &src_pixels);
  if ((src_width==0) || (src_pixels==NULL)) return;

  // Now make copy of destination & get DIB pixel pointer...
  CBitmap dBitmap;
  if (dBitmap.CreateBitmap(newimgsize.cx, newimgsize.cy, 1, 32, NULL)) {
    CDCBitmap tempDC(dc, dBitmap); 
    tempDC.BitBlt(0, 0, newimgsize.cx, newimgsize.cy, &dc, icoord.x-BORDER, icoord.y-BORDER, SRCCOPY);
  }

  BITMAPINFO dest_bmi;
  DWORD *dest_pixels=NULL;
  int dest_width = GetDIBPixels(dc, dBitmap, dest_bmi, &dest_pixels);
  if ((dest_width==0) || (dest_pixels==NULL)) {delete [] src_pixels; return;}

  // Finally... the blurring can begin!  Guassian 5x5 matrix applied.
  // A weighted average of the 25 pixels surrounding the current position.
  // Once average value computed, blend with destination image.
  int src_width2x = src_width<<1;
  int linestart = (BORDER + src_width*BORDER);
  for (int y=0; y<(imagesize.cy+2*BORDER); y++) {
    DWORD *srcpix = &src_pixels[linestart];
    DWORD *destpix = &dest_pixels[linestart];
    for (int x=0; x<(imagesize.cx+2*BORDER); x++) {
      int value = // average 5x5 matrix of pixels, weighting towards center...
        (1*SRCPIX(-2,-2) + 2*SRCPIX(-1,-2) + 3*SRCPIX(0,-2) + 2*SRCPIX(1,-2) + 1*SRCPIX(2,-2) +
         2*SRCPIX(-2,-1) + 4*SRCPIX(-1,-1) + 6*SRCPIX(0,-1) + 4*SRCPIX(1,-1) + 2*SRCPIX(2,-1) +
         3*SRCPIX(-2,0)  + 6*SRCPIX(-1,0)  + 8*SRCPIX(0,0)  + 6*SRCPIX(1,0)  + 3*SRCPIX(2,0) +
         2*SRCPIX(-2,1)  + 4*SRCPIX(-1,1)  + 6*SRCPIX(0,1)  + 4*SRCPIX(1,1)  + 2*SRCPIX(2,1)  +
         1*SRCPIX(-2,2)  + 2*SRCPIX(-1,2)  + 3*SRCPIX(0,2)  + 2*SRCPIX(1,2)  + 1*SRCPIX(2,2))/80;
      if (value>0xFF) value=0xFF;

      // Blend with destination...
      DWORD dpix = *destpix;
      *destpix = RGB((GetRValue(dpix)*value/0xFF), (GetGValue(dpix)*value/0xFF), (GetBValue(dpix)*value/0xFF));

      destpix++;
      srcpix++;
    }
    linestart += src_width;
  }

  // Draw finished shadow...
  ::StretchDIBits(dc, 
    icoord.x, icoord.y, newimgsize.cx-2*BORDER, newimgsize.cy-2*BORDER,
    BORDER, BORDER, newimgsize.cx-2*BORDER, newimgsize.cy-2*BORDER, 
    dest_pixels, &dest_bmi,
    DIB_RGB_COLORS, SRCCOPY);

  // Cleanup...
  delete [] src_pixels;
  delete [] dest_pixels;
}




// Alpha blend bitmap into destination.  Alpha is 0 (nothing) to 255 (max).
void AlphaBlt(HDC destdc, CRect rdest, HDC srcdc, CRect rsrc, int alpha) 
{
  ASSERT ((alpha>=0) && (alpha<=255));
  if (alpha<0) alpha=0;
  if (alpha>255) alpha=255;
  BLENDFUNCTION bf;
  bf.BlendOp = AC_SRC_OVER;
  bf.BlendFlags = 0;
  bf.SourceConstantAlpha = alpha;
  bf.AlphaFormat = 0; // AC_SRC_ALPHA;
  AlphaBlend(destdc, rdest.left, rdest.top, rdest.Width(), rdest.Height(), 
    srcdc, rsrc.left, rsrc.top, rsrc.Width(), rsrc.Height(), bf);
}

// Alpha blend just an empty frame, of "border_size" thickness.  Alpha is 0 (nothing) to 255 (max). 
void AlphaBltFrame(HDC destdc, HDC srcdc, CRect rc, CSize border_size, int alpha)
{
  CRect use_rc;
  use_rc.SetRect(rc.left, rc.top, rc.right, rc.top+border_size.cy);
  AlphaBlt (destdc, use_rc, srcdc, use_rc, alpha);
  use_rc.SetRect(rc.left, rc.bottom-border_size.cy, rc.right, rc.bottom);
  AlphaBlt (destdc, use_rc, srcdc, use_rc, alpha);
  use_rc.SetRect(rc.left, rc.top+border_size.cy, rc.left+border_size.cx, rc.bottom-border_size.cy);
  AlphaBlt (destdc, use_rc, srcdc, use_rc, alpha);
  use_rc.SetRect(rc.right-border_size.cx, rc.top+border_size.cy, rc.right, rc.bottom-border_size.cy);
  AlphaBlt (destdc, use_rc, srcdc, use_rc, alpha);
}



//
// Create necessary temporary drawing context & display bitmap...
//
void DrawBitmap (CDC *pDC, HBITMAP bmp, CRect dest, CPoint src) 
{
	CDC dcTmp;
	dcTmp.CreateCompatibleDC(pDC);
	HGDIOBJ old_bitmap = dcTmp.SelectObject(bmp);
	pDC->BitBlt(dest.left, dest.top, dest.Width(), dest.Height(), &dcTmp, src.x, src.y, SRCCOPY);
  dcTmp.SelectObject(old_bitmap);
}


//
// Draw bitmap with transparent color...
//
#define ROP_DSna 0x00220326L  // Dest = (not SRC) and Dest
void DrawTransparentBitmap(HDC hDC, HDC hdcSrc, short xDest, short yDest, int xSrc, int ySrc, int xSize, int ySize, COLORREF cTransparentColor)
{
  // Create work copy of bitmap...
  HDC hdcWork = CreateCompatibleDC(hDC);
  HBITMAP bmWork = CreateCompatibleBitmap(hDC, xSize, ySize);
  HBITMAP bmWorkOld = (HBITMAP)SelectObject(hdcWork, bmWork);
  BitBlt(hdcWork, 0, 0, xSize, ySize, hdcSrc, xSrc, ySrc, SRCCOPY);

  // Create some DCs to hold temporary data...
  HDC hdcMask = CreateCompatibleDC(hDC);

  // Create a bitmaps for each DC....
  HBITMAP bmAndMask = CreateBitmap(xSize, ySize, 1, 1, NULL); // Monochrome DC

  // Each DC must select a bitmap object to store pixel data.
  HBITMAP bmMaskOld = (HBITMAP)SelectObject(hdcMask, bmAndMask);

  // Set proper mapping mode.
  SetMapMode(hdcWork, GetMapMode(hDC));

  // Create the AND mask for the bitmap by BitBlting src bitmap onto mono DC.
  // Transparent color becomes white (1).  Other colors black (0).
  COLORREF cColor = SetBkColor(hdcWork, cTransparentColor);
  BitBlt(hdcMask, 0, 0, xSize, ySize, hdcWork, 0, 0, SRCCOPY);
  SetBkColor(hdcWork, cColor);

  // Mask out the transparent colored pixels on the bitmap...
  BitBlt(hdcWork, 0, 0, xSize, ySize, hdcMask, 0, 0, ROP_DSna); // Dest = (not Mask) and Dest

  // Mask out the places where the bitmap will be placed...
  cColor = SetBkColor(hDC, RGB(0xFF,0xFF,0xFF));
  BitBlt(hDC, xDest, yDest, xSize, ySize, hdcMask, 0, 0, SRCAND); // ROP_DSa

  // XOR the bitmap with the background on the destination DC...
  BitBlt(hDC, xDest, yDest, xSize, ySize, hdcWork, 0, 0, SRCPAINT); // ROP_DSo
  SetBkColor(hDC, cColor);

  // Delete the memory bitmaps.
  DeleteObject(SelectObject(hdcMask, bmMaskOld));
  DeleteObject(SelectObject(hdcWork, bmWorkOld));

  // Delete the memory DCs.
  DeleteDC(hdcMask);
  DeleteDC(hdcWork);
} 



//
// Convert bitmap into DIB (device-independent-bitmap) byte array...
//
#define WIDTHBYTES(i) ((i+31)/32*4)
bool GetBitmapPixels(
  HDC hdc, HBITMAP image, int width, int height, BOOL invert, 
  byte **dib_ptr, byte **pixel_ptr, int *pixels_bytesperline)
{
  if (image==NULL) return false;

  // Get bitmap size...
  BITMAP bmp;
  ::GetObject(image, sizeof(BITMAP), (LPSTR)&bmp);
  if (width<0) width = bmp.bmWidth;
  if (height<0) height = bmp.bmHeight;

  // Init var "pixels" with image pixels...
  BITMAPINFO bi;
  bi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth         = bmp.bmWidth;
  bi.bmiHeader.biHeight        = (invert) ? -bmp.bmHeight : bmp.bmHeight;
  bi.bmiHeader.biPlanes        = 1;
  bi.bmiHeader.biBitCount      = 24;
  bi.bmiHeader.biCompression   = BI_RGB;
  bi.bmiHeader.biSizeImage     = 0;
  bi.bmiHeader.biXPelsPerMeter = 0;
  bi.bmiHeader.biYPelsPerMeter = 0;
  bi.bmiHeader.biClrUsed       = 0;
  bi.bmiHeader.biClrImportant  = 0;
  int bytesperline = WIDTHBYTES((DWORD)bmp.bmWidth * bi.bmiHeader.biBitCount);

  // Get image size of bitmap...
  int result = ::GetDIBits(hdc, image, 0, bmp.bmHeight, NULL, &bi, DIB_RGB_COLORS);
  if (bi.bmiHeader.biSizeImage==0) bi.bmiHeader.biSizeImage = bytesperline * bmp.bmHeight;

  byte *image_dib = new byte[sizeof(BITMAPINFOHEADER)+bi.bmiHeader.biSizeImage+256];
  if (image_dib==NULL) return false;
  memcpy (image_dib, &bi, sizeof(BITMAPINFOHEADER));

  byte *pixels = image_dib+sizeof(BITMAPINFOHEADER);
  result = ::GetDIBits(hdc, image, 0, bmp.bmHeight, (LPSTR)pixels, (LPBITMAPINFO)image_dib, DIB_RGB_COLORS);

  if (result==0) {
    delete [] image_dib;
    return false;
  }

  //
  // Scale DIB if necessary...
  //
  if ((bmp.bmWidth!=width) || (bmp.bmHeight!=height)) {
    int dest_byte_size = WIDTHBYTES(width * bi.bmiHeader.biBitCount);

    BITMAPINFO sbmi;
	  sbmi.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	  sbmi.bmiHeader.biWidth         = width;
	  sbmi.bmiHeader.biHeight        = height;
	  sbmi.bmiHeader.biPlanes        = bi.bmiHeader.biPlanes;
	  sbmi.bmiHeader.biBitCount      = bi.bmiHeader.biBitCount;
    sbmi.bmiHeader.biCompression   = bi.bmiHeader.biCompression;
    sbmi.bmiHeader.biSizeImage     = dest_byte_size * height;
    sbmi.bmiHeader.biXPelsPerMeter = 0;
    sbmi.bmiHeader.biYPelsPerMeter = 0;
    sbmi.bmiHeader.biClrUsed       = 0;
    sbmi.bmiHeader.biClrImportant  = 0;

    byte *scale_image_dib = new byte[sizeof(BITMAPINFOHEADER)+sbmi.bmiHeader.biSizeImage+256];
    if (scale_image_dib==NULL) return false;
    memcpy (scale_image_dib, &sbmi, sizeof(BITMAPINFOHEADER));
    byte *dest_pixels = scale_image_dib+sizeof(BITMAPINFOHEADER);

    C2PassScale scaler;
    pixels = scaler.Scale (
      pixels, bi.bmiHeader.biBitCount/8, // src-image, bits-per-pixel
      bmp.bmWidth, bytesperline, bmp.bmHeight, // x, x-bytes, y
      dest_pixels, width, dest_byte_size, height); // x, x-bytes, y

    delete [] image_dib;
    if (pixels == NULL) {
      delete [] scale_image_dib;
      return false;
    }

    image_dib = scale_image_dib;
    bytesperline = dest_byte_size;
  }

  if (dib_ptr) *dib_ptr = image_dib;
  if (pixel_ptr) *pixel_ptr = pixels;
  if (pixels_bytesperline) *pixels_bytesperline = bytesperline;
  return true;
}   


//
// Load icon into bitmap...
//
HICON LoadIconResource (int resource_id, int cx, int cy)
{
  LPCTSTR lpszResourceName = MAKEINTRESOURCE(resource_id);
  HINSTANCE hInstImageWell = AfxFindResourceHandle(lpszResourceName, RT_ICON);
  return (HICON)LoadImage(hInstImageWell, lpszResourceName, IMAGE_ICON, cx, cy, LR_DEFAULTCOLOR);
}


//
// Load bitmap...
//
HBITMAP LoadBitmapResource (int resource_id, int cx, int cy)
{
  LPCTSTR lpszResourceName = MAKEINTRESOURCE(resource_id);
  HINSTANCE hInstImageWell = AfxFindResourceHandle(lpszResourceName, RT_BITMAP);
  return (HBITMAP)LoadImage(hInstImageWell, lpszResourceName, IMAGE_BITMAP, cx, cy, LR_LOADTRANSPARENT);
}


//
// Load icon into bitmap...
//

BOOL CBitmapEx::LoadIcon(int resource_id, COLORREF bgcolor, int cx, int cy)
{
  CDC *pDC = mainwnd(GetDC());
  if (pDC==NULL) return FALSE;

  // Get handle to icon from resources...
  HICON icon_handle = LoadIconResource(resource_id, cx, cy);

  // Create DC...
  CDC mDC;
  mDC.CreateCompatibleDC(pDC);

  // Create bitmap to hold icon...
  CreateCompatibleBitmap(pDC,cx,cy);
  CBitmap *old_bitmap = mDC.SelectObject(this);

  // Set background of bitmap to default...
  mDC.FillSolidRect(0,0,cx,cy,bgcolor^RGB(1,1,1));

  // Draw icon into bitmap.  Note: DrawIconEx used since on Vista 
  // likes scaling icons to be "helpful"...  ARGH!
  ::DrawIconEx(mDC.m_hDC, 0, 0, icon_handle, cx, cy, 0, NULL, DI_NORMAL);

  // HACK for Vista dragging bug.  Vista considers the color BLACK to be
  // transparent... in ADDITION to whatever color is supposed to be used.
  // So this XOR's all colors by RGB(1,1,1), thus no more black...
  HBRUSH hbrBtnShadow = ::CreateSolidBrush(RGB(1,1,1));
  HGDIOBJ hbrOld = mDC.SelectObject(hbrBtnShadow); 
  if (hbrOld != NULL) {
    BitBlt(mDC.m_hDC, 0, 0, cx, cy, mDC.m_hDC, 0, 0, ROP_PSDPxax); 
    mDC.SelectObject(hbrOld); 
  } 
  AfxDeleteObject((HGDIOBJ*)&hbrBtnShadow);

  // Cleanup...
  mDC.SelectObject(old_bitmap);
  mDC.DeleteDC();
  mainwnd(ReleaseDC(pDC));
  DestroyIcon(icon_handle);
  return TRUE;
}


