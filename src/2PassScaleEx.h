//
// Two Pass Scaling using Filters
// Eran Yariv, Dec 11 1999
// http://www.codeproject.com/KB/graphics/2_pass_scaling.aspx
//
// ---
//
// 1-17-2005 - Jake Montgomery - Modifed to use more integer math -- much faater. [JRM] 
// 3/18/2007 - Ian Davis - Tweaked to remove dependancy on templates...
//
#ifndef _2PASS_SCALE_H_
#define _2PASS_SCALE_H_

#include <math.h>

#define BOUND(x,a,b) \
    (((x) <= (a)) ? (a) : (((x) > (b)) ? (b) : (x)))

#define PS_MAX_DEPTH 4
              
// Based on Eran Yariv's TwoPassScale, modified for speed, bug fixes, and variable depths.

// Does 2 pass scaling of bitmaps
// Template based, can use various methods for interpolation.
// Could certainly be improved.

//    to use:
//    C2PassScale  ScaleEngine;
//    ScaleEngine.Scale ((UCHAR *)dibSrc.GetBits(),		  // A pointer to the source bitmap bits
//                        depth,						  // The size of a single pixel in bytes (both source and scaled image)
//                        dibSrc.Width(),				  // The width of a line of the source image to scale in pixels
//                        dibSrc.WidthBytes(),			  // The width of a single line of the source image in bytes (to allow for padding, etc.)
//                        dibSrc.Height(),				  // The height of the source image to scale in pixels.
//                        (UCHAR *)GetBits(),			  // A pointer to a buffer to hold the ecaled image
//                        Width(),						  // The desired width of a line of the scaled image in pixels
//                        WidthBytes(),					  // The width of a single line of the scaled image in bytes (to allow for padding, etc.)
//                        Height());					  // The desired height of the scaled image in pixels.
//    or AllocAndScale()



#define FilterClass CBilinearFilter

class CGenericFilter
{
public:
    
    CGenericFilter (double dWidth) : m_dWidth (dWidth) {}
    virtual ~CGenericFilter() {}

    double GetWidth()                   { return m_dWidth; }
    void   SetWidth (double dWidth)     { m_dWidth = dWidth; }

    virtual double Filter (double dVal) = 0;

protected:

    #define FILTER_PI  double (3.1415926535897932384626433832795)
    #define FILTER_2PI double (2.0 * 3.1415926535897932384626433832795)
    #define FILTER_4PI double (4.0 * 3.1415926535897932384626433832795)

    double  m_dWidth;
};

class CBoxFilter : public CGenericFilter
{
public:

    CBoxFilter (double dWidth = double(0.5)) : CGenericFilter(dWidth) {}
    virtual ~CBoxFilter() {}

    double Filter (double dVal) { return (fabs(dVal) <= m_dWidth ? 1.0 : 0.0); }
};

class CBilinearFilter : public CGenericFilter
{
public:

    CBilinearFilter (double dWidth = double(1.0)) : CGenericFilter(dWidth) {}
    virtual ~CBilinearFilter() {}

    double Filter (double dVal)
        {
            dVal = fabs(dVal);
            return (dVal < m_dWidth ? m_dWidth - dVal : 0.0);
        }
};

class CGaussianFilter : public CGenericFilter
{
public:

    CGaussianFilter (double dWidth = double(3.0)) : CGenericFilter(dWidth) {}
    virtual ~CGaussianFilter() {}

    double Filter (double dVal)
        {
            if (fabs (dVal) > m_dWidth)
            {
                return 0.0;
            }
            return exp (-dVal * dVal / 2.0) / sqrt (FILTER_2PI);
        }
};

class CHammingFilter : public CGenericFilter
{
public:

    CHammingFilter (double dWidth = double(0.5)) : CGenericFilter(dWidth) {}
    virtual ~CHammingFilter() {}

