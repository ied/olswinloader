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
#include "stdafx.h"
#include "2PassScaleEx.h"

//template 
LineContribType *C2PassScale::AllocContributions (UINT uLineLength, UINT uWindowSize)
{
    LineContribType *res = new LineContribType;
        // Init structure header
    res->WindowSize = uWindowSize;
    res->LineLength = uLineLength;
        // Allocate list of contributions
    res->ContribRow = new ContributionType[uLineLength];
    for (UINT u = 0 ; u < uLineLength ; u++)
    {
        // Allocate contributions for every pixel
        res->ContribRow[u].Weights = new unsigned int[uWindowSize];
    }
    return res;     
}

//template 
void C2PassScale::FreeContributions (LineContribType * p)
{
    for (UINT u = 0; u < p->LineLength; u++)
    {
        // Free contribs for every pixel
        delete [] p->ContribRow[u].Weights;
    }
    delete [] p->ContribRow;    // Free list of pixels contribs
    delete p;                   // Free contribs header
}

//template 
LineContribType *C2PassScale::CalcContributions (UINT uLineSize, UINT uSrcSize, double dScale)
{
    FilterClass CurFilter;

    double dWidth;
    double dFScale = 1.0;
    double dFilterWidth = CurFilter.GetWidth();

    if (dScale < 1.0)
    {    // Minification
        dWidth = dFilterWidth / dScale;
        dFScale = dScale;
    }
    else
    {    // Magnification
        dWidth= dFilterWidth;
    }

    // Window size is the number of sampled pixels
//    int iWindowSize = 2 * (int)ceil(dWidth) + 1;
    int iWindowSize = 2 * ((int)ceil(dWidth) + 1);     // changed ... causing crash with bi-linear filiter?? [JRM]

    // Allocate a new line contributions strucutre
    LineContribType *res = AllocContributions (uLineSize, iWindowSize);

    double *dWeights = new double[iWindowSize];

    for (UINT u = 0; u < uLineSize; u++)
    {   // Scan through line of contributions
        double dCenter = (double)u / dScale;   // Reverse mapping
        // Find the significant edge points that affect the pixel
        int iLeft = max (0, (int)floor (dCenter - dWidth));
        int iRight = min ((int)ceil (dCenter + dWidth), int(uSrcSize) - 1);

        // Cut edge points to fit in filter window in case of spill-off
        if (iRight - iLeft + 1 > iWindowSize)
        {
            if (iLeft < (int(uSrcSize) - 1 / 2))
            {
                iLeft++;
            }
            else
            {
                iRight--;
            }
        }
        int nFallback = iLeft;    

        BOOL bNonZeroFound = false;
        double dTotalWeight = 0.0;  // Zero sum of weights
        double dVal;
        for (int iSrc = iLeft; iSrc <= iRight; iSrc++)
        {   // Calculate weights
            dVal = CurFilter.Filter (dFScale * (dCenter - (double)iSrc));
            if (dVal > 0.0)
                dVal *= dFScale;
            else
            {
                dVal = 0.0;
                // zero conribution, trim
                if (!bNonZeroFound)
                {
                    // we are on the left side, trim left
                    iLeft = iSrc+1;
                    continue;
                }
                else
                {
                    // we are on the right side, trim right
                    iRight = iSrc-1;
                    break;
                }
            }
            bNonZeroFound = true;
            dTotalWeight += dVal;
            dWeights[iSrc-iLeft] = dVal;
        }

        if (iLeft > iRight)
        {
            ASSERT(FALSE);
            iLeft = iRight = nFallback;
            dWeights[0] = 0.0;
        }
        res->ContribRow[u].Left = iLeft;
        res->ContribRow[u].Right = iRight;

        ASSERT (dTotalWeight >= 0.0);   // An error in the filter function can cause this
        if (dTotalWeight > 0.0)
        {   // Normalize weight of neighbouring points
            for (iSrc = iLeft; iSrc <= iRight; iSrc++)
            {   // Normalize point
                dWeights[iSrc-iLeft] /= dTotalWeight;
            }
        }
        // scale weights to integers weights for effeciency
        for (iSrc = iLeft; iSrc <= iRight; iSrc++)
            res->ContribRow[u].Weights[iSrc-iLeft] = (unsigned int)(dWeights[iSrc-iLeft] * 0xffff);
   }
   delete [] dWeights;
   return res;
}


