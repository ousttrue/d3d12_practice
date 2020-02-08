#include "GenMipMapRGBA.h"
typedef unsigned char UINT8;

//-----------------------------------------------------------------------------
// Purpose: generate next level mipmap for an RGBA image
//-----------------------------------------------------------------------------
void GenMipMapRGBA(const UINT8 *pSrc, UINT8 **ppDst, int nSrcWidth, int nSrcHeight, int *pDstWidthOut, int *pDstHeightOut)
{
    *pDstWidthOut = nSrcWidth / 2;
    if (*pDstWidthOut <= 0)
    {
        *pDstWidthOut = 1;
    }
    *pDstHeightOut = nSrcHeight / 2;
    if (*pDstHeightOut <= 0)
    {
        *pDstHeightOut = 1;
    }

    *ppDst = new UINT8[4 * (*pDstWidthOut) * (*pDstHeightOut)];
    for (int y = 0; y < *pDstHeightOut; y++)
    {
        for (int x = 0; x < *pDstWidthOut; x++)
        {
            int nSrcIndex[4];
            float r = 0.0f;
            float g = 0.0f;
            float b = 0.0f;
            float a = 0.0f;

            nSrcIndex[0] = (((y * 2) * nSrcWidth) + (x * 2)) * 4;
            nSrcIndex[1] = (((y * 2) * nSrcWidth) + (x * 2 + 1)) * 4;
            nSrcIndex[2] = ((((y * 2) + 1) * nSrcWidth) + (x * 2)) * 4;
            nSrcIndex[3] = ((((y * 2) + 1) * nSrcWidth) + (x * 2 + 1)) * 4;

            // Sum all pixels
            for (int nSample = 0; nSample < 4; nSample++)
            {
                r += pSrc[nSrcIndex[nSample]];
                g += pSrc[nSrcIndex[nSample] + 1];
                b += pSrc[nSrcIndex[nSample] + 2];
                a += pSrc[nSrcIndex[nSample] + 3];
            }

            // Average results
            r /= 4.0;
            g /= 4.0;
            b /= 4.0;
            a /= 4.0;

            // Store resulting pixels
            (*ppDst)[(y * (*pDstWidthOut) + x) * 4] = (UINT8)(r);
            (*ppDst)[(y * (*pDstWidthOut) + x) * 4 + 1] = (UINT8)(g);
            (*ppDst)[(y * (*pDstWidthOut) + x) * 4 + 2] = (UINT8)(b);
            (*ppDst)[(y * (*pDstWidthOut) + x) * 4 + 3] = (UINT8)(a);
        }
    }
}