    double Filter (double dVal)
        {
            if (fabs (dVal) > m_dWidth)
            {
                return 0.0;
            }
            double dWindow = 0.54 + 0.46 * cos (FILTER_2PI * dVal);
            double dSinc = (dVal == 0) ? 1.0 : sin (FILTER_PI * dVal) / (FILTER_PI * dVal);
            return dWindow * dSinc;
        }
};

class CBlackmanFilter : public CGenericFilter
{
public:

    CBlackmanFilter (double dWidth = double(0.5)) : CGenericFilter(dWidth) {}
    virtual ~CBlackmanFilter() {}

    double Filter (double dVal)
        {
            if (fabs (dVal) > m_dWidth)
            {
                return 0.0;
            }
            double dN = 2.0 * m_dWidth + 1.0;
            return 0.42 + 0.5 * cos (FILTER_2PI * dVal / ( dN - 1.0 )) +
                   0.08 * cos (FILTER_4PI * dVal / ( dN - 1.0 ));
        }
};


typedef struct
{
   unsigned int*Weights;  // Normalized weights of neighboring pixels
   int Left,Right;   // Bounds of source pixels window
} ContributionType;  // Contirbution information for a single pixel

typedef struct
{
   ContributionType *ContribRow; // Row (or column) of contribution weights
   UINT WindowSize,              // Filter window size (of affecting source pixels)
        LineLength;              // Length of line (no. or rows / cols)
} LineContribType;               // Contribution information for an entire line (row or column)



typedef BOOL (*ProgressAnbAbortCallBack)(BYTE bPercentComplete);


//template 
class C2PassScale
{
public:

    C2PassScale (ProgressAnbAbortCallBack callback = NULL) :
        m_Callback (callback) {m_byteDepth = 3;}

    virtual ~C2PassScale() {}

    UCHAR * AllocAndScale (  
        UCHAR   *pOrigImage,
        UINT         pixelBytes,
        UINT        uOrigWidth,
        UINT        uOrigWidthBytes,
        UINT        uOrigHeight,
        UINT        uNewWidth,
        UINT        uNewWidthBytes,
        UINT        uNewHeight);

    UCHAR * Scale (  
        UCHAR   *pOrigImage,
        UINT         pixelBytes,
        UINT        uOrigWidth,
        UINT        uOrigWidthBytes,
        UINT        uOrigHeight,
        UCHAR   *pDstImage,
        UINT        uNewWidth,
        UINT        uNewWidthBytes,
        UINT        uNewHeight);

        int                            m_byteDepth;

private:

    ProgressAnbAbortCallBack    m_Callback;
    BOOL                        m_bCanceled;

    LineContribType *AllocContributions (   UINT uLineLength,
                                            UINT uWindowSize);

    void FreeContributions (LineContribType * p);

    LineContribType *CalcContributions (    UINT    uLineSize,
                                            UINT    uSrcSize,
                                            double  dScale);

    void ScaleRow ( UCHAR           *pSrc,
                    UINT                uSrcWidth,
                    UINT                 uSrcWidthBytes,
                    UCHAR           *pRes,
                    UINT                uResWidth,
                    UINT                 uDstWidthBytes,
                    UINT                uRow,
                    LineContribType    *Contrib);

    void HorizScale (   UCHAR           *pSrc,
                        UINT                uSrcWidth,
                        UINT                 uSrcWidthBytes,
                        UINT                uSrcHeight,
                        UCHAR           *pDst,
                        UINT                uResWidth,
                        UINT                 uResWidthBytes);

    void ScaleCol ( UCHAR           *pSrc,
                    UINT                uSrcWidth,
                    UINT                 uSrcWidthBytes,
                    UCHAR           *pRes,
                    UINT                uResWidth,
                    UINT                 uResWidthBytes,
                    UINT                uResHeight,
                    UINT                uCol,
                    LineContribType    *Contrib);

    void VertScale (    UCHAR           *pSrc,
                        UINT                uSrcWidth,
                        UINT                 uSrcWidthBytes,
                        UINT                uSrcHeight,
                        UCHAR           *pDst,
                        UINT                uResHeight);
};

#endif // _2PASS_SCALE_H_