//template 
void C2PassScale::ScaleRow (  UCHAR           *pSrc,
            UINT                uSrcWidth,
            UINT                 uSrcWidthBytes,
            UCHAR           *pRes,
            UINT                uResWidth,
            UINT                 uResWidthBytes,
            UINT                uRow,
            LineContribType    *Contrib)
{
    UCHAR * const pSrcRow = &(pSrc[uRow * uSrcWidthBytes]);
    UCHAR * const pDstRow = &(pRes[uRow * uResWidthBytes]);
    UCHAR *pSrcLoc;
    UCHAR *pDstLoc;
    unsigned int vals[PS_MAX_DEPTH];

    for (UINT x = 0; x < uResWidth; x++)
    {   // Loop through row
        int v, i;
        for (v= 0; v < m_byteDepth; v++)
            vals[v] = 0;
        int iLeft = Contrib->ContribRow[x].Left;    // Retrieve left boundries
        int iRight = Contrib->ContribRow[x].Right;  // Retrieve right boundries
        pSrcLoc = &pSrcRow[iLeft*m_byteDepth];
        for (i = iLeft; i <= iRight; i++)
        {   // Scan between boundries
            #ifdef _DEBUG
                ASSERT(i-iLeft < (int)Contrib->WindowSize);
            #endif
            // Accumulate weighted effect of each neighboring pixel
            for (v= 0; v < m_byteDepth; v++)
                vals[v] += Contrib->ContribRow[x].Weights[i-iLeft] * *pSrcLoc++;
        }
        pDstLoc = &pDstRow[x*m_byteDepth];
        for (v= 0; v < m_byteDepth; v++)
        {
            // copy to destination, and scale back down by BYTE
            *pDstLoc++ = BOUND(vals[v] >> 16, 0, 0xff); // Place result in destination pixel
        }
    }
}

//template 
void C2PassScale::HorizScale (    UCHAR           *pSrc,
                UINT                uSrcWidth,
                UINT                 uSrcWidthBytes,
                UINT                uSrcHeight,
                UCHAR           *pDst,
                UINT                uResWidth,
                UINT                 uResWidthBytes)
{
    // Assumes heights are the same
//    TRACE ("Performing horizontal scaling...\n");
    if (uResWidth == uSrcWidth)
    {   // No scaling required, just copy
        if(uSrcHeight <= 0) return;
        if (uResWidthBytes == uSrcWidthBytes)
        {
            int copy = ((uSrcHeight -1) * uSrcWidthBytes) + uSrcWidth*m_byteDepth; // avoids overrun if starting in middle of image.
               memcpy (pDst, pSrc, copy);
            return;
        }
        else
        {
            for (UINT y = 0; y < uSrcHeight; y++)
                memcpy(pDst+uResWidthBytes*y, pSrc+uSrcWidthBytes*y, uSrcWidth*m_byteDepth);
            return;
        }
    }
    // Allocate and calculate the contributions
    LineContribType * Contrib = CalcContributions (uResWidth, uSrcWidth, double(uResWidth) / double(uSrcWidth));
    for (UINT u = 0; u < uSrcHeight; u++)
    {   // Step through rows
        if (NULL != m_Callback)
        {
            //
            // Progress and report callback supplied
            //
            if (!m_Callback (BYTE(double(u) / double (uSrcHeight) * 50.0)))
            {
                //
                // User wished to abort now
                //
                m_bCanceled = TRUE;
                FreeContributions (Contrib);  // Free contributions structure
                return;
            }
        }
                
        ScaleRow (  pSrc,
                    uSrcWidth,
                    uSrcWidthBytes,
                    pDst,
                    uResWidth,
                    uResWidthBytes,
                    u,
                    Contrib);    // Scale each row
    }
    FreeContributions (Contrib);  // Free contributions structure
}

//template 
void C2PassScale::ScaleCol (  UCHAR           *pSrc,
            UINT                uSrcWidth,
            UINT                 uSrcWidthBytes,
            UCHAR           *pRes,
            UINT                uResWidth,
            UINT                 uResWidthBytes,
            UINT                uResHeight,
            UINT                uCol,
            LineContribType    *Contrib)
{
    UCHAR *pSrcLoc;
    UCHAR *pDstLoc;
    unsigned int vals[PS_MAX_DEPTH];

    // assumes same height
    for (UINT y = 0; y < uResHeight; y++)
    {    // Loop through column
        int v, i;
        for (v= 0; v < m_byteDepth; v++)
            vals[v] = 0;

        int iLeft = Contrib->ContribRow[y].Left;    // Retrieve left boundries
        int iRight = Contrib->ContribRow[y].Right;  // Retrieve right boundries
        pSrcLoc = pSrc + uSrcWidthBytes*iLeft + uCol* m_byteDepth;
        for (i = iLeft; i <= iRight; i++)
        {   // Scan between boundries
            // Accumulate weighted effect of each neighboring pixel
            UCHAR *pCurSrc = pSrc + uSrcWidthBytes*i + uCol* m_byteDepth;
            #ifdef _DEBUG
                ASSERT(i-iLeft < (int)Contrib->WindowSize);
            #endif
            for (v= 0; v < m_byteDepth; v++)
                vals[v] += Contrib->ContribRow[y].Weights[i-iLeft] * pSrcLoc[v];
            pSrcLoc += uSrcWidthBytes;
        }
        pDstLoc = pRes + (y * uResWidthBytes) + uCol*m_byteDepth;
        for (v= 0; v < m_byteDepth; v++)
        {
            // scale back
            *pDstLoc++ = BOUND( vals[v] >> 16, 0, 0xff);   // Place result in destination pixel
        }
    }
}


