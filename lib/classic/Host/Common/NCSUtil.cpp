#include "NCSDefs.h"
#include "fxUtils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Malloc.
//
void *NCSMalloc(UINT iSize, BOOLEAN bClear)
{
    void *p = MEM_ALLOC(iSize);

    if (p && bClear)
    {
        memset(p, 0, iSize);
    }

    return p;
}

void *NCSRealloc(void *pPtr, UINT iSize, BOOLEAN bClear)
{
    VOID *p = NULL;

    if (pPtr == NULL) {
        p = MEM_ALLOC(iSize);
    } else {
        p = MEM_REALLOC(pPtr, iSize);
    }

    return p;
}

void NCSFree(void *pPtr)
{
    MEM_FREE(pPtr);
}

UINT16 NCSByteSwap16(UINT16 n)
{
    UINT16          res;
    unsigned char   ch;

    union
        {
        UINT16          a;
        unsigned char   b[2];
        } un;

    un.a = n;
    ch = un.b[0];
    un.b[0] = un.b[1];
    un.b[1] = ch;

    res = un.a;
    return res;
}

void NCSByteSwapRange16(UINT16 *pDst, UINT16 *pSrc, INT nValues)
{
    while(nValues-- > 0) {
        *pDst++ = NCSByteSwap16( *pSrc++ );
    }
}

UINT NCSByteSwap32(UINT n)
{
    UINT  res;
    UINT16  ch;

    union
        {
        UINT   a;
        UINT16   b[2];
        } un;

    //
    //  Swap HI/LO word
    //
    un.a = n;

    ch = un.b[0];
    un.b[0] = un.b[1];
    un.b[1] = ch;

    //
    //  Swap Bytes
    //
    un.b[0] = NCSByteSwap16(un.b[0]);
    un.b[1] = NCSByteSwap16(un.b[1]);


    res = un.a;
    return res;
}

void NCSByteSwapRange32(UINT *pDst, UINT *pSrc, INT nValues)
{
    while(nValues-- > 0) {
        *pDst++ = NCSByteSwap32( *pSrc++ );
    }
}

UINT64 NCSByteSwap64(UINT64 n)
{
    register int i;
    UINT8	temp;
    union {
        UINT8	dataChar[8]; /**[02]**/
        UINT64	dataInt;
    } dataChanger;

    dataChanger.dataInt = n;

    for(i = 0; i < 4; ++i) {
        temp = dataChanger.dataChar[i];
        dataChanger.dataChar[i] = dataChanger.dataChar[7-i];
        dataChanger.dataChar[7-i] = temp;
    }
    return(dataChanger.dataInt);
}