//template 
void C2PassScale::VertScale ( UCHAR           *pSrc,
            UINT                uSrcWidth,
            UINT                 uSrcWidthBytes,
            UINT                uSrcHeight,
            UCHAR           *pDst,
            UINT                uResHeight)
{
//    TRACE ("Performing vertical scaling...");

    // assumes widths are the same!
    if (uSrcHeight == uResHeight)
    {   // No scaling required, just copy
        if (uSrcHeight <= 0) return;
        int copy = ((uSrcHeight -1) * uSrcWidthBytes) + uSrcWidth*m_byteDepth; // avoids overrun if starting in middle of image.
        memcpy (pDst, pSrc, copy);
        return;
    }
    // Allocate and calculate the contributions
    LineContribType * Contrib = CalcContributions (uResHeight, uSrcHeight, double(uResHeight) / double(uSrcHeight));
    for (UINT u = 0; u < uSrcWidth; u++)
    {   // Step through columns
        if (NULL != m_Callback)
        {
            //
            // Progress and report callback supplied
            //
            if (!m_Callback (BYTE(double(u) / double (uSrcWidth) * 50.0) + 50))
            {
                //
                // User wished to abort now
                //
                m_bCanceled = TRUE;
                FreeContributions (Contrib);  // Free contributions structure
                return;
            }
        }
        ScaleCol (  pSrc,
                    uSrcWidth,
                    uSrcWidthBytes,
                    pDst,
                    uSrcWidth,
                    uSrcWidthBytes,
                    uResHeight,
                    u,
                    Contrib);   // Scale each column
    }
    FreeContributions (Contrib);     // Free contributions structure
}

//template 
UCHAR *C2PassScale::AllocAndScale (
    UCHAR   *pOrigImage,			// A pointer to the source bitmap bits
    UINT         pixelBytes,		// The size of a single pixel in bytes (both source and scaled image)
    UINT        uOrigWidth,			// The width of a line of the source image to scale in pixels
    UINT        uOrigWidthBytes,	// The width of a single line of the source image in bytes (to allow for padding, etc.)
    UINT        uOrigHeight,		// The height of the source image to scale in pixels.
    UINT        uNewWidth,			// The desired width of a line of the scaled image in pixels
    UINT        uNewWidthBytes,		// The width of a single line of the scaled image in bytes (to allow for padding, etc.)
    UINT        uNewHeight)			// The desired height of the scaled image in pixels.
{
    // Scale source image horizontally into temporary image
    m_byteDepth = pixelBytes;
    m_bCanceled = FALSE;
    UCHAR *pTemp = new UCHAR [uNewWidthBytes * uOrigHeight];
    HorizScale (pOrigImage,
                uOrigWidth,
                uOrigWidthBytes,
                uOrigHeight,
                pTemp,
                uNewWidth,
                uNewWidthBytes);
    if (m_bCanceled)
    {
        delete [] pTemp;
        return NULL;
    }
    // Scale temporary image vertically into result image    
    UCHAR *pRes = new UCHAR [uNewWidth * uNewHeight *m_byteDepth];
    VertScale ( pTemp,
                uNewWidth,
                uNewWidthBytes,
                uOrigHeight,
                pRes,
                uNewHeight);
    if (m_bCanceled)
    {
        delete [] pTemp;
        delete [] pRes;
        return NULL;
    }
    delete [] pTemp;
    return pRes;
}

//template 
UCHAR *C2PassScale::Scale (
    UCHAR   *pOrigImage,			// A pointer to the source bitmap bits
    UINT         pixelBytes,		// The size of a single pixel in bytes (both source and scaled image)
    UINT        uOrigWidth,			// The width of a line of the source image to scale in pixels
    UINT        uOrigWidthBytes,	// The width of a single line of the source image in bytes (to allow for padding, etc.)
    UINT        uOrigHeight,		// The height of the source image to scale in pixels.
    UCHAR   *pDstImage,				// A pointer to a buffer to hold the ecaled image
    UINT        uNewWidth,			// The desired width of a line of the scaled image in pixels
    UINT        uNewWidthBytes,		// The width of a single line of the scaled image in bytes (to allow for padding, etc.)
    UINT        uNewHeight)			// The desired height of the scaled image in pixels.
{
    // Scale source image horizontally into temporary image
    ASSERT(PS_MAX_DEPTH >= pixelBytes);
    m_byteDepth = pixelBytes;
    m_bCanceled = FALSE;
    UCHAR *pTemp = new UCHAR [ uOrigHeight *uNewWidthBytes];
    HorizScale (pOrigImage,
                uOrigWidth,
                uOrigWidthBytes,
                uOrigHeight,
                pTemp,
                uNewWidth,
                uNewWidthBytes);
    if (m_bCanceled)
    {
        delete [] pTemp;
        return NULL;
    }

    // Scale temporary image vertically into result image    
    VertScale ( pTemp,
                uNewWidth,
                uNewWidthBytes,
                uOrigHeight,
                pDstImage,
                uNewHeight);
    delete [] pTemp;
    if (m_bCanceled)
    {
        return NULL;
    }
    return pDstImage;
}