void NCSByteSwapRange64(UINT64 *pDst, UINT64 *pSrc, INT nValues)
{
    register int count;

    for(count = 0; count < nValues; count++) {
        *pDst++ = NCSByteSwap64(*pSrc++);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Pool.
//

static void NCSPoolFreeNodeContents(NCSPoolNode *pNode);
static BOOLEAN NCSPoolInitNodeContents(NCSPool *pPool, NCSPoolNode *pNode);
static NCSPoolNode *NCSPoolAddNode(NCSPool *pPool);
static void NCSPoolRemoveNode(NCSPool *pPool, NCSPoolNode *pNode);
static void *NCSPoolGetElement(NCSPool *pPool);
static void NCSPoolFreeElement(NCSPool *pPool, void *pElement);

NCSPool *NCSPoolCreate(UINT iElementSize, UINT nElementsPerNode)
{
    NCSPool *pPool = (NCSPool*)NULL;

    if(NULL != (pPool = (NCSPool*)NCSMalloc(sizeof(NCSPool), TRUE))) {
        pPool->psStats.iElementSize = iElementSize;
        pPool->psStats.nElementsPerNode = nElementsPerNode;
        pPool->nMaxElements = 0; 
    }
    return(pPool);
}

void NCSPoolDestroy(NCSPool *pPool)
{
    if(pPool) {
        INT iNode;

        iNode = (INT)pPool->psStats.nNodes;

        while(iNode-- > 0) {
            NCSPoolRemoveNode(pPool, pPool->pNodes);
        }
        
        NCSFree(pPool);
    }
}

void NCSPoolSetMaxSize(NCSPool *pPool, 
                       UINT nMaxElements)
{
    if(pPool) {
        pPool->nMaxElements = nMaxElements;
    }
}

void *NCSPoolAlloc(NCSPool *pPool, BOOLEAN bClear)
{
    void *pData;
    NCSTimeStampMs tsStart = 0;
    
    if(pPool->bCollectStats) {
        tsStart = NCSGetTimeStampMs();
    }

    if(NULL != (pData = NCSPoolGetElement(pPool))) {
        if(bClear) {
            memset(pData, 0, pPool->psStats.iElementSize);
        }
    } else {
        pData = NCSMalloc(pPool->psStats.iElementSize, bClear);
    }
    if(pPool->bCollectStats) {
        pPool->psStats.nAllocElements += 1;
        pPool->psStats.tsAllocElementTime += NCSGetTimeStampMs() - tsStart;
    }
    
    return(pData);
}

void NCSPoolFree(NCSPool *pPool, void *pPtr)
{
    if(pPtr) {
        NCSTimeStampMs tsStart = 0;
        
        if(pPool->bCollectStats) {
            tsStart = NCSGetTimeStampMs();
        }

        NCSPoolFreeElement(pPool, pPtr);

        if(pPool->bCollectStats) {
            pPool->psStats.nFreeElements += 1;
            pPool->psStats.tsFreeElementTime += NCSGetTimeStampMs() - tsStart;
        }
    }
}

static BOOLEAN NCSPoolInitNodeContents(NCSPool *pPool, NCSPoolNode *pNode)
{
    if(pNode) {
        pNode->iLastFreeElement = 0;
        if(NULL != (pNode->pElements = (void*)NCSMalloc(pPool->psStats.nElementsPerNode * pPool->psStats.iElementSize, FALSE))) {
            if(NULL != (pNode->pbElementInUse = (BOOLEAN*)NCSMalloc(pPool->psStats.nElementsPerNode * sizeof(BOOLEAN), TRUE))) {
                return(TRUE);
            }
            NCSPoolFreeNodeContents(pNode);
        }
    }
    return(FALSE);
}

static void NCSPoolFreeNodeContents(NCSPoolNode *pNode)
{
    if(pNode) {
        NCSFree(pNode->pbElementInUse);
        NCSFree(pNode->pElements);
    }
}

static NCSPoolNode *NCSPoolAddNode(NCSPool *pPool)
{
    NCSPoolNode *pNode = (NCSPoolNode*)NULL;
    NCSTimeStampMs tsStart = 0;
    
    if(pPool->bCollectStats) {
        tsStart = NCSGetTimeStampMs();
    }
    NCSArrayAppendElement(NCSPoolNode, pPool->pNodes, pPool->psStats.nNodes, (NCSPoolNode*)NULL);
    pNode = &(pPool->pNodes[pPool->psStats.nNodes - 1]);

    if(NCSPoolInitNodeContents(pPool, pNode)) {
        if(pNode->pElements && pNode->pbElementInUse) {
            if(pPool->bCollectStats) {
                pPool->psStats.nAddNodes++;
                pPool->psStats.tsAddNodeTime += NCSGetTimeStampMs() - tsStart;
            }
        } else {
            NCSPoolRemoveNode(pPool, pNode);
            pNode = (NCSPoolNode*)NULL;
        }
    }
    return(pNode);
}

static void NCSPoolRemoveNode(NCSPool *pPool, NCSPoolNode *pNode)
{
    if(pNode) {
        UINT iNode;
        NCSTimeStampMs tsStart = 0;
        
        if(pPool->bCollectStats) {
            tsStart = NCSGetTimeStampMs();
        }
        NCSPoolFreeNodeContents(pNode);

        for(iNode = 0; iNode < pPool->psStats.nNodes; iNode++) {
            if(pNode == &(pPool->pNodes[iNode])) {
                NCSArrayRemoveElement(NCSPoolNode, pPool->pNodes, pPool->psStats.nNodes, iNode);

                if(pPool->bCollectStats) {
                    pPool->psStats.nRemoveNodes++;
                }
                break;
            }
        }
        if(pPool->bCollectStats) {
            pPool->psStats.tsRemoveNodeTime += NCSGetTimeStampMs() - tsStart;
        }
    }
}

static void *NCSPoolGetElement(NCSPool *pPool)
{
    UINT iNode;
    NCSPoolNode *pNode;

    if((pPool->nMaxElements != 0) && 
       (pPool->psStats.nElementsInUse >= pPool->nMaxElements)) {
        return((void*)NULL);
    }
    for(iNode = 0; iNode < pPool->psStats.nNodes; iNode++) {
        INT iElement;
        INT nElements;
        BOOLEAN *pbElementInUse;

        pNode = &(pPool->pNodes[iNode]);
        nElements = (INT)pPool->psStats.nElementsPerNode;

        if(pNode->nElementsInUse == nElements) {
            continue;
        }
        pbElementInUse = pNode->pbElementInUse;

        for(iElement = pNode->iLastFreeElement; iElement < nElements; iElement++) {
            if(pbElementInUse[iElement] == FALSE) {
                pbElementInUse[iElement] = TRUE;
                pNode->nElementsInUse += 1;
                pNode->iLastFreeElement = iElement;

                return((void*)&(((UINT8*)(pNode->pElements))[iElement * pPool->psStats.iElementSize]));
            }
        }
    }
    if(NULL != (pNode = NCSPoolAddNode(pPool))) {
        pNode->pbElementInUse[0] = TRUE;
        pNode->nElementsInUse += 1;

        return(pNode->pElements);
    }
    return((void*)NULL);
}


static void NCSPoolFreeElement(NCSPool *pPool, void *pElement)
{
    UINT iNode;
    NCSPoolNode *pNode;

    for(iNode = 0; iNode < pPool->psStats.nNodes; iNode++) {
        INT64 iElement;
        INT nElements;

        pNode = &(pPool->pNodes[iNode]);
        nElements = (INT)pPool->psStats.nElementsPerNode;

        if((pElement >= pNode->pElements) && (
                (UINT8*)pElement < (UINT8*)pNode->pElements + pPool->psStats.iElementSize * nElements)) {
            iElement = (INT64)((INT64)pElement - (INT64)pNode->pElements) / (INT64)pPool->psStats.iElementSize;

            pNode->pbElementInUse[iElement] = FALSE;

            pNode->nElementsInUse -= 1;
            pNode->iLastFreeElement = MIN(iElement, pNode->iLastFreeElement);

            /*
            ** Leave at least 1 node in the pool at all time, in case we're pumped empty.
            ** It'll get freed on the NCSPoolDestroy();
            */
            if((pNode->nElementsInUse == 0) && (pPool->psStats.nNodes > 1)) {
                NCSPoolRemoveNode(pPool, pNode);
            }
            pElement = (void*)NULL;
            break;
        }
    }
    NCSFree(pElement);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// GetTimeStampMs.
//

NCSTimeStampMs NCSGetTimeStampMs(void)
{
    static NCSTimeStampMs tsLast = 0;
    static NCSTimeStampMs tsAdd = 0;
    NCSTimeStampMs tsLocalLast = tsLast;
    NCSTimeStampMs tsLocalAdd = tsAdd;
    NCSTimeStampMs tsNow;

    tsNow = (NCSTimeStampMs)FxGetTickCount();

    if(tsNow < tsLocalLast) {		// wrapped
        tsLocalAdd += (NCSTimeStampMs)0x100000000;		
        tsAdd = tsLocalAdd;
    }
    tsLast = tsNow;
    return(tsNow + tsLocalAdd);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// File.
//

INT64 NCSFileSeek(NCS_FILE_HANDLE hFile,
                  INT64 nOffset,
                  NCS_FILE_ORIGIN origin)
{
    fseek(hFile, (INT)nOffset, origin);
    return (INT64)ftell(hFile);
}

INT64 NCSFileTell(NCS_FILE_HANDLE hFile)
{
    return (INT64)ftell(hFile);
}

NCSError NCSFileOpen(const char *szFilename, int iFlags, NCS_FILE_HANDLE *phFile)
{
    *phFile = fopen(szFilename, "rb");

    if(*phFile != NULL) {
        return(NCS_SUCCESS);
    } else {
        return(NCS_FILE_OPEN_FAILED);
    }
}

NCSError NCSFileClose(NCS_FILE_HANDLE hFile)
{
    if (hFile) {
        fclose(hFile);
    }

    return(NCS_SUCCESS);
}

NCSError NCSFileRead(NCS_FILE_HANDLE hFile, void *pBuffer, UINT nLength, UINT* pRead)
{
    if (fread((void *)pBuffer, nLength, 1, hFile) == 1) {
        *pRead = nLength;
        return(NCS_SUCCESS);
    } 
    else {
        return(NCS_FILE_IO_ERROR);
    }
}

NCSError NCSFileReadUINT8_MSB(NCS_FILE_HANDLE hFile, UINT8 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    return(NCSFileRead(hFile, pBuffer, nSize, &nSize));
}

NCSError NCSFileReadUINT8_LSB(NCS_FILE_HANDLE hFile, UINT8 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    return(NCSFileRead(hFile, pBuffer, nSize, &nSize));
}

NCSError NCSFileReadUINT16_MSB(NCS_FILE_HANDLE hFile, UINT16 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_LSBFIRST
    *pBuffer = NCSByteSwap16(*pBuffer);
#endif
    return(eError);
}

NCSError NCSFileReadUINT16_LSB(NCS_FILE_HANDLE hFile, UINT16 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_MSBFIRST
    *pBuffer = NCSByteSwap16(*pBuffer);
#endif
    return(eError);
}

NCSError NCSFileReadUINT32_MSB(NCS_FILE_HANDLE hFile, UINT *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_LSBFIRST
    *pBuffer = NCSByteSwap32(*pBuffer);
#endif
    return(eError);
}

NCSError NCSFileReadUINT32_LSB(NCS_FILE_HANDLE hFile, UINT *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_MSBFIRST
    *pBuffer = NCSByteSwap32(*pBuffer);
#endif
    return(eError);
}

NCSError NCSFileReadUINT64_MSB(NCS_FILE_HANDLE hFile, UINT64 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_LSBFIRST
    *pBuffer = NCSByteSwap64(*pBuffer);
#endif
    return(eError);
}

NCSError NCSFileReadUINT64_LSB(NCS_FILE_HANDLE hFile, UINT64 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_MSBFIRST
    *pBuffer = NCSByteSwap64(*pBuffer);
#endif
    return(eError);
}

NCSError NCSFileReadIEEE4_MSB(NCS_FILE_HANDLE hFile, IEEE4 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_LSBFIRST
    NCSByteSwapRange32((UINT*)pBuffer, (UINT*)pBuffer, 1);
#endif
    return(eError);
}

NCSError NCSFileReadIEEE4_LSB(NCS_FILE_HANDLE hFile, IEEE4 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_MSBFIRST
    NCSByteSwapRange32((UINT*)pBuffer, (UINT*)pBuffer, 1);
#endif
    return(eError);
}

NCSError NCSFileReadIEEE8_MSB(NCS_FILE_HANDLE hFile, IEEE8 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_LSBFIRST
    NCSByteSwapRange64((UINT64*)pBuffer, (UINT64*)pBuffer, 1);
#endif
    return(eError);
}

NCSError NCSFileReadIEEE8_LSB(NCS_FILE_HANDLE hFile, IEEE8 *pBuffer)
{
    UINT nSize = sizeof(*pBuffer);
    NCSError eError = NCSFileRead(hFile, pBuffer, nSize, &nSize);
#ifdef NCSBO_MSBFIRST
    NCSByteSwapRange64((UINT64*)pBuffer, (UINT64*)pBuffer, 1);
#endif
    return(eError);
}
