#include "NCSECW.h"

NCSEcwInfo *pNCSEcwInfo = NULL;

/*******************************************************
**	NCSecwInitInternal/ShutdownInternal() - Inits/shutsdown the NCSecw library
**	WARNING! ONLY CALL THESE if you are not linking to the DLL,
**	but are including the code directly inside your application
********************************************************/

void NCSecwInitInternal()
{
    if( !pNCSEcwInfo ) {
        pNCSEcwInfo = (NCSEcwInfo *) NCSMalloc(sizeof(NCSEcwInfo), TRUE);	/**[17]**/

        pNCSEcwInfo->pStatistics = (NCSecwStatistics *)NCSMalloc(sizeof(NCSecwStatistics), TRUE);
        pNCSEcwInfo->bEcwpReConnect = FALSE; //[21]
        pNCSEcwInfo->bJP2ICCManage = TRUE;	//[22]
        pNCSEcwInfo->nMaxJP2FileIOCache = 1024; //[22]
        pNCSEcwInfo->nMaxProgressiveViewSize = NCSECW_MAX_VIEW_SIZE_TO_CACHE; /**[25]**/
        pNCSEcwInfo->nForceFileReopen = FALSE;	// normally want to merge file opens for optimization
        pNCSEcwInfo->bForceLowMemCompress = FALSE;
        pNCSEcwInfo->tLastCachePurge = NCSGetTimeStampMs();
        pNCSEcwInfo->nAggressivePurge = 0;		// larger number means be more aggressive next purge
        pNCSEcwInfo->nMaximumOpen = NCSECW_MAX_UNUSED_CACHED_FILES;		/**[18]**/
        pNCSEcwInfo->nPurgeDelay = NCSECW_PURGE_DELAY_MS;					/**[19]**/
        pNCSEcwInfo->nFilePurgeDelay = NCSECW_FILE_PURGE_TIME_MS;			/**[19]**/
        pNCSEcwInfo->nMinFilePurgeDelay = NCSECW_FILE_MIN_PURGE_TIME_MS;	/**[19]**/
        pNCSEcwInfo->nMaxOffsetCache = NCSECW_MAX_OFFSET_CACHE;				/**[19]**/

        pNCSEcwInfo->pStatistics->nMaximumCacheSize = 0x1fffffff;//1024 * 1024 * 64;
        pNCSEcwInfo->pStatistics->nBlockingTime		= NCSECW_BLOCKING_TIME_MS;
        pNCSEcwInfo->pStatistics->nRefreshTime		= NCSECW_REFRESH_TIME_MS;

        NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nApplicationsOpen, 1);

        pNCSEcwInfo->bShutdown = FALSE;
    }
}

void NCSecwInit()
{
    NCSecwInitInternal();
}

void NCSecwShutdownInternal()
{
    if( pNCSEcwInfo ) {
        BOOLEAN bTrueShutdown;

        pNCSEcwInfo->bShutdown = TRUE;

        bTrueShutdown = pNCSEcwInfo->bShutdown;
        pNCSEcwInfo->bShutdown = TRUE;

        if( pNCSEcwInfo->pStatistics) {
            NCSEcwStatsDecrement(&pNCSEcwInfo->pStatistics->nApplicationsOpen, 1);
        }

        NCSFree(pNCSEcwInfo->pStatistics);

        NCSFree(pNCSEcwInfo);
        pNCSEcwInfo = NULL;
    }
}

/*
** This stub now exported to allow the same code to be linked both statically and
** against either the NCSEcw, NCSUtil, and NCScnet DLLs or the libecwj2 DLL.
** It is expected that applications linking statically will explicitly call
** NCSecwShutdown and NCSecwInit to destroy and initialise library resources. [23]
*/
void NCSecwShutdown()
{
    NCSecwShutdownInternal();
}

/*******************************************************
**	NCSecwOpenFile() - open an NCS pointer to an ECW file
**
**	Notes:
**	a)	If bReadOffsets, p_block_offsets etc will be valid
**	b)	if bReadMemImage, pHeaderMemImage will be valid
**	c)	Can only bReadOffsets and/or bReadMemImage if the szInputFilename
**		points to a local file. So if this is a REMOTE file, then these
**		values are forced to FALSE, regardless of what the caller wanted.
**	d)	If bReadOffsets, then a file handle will be left open for the file,
**		and the NCSecwReadBlock() function call can be used (on this local file)
**	e)	If the file is already open, your re-open must not ask for more that was
**		previously opened (so you can't open the file without asking for block
**		offsets, and then open it again, this time asking for block offsets).
********************************************************/

NCSError NCSecwOpenFile(
                    NCSFile **ppNCSFile,
                    char *szUrlPath,			// input file name or network path
                    BOOLEAN bReadOffsets,		// TRUE if the client wants the block Offset Table
                    BOOLEAN bReadMemImage)		// TRUE if the client wants a Memory Image of the Header
{
    NCSFile	*pNCSFile = NULL;
    UINT8	*pFileHeaderMemImage=NULL;
    UINT	nFileHeaderMemImageLen=0;
    char	*pProtocol, *pHost, *pFilename;
    pProtocol = (char *)NULL;
    pHost = (char *)NULL;
    pFilename = (char *)NULL;

    if (!pNCSEcwInfo) {
        NCSecwInitInternal();
    }

    if (!ppNCSFile)
        return NCS_INVALID_ARGUMENTS;
    *ppNCSFile = NULL;

    // File was not in cache, so have to load it
    NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nFilesCacheMisses, 1);

    pNCSFile = (NCSFile *) NCSMalloc(sizeof(NCSFile), FALSE);

    if( !pNCSFile ) {
        return(NCS_FILE_NO_MEMORY);
    }
    pNCSFile->szUrlPath = (char *)NCSMalloc( (UINT)strlen(szUrlPath) + 1, FALSE);
    if( !pNCSFile->szUrlPath ) {
        NCSFree(pNCSFile);
        return(NCS_FILE_NO_MEMORY);
    }
    strcpy(pNCSFile->szUrlPath, szUrlPath);

    // Init client specific information
    pNCSFile->pBlockCachePool = NULL;
    pNCSFile->pFirstCachedBlock = NULL;
    pNCSFile->pWorkingCachedBlock = NULL;
    pNCSFile->pLastReceivedCachedBlock = NULL;

    pNCSFile->nClientUID = 0;
    pNCSFile->nServerSequence = 0;
    pNCSFile->nClientSequence = 1;
    pNCSFile->pLevel0ZeroBlock = pNCSFile->pLevelnZeroBlock = NULL;
    pNCSFile->bSendInProgress = FALSE;
    pNCSFile->nRequestsXmitPending = 0;
    pNCSFile->nCancelsXmitPending = 0;
    pNCSFile->tLastSetViewTime = NCSGetTimeStampMs();

    // init list and file information
    pNCSFile->bValid = TRUE;		// file is currently valid (not changed on disk yet)

    pNCSFile->pNextNCSFile = pNCSFile->pPrevNCSFile = NULL;
    pNCSFile->nUsageCount = 1;

    pNCSFile->bIsConnected = TRUE;
    pNCSFile->bIsCorrupt = FALSE;
    pNCSFile->bFileIOError = FALSE;

    pFileHeaderMemImage = NULL;

    bReadMemImage = TRUE;

    pNCSFile->bReadOffsets = bReadOffsets;
    pNCSFile->bReadMemImage = bReadMemImage;
    pNCSFile->pNCSFileViewList = NULL;
    pNCSFile->pNCSCachedFileViewList = NULL;

    pNCSFile->pOffsetCache = NULL;
    pNCSFile->nOffsetCache = 0;

    pNCSFile->pTopQmf = erw_decompress_open( szUrlPath, pFileHeaderMemImage, bReadOffsets, bReadMemImage ); //[21]
    if( !pNCSFile->pTopQmf ) {
        if( pFileHeaderMemImage )
            NCSFree( pFileHeaderMemImage );
        NCSFree(pNCSFile->szUrlPath);
        NCSFree(pNCSFile);
        return(NCS_ECW_ERROR);
    }

    pNCSFile->nUnpackedBlockBandLength =
        pNCSFile->pTopQmf->x_block_size * pNCSFile->pTopQmf->y_block_size * sizeof(INT16);

    pNCSFile->pNCSCachePurge = (NCSFileCachePurge *) NCSMalloc(sizeof(NCSFileCachePurge) *
        pNCSFile->pTopQmf->nr_levels, FALSE);
    if(!pNCSFile->pNCSCachePurge) {
        return NCS_COULDNT_ALLOC_MEMORY;
    }

    // Point to File Info in the QMF structure
    pNCSFile->pFileInfo = pNCSFile->pTopQmf->pFileInfo;

    // Note the file is now open
    NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nFilesOpen, 1);

    *ppNCSFile = pNCSFile;
    return(NCS_SUCCESS);
}

/*******************************************************
**	NCSecwCloseFile() - close a NCS file handle to an ECW file
**
** Indicates finished with a NCS file handle. Will only close the file and
** free the memory structures if total usage for this file has dropped to zero
** and the file will be flushed from cache due to too many files open.
**	Returns zero if no error on the close.
********************************************************/

int	NCSecwCloseFile( NCSFile *pNCSFile)
{
    if (!pNCSEcwInfo) {
        NCSecwInitInternal();
    }

    if( pNCSFile ) {
        pNCSFile->nUsageCount -= 1;
        NCSecwCloseFileCompletely(pNCSFile);
    }
    return(0);
}

/*
**	Flush file from cache. Does NOT mutex - caller must do the mutex
*/

int	NCSecwCloseFileCompletely( NCSFile *pNCSFile)
{

    // Remove any outstanding file views. Can't call the CloseFileView,
    // as that calls this routine (recursive error)
    while(pNCSFile->pNCSFileViewList) {
        NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nFileViewsOpen, 1);
        NCScbmCloseFileViewCompletely(&(pNCSFile->pNCSFileViewList), pNCSFile->pNCSFileViewList);
    }

    // Remove this file from the File List
    if( pNCSFile->pNextNCSFile )
        pNCSFile->pNextNCSFile->pPrevNCSFile = pNCSFile->pPrevNCSFile;
    if( pNCSFile->pPrevNCSFile )
        pNCSFile->pPrevNCSFile->pNextNCSFile = pNCSFile->pNextNCSFile;
    erw_decompress_close(pNCSFile->pTopQmf);
    NCSFree(pNCSFile->szUrlPath);
    if( pNCSFile->pLevel0ZeroBlock )
        NCSFree( pNCSFile->pLevel0ZeroBlock );
    if( pNCSFile->pLevelnZeroBlock )
        NCSFree( pNCSFile->pLevelnZeroBlock );

    // Free the cached blocks
    {
        NCSFileCachedBlock	*pNextCachedBlock = pNCSFile->pFirstCachedBlock;
        while( pNextCachedBlock ) {
            if( pNextCachedBlock->pPackedECWBlock ) {
                NCSFree(pNextCachedBlock->pPackedECWBlock);
                pNextCachedBlock->pPackedECWBlock = NULL;
                NCSEcwStatsDecrement(&pNCSEcwInfo->pStatistics->nPackedBlocksCacheSize, pNextCachedBlock->nPackedECWBlockLength);
            }
            if( pNextCachedBlock->pUnpackedECWBlock ) {
                NCSFree(pNextCachedBlock->pUnpackedECWBlock);
                pNextCachedBlock->pUnpackedECWBlock = NULL;
                NCSEcwStatsDecrement(&pNCSEcwInfo->pStatistics->nUnpackedBlocksCacheSize, pNextCachedBlock->nUnpackedECWBlockLength);
            }
            pNextCachedBlock = pNextCachedBlock->pNextCachedBlock;
        }
    }

    // Free cached block pointers pool, which will destroy the pool alloc'd pointers to cached blocks
    // As we do this after freeing the cached blocks, we can just block the entire list out
    // rather than freeing each entry.
    if( pNCSFile->pBlockCachePool )
        NCSPoolDestroy(pNCSFile->pBlockCachePool);


    // Free the cache purging structures
    if(pNCSFile->pNCSCachePurge)
        NCSFree(pNCSFile->pNCSCachePurge);

    NCSFree(pNCSFile->pOffsetCache);

    NCSFree(pNCSFile);

    return(0);
}

void NCSEcwStatsIncrement(NCSEcwStatsType *pVal, INT n)
{
    *pVal += n;
}

void NCSEcwStatsDecrement(NCSEcwStatsType *pVal, INT n)
{
    *pVal -= n;
}

void NCSFreeFileInfoEx(NCSFileViewFileInfoEx *pDst)
{
    if (pDst->szDatum != NULL) NCSFree(pDst->szDatum);
    if (pDst->szProjection != NULL) NCSFree(pDst->szProjection);
    if(pDst->pBands) {
        NCSFree(pDst->pBands);
    }
    memset(pDst, 0, sizeof(NCSFileViewFileInfoEx));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// collapse_pyr.c
//

static int qdecode_qmf_line_sidebands(QmfRegionLevelStruct	*p_level );
static int qdecode_qmf_line_a_sideband( QmfRegionLevelStruct *p_level, INT *p_sideband );
static int qdecode_qmf_line_blocks(QmfRegionLevelStruct	*p_level);

/*
**
**	[see erw_decompress_start_region() header for more information]
**
**	qdecode_qmf_level_line() -- collapse (reconstruct) a single line from a QMF-style pyramid
**	using a 3-tap (1-2-1) filter. This means we need at any time two input lines in memory
**	(recursively for each level) for any one output line.
**
**	For every two calls to this routine, one line of input LL and other sideband data
**	is generated by recursive calls to smaller levels. This is how expansion of
**	data is carried out.  This routine uses a 3 tap decompression, so we only need 3 lines
**	of subbands present at any time to preform the recursive reconstruction.
**
**	The data flow is complicated by the blocking of compressed data on disk. We have to retrieve
**	the set of blocks required in the X direction to expansion of a line.  However, we DO NOT decompress
**	the all lines for the set of blocks Y blocks, instead we decompress one line at a time.
**	The reasons for this require a bit of explanation:
**
**	1) We don't want to have all lines of a decompressed set of blocks in memory at the same time.
**	This is for memory usage reasons; if (for example) the Y direction blocking factor is 64MB,
**	and we have a TB file with 1,000,000 data lines in the X direction, and we want to decompress
**	the entire line (for example if they are decompressing the entire file for some reason such
**	as merging and recompressing it with other data), we would need 64 * 3 * 500,000 * 32 bits of RAM
**	e.g. 384MB of RAM to hold one line, at 2x that for all levels, e.g. over 600MB of RAM. This is
**	not desirable.
**
**	2) Also, we want to work with the CPU L1 and L2 cache systems as much as possible.  So what we
**	really want to do is decompress a line, use it immediately, and then discard it. This happens
**	to be the way the recursive logic works - it only wants 3 lines in memory during recursive
**	decompression, so for even very large files the 3 lines are likely to fit in CPU L1 and/or L2 cache
**	(and only two lines are used in sequence for any particular subband),
**	resulting in a very significant performance boost as everything can run out of cache.
**
**	3) By keeping blocks compressed, we can potentially cache them (or let the OS do the caching),
**	further boosting performance.
**
**	4) By keeping issues like blocking away from the core recursive QMF rebuilt routines,
**	we reuse the routine with minimal changes for other applications, such as streaming
**	data via transmission from the compression logic (which can also directly compress line by line)
**	straight into the decompression routine.
**
**
**	PROCESSING LOGIC:
**
**	Because of this, our logic is as follows:
**
**	for( number of lines to extract ) {
**		request_line()
**	}
**	
**
**	request_line() is actually the top level call into qdecode_qmf_level_line(). That routine
**	in turn recursivelly requests smaller QMF level lines to return LL lines.  The recursive
**	call, once it gets the LL data, then calls the decompression to decompress a single line
**	of the compressed block data (thus keeping blocking well away from the decompression routine),
**	expanding each smaller level line into two lines of this level, and so on up the recursive levels.
**
**	Recall that as this uses a 3 tap filter, two subband lines must be kept in memory at one
**	time.  We store these in a ring buffer, rolling the ring buffer points down by one each
**	time we want to decompress another line of input subbands from input.
** 
**	The only final point to note is that line [-1] is a reflection of line [0], and line
**	[y_size] is a reflection of line [y_size - 1], which is done as a test for these lines,
**	and using the reflected line when we have to.
**
**
** Data expansion is expanded by a simple filter as follows for a given [x,y]:
**
** First set baseline LL values:
**	O[0,0]  =  LL[0,0];									O[1,0]	= (LL[0,0]+LL[1,0])/2;
**	O[0,1]  = (LL[0,0]+LL[0,1])/2;						O[1,1]	= (LL[0,0]+LL[1,0]+LL[0,1]+LL[1,1])/4;
**
** Add in LH values. These are added to the above. Input is offset -1 in the Y axis
**	O[0,0] -= (LH[0,-1]+LH[0,0])/2;						O[1,0] -= (LH[0,-1]+LH[1,-1]+LH[0,0]+LH[1,0])/4;
**	O[0,1] +=  LH[0,0];									O[1,1] += (LH[0,0]+LH[1,0])/2;
**
** Add in HL values. These are added to the above. Input is offset -1 in the X axis
**	O[0,0] -= (HL[-1,0]+HL[0,0])/2;						O[1,0] +=  HL[0,0];
**	O[0,1] -= (HL[-1,0]+HL[0,0]+HL[-1,1]+HL[0,1])/4;	O[1,1] += (HL[0,0]+HL[0,1])/2;
**
** Add in HH values. These are added to the above. Input is offset -1 in the X axis and -1 in the Y axis
**	O[0,0] += (HH[-1,-1]+HH[0,-1]+HH[-1,0]+HH[0,0])/4;	O[1,0] -= (HH[0,-1]+HH[0,0])/2;
**	O[0,1] -= (HH[-1,0]+HH[0,0])/2;						O[1,1] +=  HH[0,0];
**
** When when considered relative to the start of the 2 input values for output, converts to:
**
**	O[0,0]  =  LL[1,1];									O[1,0]	= (LL[0,1]+LL[1,1])/2;
**	O[0,0] -= (LH[1,0]+LH[1,1])/2;						O[1,0] -= (LH[0,0]+LH[1,0]+LH[0,1]+LH[1,1])/4;
**	O[0,0] -= (HL[0,1]+HL[1,1])/2;						O[1,0] +=  HL[0,1];
**	O[0,0] += (HH[0,0]+HH[1,0]+HH[0,1]+HH[1,1])/4;		O[1,0] -= (HH[0,0]+HH[0,1])/2;
**
**	O[0,1]  = (LL[1,0]+LL[1,1])/2;						O[1,1]	= (LL[0,0]+LL[1,0]+LL[0,1]+LL[1,1])/4;
**	O[0,1] +=  LH[1,0];									O[1,1] += (LH[0,0]+LH[1,0])/2;
**	O[0,1] -= (HL[0,0]+HL[1,0]+HL[0,1]+HL[1,1])/4;		O[1,1] += (HL[0,0]+HL[0,1])/2;
**	O[0,1] -= (HH[0,0]+HH[1,0])/2;						O[1,1] +=  HH[0,0];
**
** Important things to note:
**	1.	Every 2nd line of output will only use one line of input for a given sideband,
**		enabling certain optimizations to be carried out.  Ditto for X direction.
**	2.	We roll input lines through, so line0 = line1; line1 = <new sideband line>
**	3.	For X<1 or Y<1 on output, we have negative input locations. These get reflected,
**		for example input value [-1,0] becomes [1,0] and [-1,-1] becomes [1,1]
**	4.	We always request one more X pixel to the left and one more X pixel to the right
**		of the line actually needed (so the buffer can potentially be 2 values larger than
**		the actual width of data); these are used for reflection handling on the edges (if really
**		at the edge of data), or for gathering left/right data as required.
**
**	A single output line is generated by a combination of two input lines.
**
*/

int qdecode_qmf_level_line( QmfRegionStruct *p_region, UINT16 level, UINT y_line,
                           IEEE4 **p_p_output_line)
{
    IEEE4	*p_temp;
    UINT	band;
    int	sideband;
    QmfRegionLevelStruct	*p_level;

    p_level = &(p_region->p_levels[level]);

    /*
    **	First, recurse up the levels to pre-read input LL lines needed to construct this level
    **	Then gather the LH, HL and HH lines needed. We need these every 2nd output line. We
    **	pre-read 2 input LL,LH,HL and HL lines at the start (p_level_read_lines = 1)
    */
    while(p_level->read_lines) {
        // roll line 0 to line -1, line1 to line 0
        for( band = 0; band < p_level->used_bands; band++ ) {
            for( sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++ ) {
                p_temp = p_level->p_p_line0[DECOMP_INDEX];
                p_level->p_p_line0[DECOMP_INDEX]	= p_level->p_p_line1[DECOMP_INDEX];
                p_level->p_p_line1[DECOMP_INDEX]	= p_temp;
            }
            // set up the new LL pointer, with X reflection, for recursive call
            p_level->p_p_line1_ll_sideband[band] =
                p_level->p_p_line1[band * MAX_SIDEBAND + LL_SIDEBAND] + p_level->reflect_start_x;
        }

        // recursively get LL from smaller level. note that we have to offset by the left
        // reflection value
        if( level )
            if( qdecode_qmf_level_line( p_region, (UINT16) (level - 1),
                            p_level->current_line, p_level->p_p_line1_ll_sideband) )
                return(1);

        if( p_level->current_line >= p_level->p_qmf->y_size ) {
            // Because of potentially 2 line pre-read to fill input, we have to reflect Y lines
            // for the last call.
            for( band = 0; band < p_level->used_bands; band++ ) {
                for( sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++ ) {
                    memcpy(p_level->p_p_line1[DECOMP_INDEX],
                           p_level->p_p_line0[DECOMP_INDEX],
                           (p_level->level_size_x + p_level->reflect_start_x + p_level->reflect_end_x)
                                * sizeof(p_level->p_p_line1[0][0]));
                }
            }
        }
        else {
            // or retrieve one line of the LH, HL and HH sidebands (and LL if this is level 0) from storage
            if( qdecode_qmf_line_sidebands(p_level) )
                return(1);
            // have to reflect line 1 to line 0 ONLY if line 0 read AND reflection is needed
            if( p_level->reflect_start_y && p_level->current_line == 0 ) {
                for( band = 0; band < p_level->used_bands; band++ ) {
                    for( sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++ ) {
                        memcpy(p_level->p_p_line0[DECOMP_INDEX],
                               p_level->p_p_line1[DECOMP_INDEX],
                              (p_level->level_size_x + p_level->reflect_start_x + p_level->reflect_end_x)
                                * sizeof(p_level->p_p_line1[0][0]));
                    }
                }
            }
        }


        // sort out reflection on start and end of line for all sidebands
        if( p_level->reflect_start_x ) {
            for( band = 0; band < p_level->used_bands; band++ )
                for( sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++ )
                    p_level->p_p_line1[DECOMP_INDEX][0] = p_level->p_p_line1[DECOMP_INDEX][1];
        }
        // end reflection must consider there may or may not be a start reflection offset
        if( p_level->reflect_end_x ) {
            for( band = 0; band < p_level->used_bands; band++ )
                for( sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++ )
                    p_level->p_p_line1[DECOMP_INDEX][p_level->level_size_x + p_level->reflect_start_x] 
                            = p_level->p_p_line1[DECOMP_INDEX][p_level->level_size_x + p_level->reflect_start_x - 1];
        }

        p_level->read_lines -= 1;
        p_level->current_line += 1;
    }

    /*
    **	Now convolve the line.  It depends on if it is an odd or even output line.
    **	Each output line uses two input lines, offset.
    **	The lines, and the X values, are always set up so 0 contains the first
    **	line & X needed, and 1 contains the second line & X needed. So if
    **	output needs input I[-1] and I[0], these are set up as 0 and 1.
    */
    for( band = 0; band < p_level->used_bands; band++ ) {
        register	UINT	x2, xoddeven;
        register	IEEE4	*p_output;

        p_output = p_p_output_line[band];
        x2		 = p_level->output_level_size_x;
        xoddeven = p_level->output_level_start_x;

/*
**
**	EVEN LINE VALUES	(0,2,4,...)
**
**	O[0,0]  =  LL[1,1];									O[1,0]	= (LL[0,1]+LL[1,1])/2;
**	O[0,0] -= (LH[1,0]+LH[1,1])/2;						O[1,0] -= (LH[0,0]+LH[1,0]+LH[0,1]+LH[1,1])/4;
**	O[0,0] -= (HL[0,1]+HL[1,1])/2;						O[1,0] +=  HL[0,1];
**	O[0,0] += (HH[0,0]+HH[1,0]+HH[0,1]+HH[1,1])/4;		O[1,0] -= (HH[0,0]+HH[0,1])/2;
**
*/
        if( (y_line & 0x0001) == 0 ) {		// Y output 0,2,4,...
            register	IEEE4	*p_ll_line1;
            register	IEEE4	*p_lh_line0, *p_lh_line1;
            register	IEEE4	*p_hl_line1;
            register	IEEE4	*p_hh_line0, *p_hh_line1;

            p_ll_line1	= p_level->p_p_line1[band * MAX_SIDEBAND + LL_SIDEBAND];
            p_lh_line0	= p_level->p_p_line0[band * MAX_SIDEBAND + LH_SIDEBAND];
            p_lh_line1  = p_level->p_p_line1[band * MAX_SIDEBAND + LH_SIDEBAND];
            p_hl_line1	= p_level->p_p_line1[band * MAX_SIDEBAND + HL_SIDEBAND];
            p_hh_line0	= p_level->p_p_line0[band * MAX_SIDEBAND + HH_SIDEBAND];
            p_hh_line1	= p_level->p_p_line1[band * MAX_SIDEBAND + HH_SIDEBAND];

            while(x2) {
                if( (xoddeven & 0x0001) == 0 ) {	// X output 0,2,4,...
                    *p_output++ =   p_ll_line1[1]
                                - ((p_lh_line0[1] + p_lh_line1[1] + p_hl_line1[0] + p_hl_line1[1]) * (IEEE4)0.5)
                                + ((p_hh_line0[0] + p_hh_line0[1] + p_hh_line1[0]+ p_hh_line1[1]) * (IEEE4)0.25);
                    p_ll_line1++;
                    p_lh_line0++;	
                    p_lh_line1++;
                    p_hl_line1++;
                    p_hh_line0++;	
                    p_hh_line1++;
                }
                else {								// X output 1,3,5,...
                    *p_output++ = (((p_ll_line1[0] + p_ll_line1[1]) - (p_hh_line0[0] + p_hh_line1[0])) * (IEEE4)0.5)
                                - ((p_lh_line0[0] + p_lh_line0[1] + p_lh_line1[0]+ p_lh_line1[1]) * (IEEE4)0.25)
                                +   p_hl_line1[0];
                }
                x2--;
                xoddeven++;
            }
        }
/*
**	ODD LINE VALUES (1,3,5,...)
**
**	O[0,1]  = (LL[1,0]+LL[1,1])/2;						O[1,1]	= (LL[0,0]+LL[1,0]+LL[0,1]+LL[1,1])/4;
**	O[0,1] +=  LH[1,0];									O[1,1] += (LH[0,0]+LH[1,0])/2;
**	O[0,1] -= (HL[0,0]+HL[1,0]+HL[0,1]+HL[1,1])/4;		O[1,1] += (HL[0,0]+HL[0,1])/2;
**	O[0,1] -= (HH[0,0]+HH[1,0])/2;						O[1,1] +=  HH[0,0];
*/
        else {								// Y output 1,3,5,...
            register	IEEE4	*p_ll_line0, *p_ll_line1;
            register	IEEE4	*p_lh_line0;
            register	IEEE4	*p_hl_line0, *p_hl_line1;
            register	IEEE4	*p_hh_line0;

            p_ll_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + LL_SIDEBAND];
            p_ll_line1 = p_level->p_p_line1[band * MAX_SIDEBAND + LL_SIDEBAND];
            p_lh_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + LH_SIDEBAND];
            p_hl_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + HL_SIDEBAND];
            p_hl_line1 = p_level->p_p_line1[band * MAX_SIDEBAND + HL_SIDEBAND];
            p_hh_line0 = p_level->p_p_line0[band * MAX_SIDEBAND + HH_SIDEBAND];

            while(x2) {
                if( (xoddeven & 0x0001) == 0 ) {	// X output 0,2,4,...
#ifdef DO_SSE
                    _m128 t = _mm_set_p(p_ll_line0[1], p_ll_line1[1], p_hh_line0[0], p_hh_line0[1]);

#else
                    *p_output++ = (((p_ll_line0[1] + p_ll_line1[1]) - (p_hh_line0[0] + p_hh_line0[1])) * (IEEE4)0.5)
                                +   p_lh_line0[1]
                                - ((p_hl_line0[0] + p_hl_line0[1] + p_hl_line1[0] + p_hl_line1[1]) * (IEEE4)0.25);
#endif
                    p_ll_line0++;	
                    p_ll_line1++;
                    p_lh_line0++;
                    p_hl_line0++;	
                    p_hl_line1++;
                    p_hh_line0++;
                }
                else {								// X output 1,3,5,...
                    *p_output++ = ((p_ll_line0[0] + p_ll_line0[1] + p_ll_line1[0] + p_ll_line1[1]) * (IEEE4)0.25)
                                + ((p_lh_line0[0] + p_lh_line0[1] + p_hl_line0[0] + p_hl_line1[0]) * (IEEE4)0.5)
                                +   p_hh_line0[0];
                }
                x2--;
                xoddeven++;
            }
        }
    }

    // If the next line is odd, roll the input lines and read another input on next call
    if( (y_line & 0x0001) == 0 ) // Y output 0,2,4,...
        p_level->read_lines = 1;

    return(0);
}


/*
**	Retrieves the input sidebands for one line (p_level->current_line) of this level.
**	If level 0, this includes LL,
**	otherwise includes just the LH,HL and HH sidebands.  If p_level->have_block is TRUE,
**	then we have already started decoding the X set of blocks that includes this line, and 
**	we can just uncompress the appropriate line.
**	If have_block is FALSE, we have to first request the set of blocks covering in the X direction
**	that cover this block, prior to decoding the line. Note that all X blocks
**	for the current line are retrieved, but are held in a compressed state; only one line
**	at a time is decompressed.
**
**	Also: We handle X reflection or side data gathering, by adding in reflect_start_x offset
*/
static int qdecode_qmf_line_sidebands(QmfRegionLevelStruct	*p_level)
{	

    // if we are at the end of this set of blocks (or none read yet), get the next set of blocks
    // in the X direction that cover this line.
    if( !p_level->have_blocks )
        if( qdecode_qmf_line_blocks(p_level) ) {
            return(1);
        }
    // now decompress this single line into line1 (which will roll
    // down after use)
    if( unpack_line( p_level) )
        return(1);

    p_level->next_input_block_y_line += 1;
    // see if we have run to the end of this block
    if(( p_level->next_input_block_y_line >= p_level->p_qmf->y_block_size ) ||
       ( p_level->current_line - p_level->start_line >= p_level->level_size_y - 1)) {
        unpack_finish_lines(p_level);
        p_level->have_blocks = FALSE;
    }
    return(0);
}


/*
**	Loads the set of blocks in the X direction that p_level->current_line falls into.
**	We have to do a number of things here:
**	1)	set up pointers to the array of X blocks that have been loaded
**	2)  pre-read and throw away any number of lines that are before p_level->current_line
**		(this can happen when we start reading at a line greater than the block level;
**		it will only happen once in the entire recursive reconstruction)
**	3)	Correctly set the p_level->next_input_block_y_line.
**	4)	Set the have_block value
*/
static int qdecode_qmf_line_blocks(QmfRegionLevelStruct	*p_level)
{
    UINT	next_y_block;
    UINT	next_y_block_line;
    UINT	x_block_count;
    UINT	next_x_block;

    next_y_block = p_level->current_line / p_level->p_qmf->y_block_size;

    next_y_block_line = p_level->current_line - (next_y_block * p_level->p_qmf->y_block_size);
    p_level->next_input_block_y_line = (INT16)next_y_block_line;

    /*
    **	Read the block, and set up decompression ready to decode line by line
    */
    for(next_x_block = p_level->start_x_block, x_block_count = 0;
                x_block_count < p_level->x_block_count;
                next_x_block++, x_block_count++ ) {
        UINT8	*p_block;

        p_block = NCScbmReadViewBlock(p_level, next_x_block, next_y_block );	// [02]
        if( !p_block )
            return(1);
        // set up unpacking for line by line. Also skip unwanted lines at the start
        // NOTE! The unpack_start_line_block routine will NOT free the p_block
        // if there is an error

        {
            //__try {
                if( unpack_start_line_block(p_level, x_block_count, p_block, next_y_block_line) ) {
                    NCScbmFreeViewBlock(p_level, p_block );
                    return(1);
                }
            /*} __except (EXCEPTION_EXECUTE_HANDLER) {
                if( p_level->p_qmf->level )
                    p_block = p_level->p_region->pNCSFileView->pNCSFile->pLevelnZeroBlock;
                else
                    p_block = p_level->p_region->pNCSFileView->pNCSFile->pLevel0ZeroBlock;

                if(!p_level->p_region->pNCSFileView->pNCSFile->bIsCorrupt) {
                    p_level->p_region->pNCSFileView->pNCSFile->bIsCorrupt = TRUE;
                }
            }*/
        }
    }

    p_level->have_blocks = TRUE;
    return(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// ecw_open.c
//

/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	
**
**
**	erw_decompress_open() at al.  Call with the name of the file to open.
**	Returns a pointer to the ERW structure, or NULL if an error occurred.
**
**	Input filename can be either the ERS or the ERW file - it does not matter
**
**	This routine is called once to open an ERW compressed file
**	to read it.  Once opened using erw_decompress_open(), it can be read multiple times
**	at different resolutions or geographic resolutions using erw_decompress_start_region().
**	So you can make multiple region calls (at the same time or sequentially) to a file
**	that has been opened.
**
**	The sequence of usage is:
**
**	erw_decompress_open()				- open a file
**	erw_decompress_start_region()		- define a region to be read at specified resolution
**	erw_decompress_read_region_line()	- read the next line from the region
**	erw_decompress_end_region()			- end reading a region.  Call at end of region, or when aborting read
**	erw_decompress_close()				- close a file
**
**
**	For maximum performance, you should minimize the number of
**	erw_decompress_open() calls. It is faster to have a single open then multiple
**	region calls for that single open.
**	[later note: files are now cached, so multiple open's don't have as much
**	overhead any more]
**
**	You can have multiple open_region() calls open for a single file, or open the file multiple
**	times. Either technique works and are just as efficient.
**
**	If you are aborting a read of a region, but plan to read another region from the
**	same file, just call erw_decompress_end_region(). If you are aborting all access
**	to the file, call erw_decompress_end_region() and then erw_decompress_close().
**
**	WARNING: do not close the file with one or more start_region() calls still open.
**	This will corrupt memory.  You should clean up all outstanding close_region()
**	calls before calling the shutdown stage.
**
*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/


/*
**	Open a QMF file for multiple region reads. File details can be
**	from a file, or directly from a memory image of the header
*/

QmfLevelStruct *erw_decompress_open( char *p_input_filename,
            UINT8	*pMemImage,			// if non-NULL, open the memory image not the file
            BOOLEAN bReadOffsets,
            BOOLEAN bReadMemImage )
{
    Byte	the_byte;
    UINT8	temp_byte;
    UINT16	temp_int16;
    UINT	temp_int;
    int		error, b;

    ECWFILE	hEcwFile = NULL_ECWFILE;		// [03]
    BOOLEAN	bEcwFileOpen = FALSE;	// [03]

    UINT16	num_levels = 0;
    UINT16  level = 0;
    UINT	x_size = 0;
    UINT  y_size = 0;
    FLOAT scale_factor = 1.0f;
    UINT16	x_block_size = 0;
    UINT16  y_block_size = 0;
    UINT8	qmf_version = 0;
    UINT8	qmf_blocking_format = BLOCKING_LEVEL;
    UINT8	qmf_compress_format = COMPRESS_NONE;
    UINT8	qmf_nr_sidebands = 0;
    UINT16	qmf_nr_bands = 0;
    QmfLevelStruct	*p_top_qmf, *p_smaller_qmf, *p_qmf, *p_file_qmf;
    UINT8	*pIntoMemImage;	// pointer into current part of MemImage being read

    // VERSION 2.0 INFORMATION
    UINT16	nCompressionRate = 1;
    UINT8	eCellSizeUnits = ECW_CELL_UNITS_METERS;
    IEEE8	fCellIncrementX = 1.0;
    IEEE8	fCellIncrementY = 1.0;
    IEEE8	fOriginX		  = 0.0;
    IEEE8	fOriginY		  = 0.0;
    char	szDatum[ECW_MAX_DATUM_LEN]			  = "RAW";	// start with a default Datum
    char	szProjection[ECW_MAX_PROJECTION_LEN]  = "RAW";	// start with a default Projection

    // If reading from memory, set the pIntoMemImage pointer
    if( pMemImage ) {
        pIntoMemImage = pMemImage;
        bReadOffsets = FALSE;
        bReadMemImage = FALSE;
    }
    else
        pIntoMemImage = NULL;

    /*
    **	Read the ECW file, and set up values.
    **	IMPORTANT!! there are two different set ups here, and in the QMF levels further down.
    **	If you change the header, you must change BOTH the memory and the file reader version.
    **	Look for all locations marked [HEADER] and update them as required
    */
    if( pIntoMemImage ) {
        // [HEADER] read main header from memory image
        error = 0;
        temp_byte = *pIntoMemImage++;
        if (temp_byte != (the_byte = ECW_HEADER_ID_TAG)) { 
            error = 1;
        }
        else {
            qmf_version = *pIntoMemImage++;
            temp_byte = *pIntoMemImage++;				qmf_blocking_format = temp_byte;
            temp_byte = *pIntoMemImage++;				qmf_compress_format = temp_byte;
            temp_byte = *pIntoMemImage++;				num_levels = (UINT16) temp_byte;
            temp_byte = *pIntoMemImage++;				qmf_nr_sidebands = temp_byte;
            temp_int = sread_int32(pIntoMemImage);
            pIntoMemImage += 4;							x_size = (int) temp_int;
            temp_int = sread_int32(pIntoMemImage);
            pIntoMemImage += 4;							y_size = (int) temp_int;
            temp_int16 = sread_int16(pIntoMemImage);
            pIntoMemImage += 2;							qmf_nr_bands = temp_int16;
            temp_int16 = sread_int16(pIntoMemImage);
            pIntoMemImage += 2;							scale_factor = (IEEE4) temp_int16;
            x_block_size = sread_int16(pIntoMemImage);	pIntoMemImage += 2;
            y_block_size = sread_int16(pIntoMemImage);	pIntoMemImage += 2;
            if( ERSWAVE_VERSION < qmf_version ) {
                error = 1;
            }

            // some more sanity checking
            if( num_levels == 0 || qmf_nr_bands == 0 || scale_factor == (IEEE4) 0.0 ) {
                error = 1;
            }


            // VERSION 2.0 INFORMATION
            if( qmf_version > 1 ) {
                temp_int16 = sread_int16(pIntoMemImage);
                pIntoMemImage += 2;							nCompressionRate = temp_int16;
                temp_byte  = *pIntoMemImage++;				eCellSizeUnits	 = temp_byte;
                sread_ieee8( &fCellIncrementX, pIntoMemImage);	pIntoMemImage += 8;
                sread_ieee8( &fCellIncrementY, pIntoMemImage);	pIntoMemImage += 8;
                sread_ieee8( &fOriginX,		 pIntoMemImage);	pIntoMemImage += 8;
                sread_ieee8( &fOriginY,		 pIntoMemImage);	pIntoMemImage += 8;

                strcpy(szDatum, (const char*)pIntoMemImage);			 pIntoMemImage += ECW_MAX_DATUM_LEN;
                strcpy(szProjection,(const char*)pIntoMemImage);		 pIntoMemImage += ECW_MAX_PROJECTION_LEN;	
            }
        }
        if( error )
            return(NULL);
    }
    else {
        // [HEADER] read main header from file
        if( EcwFileOpenForRead(p_input_filename, &hEcwFile) )
            bEcwFileOpen = FALSE;
        else
            bEcwFileOpen = TRUE;

        error = 0;
        if( bEcwFileOpen ) {
            EcwFileReadUint8(hEcwFile, &temp_byte);
            if (temp_byte != (the_byte = ECW_HEADER_ID_TAG)) { 
                error = 1;
            }
            else {
                EcwFileReadUint8(hEcwFile, &qmf_version);
                EcwFileReadUint8(hEcwFile, &temp_byte);				qmf_blocking_format = temp_byte;
                EcwFileReadUint8(hEcwFile, &temp_byte);				qmf_compress_format = temp_byte;
                EcwFileReadUint8(hEcwFile, &temp_byte);				num_levels = (UINT16) temp_byte;
                EcwFileReadUint8(hEcwFile, &temp_byte);				qmf_nr_sidebands = temp_byte;
                EcwFileReadUint32(hEcwFile, &temp_int);				x_size = (int) temp_int;
                EcwFileReadUint32(hEcwFile, &temp_int);				y_size = (int) temp_int;
                EcwFileReadUint16(hEcwFile, &temp_int16);			qmf_nr_bands = temp_int16;
                EcwFileReadUint16(hEcwFile, &temp_int16);			scale_factor = (IEEE4) temp_int16;
                EcwFileReadUint16(hEcwFile, &x_block_size);
                EcwFileReadUint16(hEcwFile, &y_block_size);
                if( ERSWAVE_VERSION < qmf_version ) {
                    error = 1;
                }

                // some more sanity checking
                if( num_levels == 0 || qmf_nr_bands == 0 || scale_factor == (IEEE4) 0.0 ) {
                    error = 1;
                }

                // VERSION 2.0 INFORMATION
                if( qmf_version > 1 ) {
                    EcwFileReadUint16(hEcwFile, &temp_int16);			nCompressionRate = temp_int16;
                    EcwFileReadUint8(hEcwFile, &temp_byte);				eCellSizeUnits = temp_byte;
                    EcwFileReadIeee8(hEcwFile,  &fCellIncrementX);
                    EcwFileReadIeee8(hEcwFile,  &fCellIncrementY);
                    EcwFileReadIeee8(hEcwFile,  &fOriginX);
                    EcwFileReadIeee8(hEcwFile,  &fOriginY);
                    EcwFileRead(hEcwFile, szDatum, ECW_MAX_DATUM_LEN);
                    EcwFileRead(hEcwFile, szProjection, ECW_MAX_PROJECTION_LEN);
                }
            }
        }
        else {
            error = 1;
        }
        if( !bEcwFileOpen || error ) {
            if( bEcwFileOpen )
                EcwFileClose(hEcwFile);
            return(NULL);
        }
    }

    /*
    **	The ERW file appears to be valid.
    **	Now create the fake file QMF level of the tree
    */
    p_file_qmf = new_qmf_level(x_block_size, y_block_size, num_levels, x_size, y_size, qmf_nr_bands, NULL, NULL);
    if( !p_file_qmf ) {
        if( bEcwFileOpen )
            EcwFileClose(hEcwFile);
        return(NULL);
    }
    p_file_qmf->version = qmf_version;
    p_file_qmf->blocking_format = (BlockingFormat)qmf_blocking_format;
    p_file_qmf->compress_format = (CompressFormat)qmf_compress_format;
    p_file_qmf->nr_sidebands = qmf_nr_sidebands;
    p_file_qmf->scale_factor = scale_factor;
    p_file_qmf->nr_levels = (UINT8) num_levels;		// [02]

    /*
    **	Now cycle through the levels of the ERW file, getting each level information
    */
    p_top_qmf = p_smaller_qmf = p_qmf = NULL;
    for(level = 0; level < num_levels; level++ ) {
        UINT8	qmf_level;
        UINT	qmf_x_size;
        UINT	qmf_y_size;
        UINT	band;

        /*
        **	Read level number, X size, Y size
        */
        if( pIntoMemImage ) {
            // [HEADER] read a QMF header from memory image
            qmf_level = *pIntoMemImage++;									// level number
            qmf_x_size = sread_int32(pIntoMemImage); pIntoMemImage += 4;	// level X size
            qmf_y_size = sread_int32(pIntoMemImage); pIntoMemImage += 4;	// level Y size
        }
        else {
            // [HEADER] read a QMF header from file
            if( (error = EcwFileReadUint8(hEcwFile, &qmf_level)) != 0 )				// level number
                break;
            if( (error = EcwFileReadUint32(hEcwFile, &qmf_x_size)) != 0 )				// level X size
                break;
            if( (error = EcwFileReadUint32(hEcwFile, &qmf_y_size)) != 0 )				// level Y size
                break;
        }


        error = 1;
        if( qmf_level != level
         || qmf_x_size >= x_size
         || qmf_y_size >= y_size ) {
            error = 1;			// Error: compressed file error. Unexpected level number
            break;
        }
        p_qmf = new_qmf_level( p_file_qmf->x_block_size,  p_file_qmf->y_block_size,
                               qmf_level, qmf_x_size, qmf_y_size, qmf_nr_bands, p_smaller_qmf, NULL);
        if( !p_qmf )
            break;
        if( !p_top_qmf )
            p_top_qmf = p_qmf;	/* first level, so ensure we point to it */

        p_qmf->p_top_qmf		= p_top_qmf;
        p_qmf->version			= p_file_qmf->version;
        p_qmf->blocking_format	= p_file_qmf->blocking_format;
        p_qmf->compress_format	= p_file_qmf->compress_format;
        p_qmf->nr_sidebands		= p_file_qmf->nr_sidebands;
        p_qmf->scale_factor		= p_file_qmf->scale_factor;
        p_qmf->nr_levels		= (UINT8) num_levels;	// [02]

        p_qmf->nr_x_blocks = QMF_LEVEL_NR_X_BLOCKS(p_qmf);
        p_qmf->nr_y_blocks = QMF_LEVEL_NR_Y_BLOCKS(p_qmf);

        p_qmf->p_file_qmf = p_file_qmf;
        if( p_smaller_qmf )
            p_smaller_qmf->p_larger_qmf = p_qmf;
        p_smaller_qmf = p_qmf;

        /*
        **	Read binsizes for this level, one value per band (e.g. 1 band = greyscale; 3 = RGB or YIQ)
        */
        if( pIntoMemImage ) {
            // [HEADER] read a QMF binsizes from memory image
            for( band = 0; band < qmf_nr_bands; band++ ) {
                p_qmf->p_band_bin_size[band] = sread_int32( pIntoMemImage );
                pIntoMemImage += 4;
            }
        }
        else {
            // [HEADER] read a QMF binsizes from file
            for( band = 0; band < qmf_nr_bands; band++ ) {
                if( (error = EcwFileReadUint32(hEcwFile, &p_qmf->p_band_bin_size[band])) != 0 )
                    break;
            }
        }
        error = 0;
    }

    if( error ) {
        if( bEcwFileOpen )
            EcwFileClose(hEcwFile);
        delete_qmf_levels(p_top_qmf);
        return(NULL);
    }
    /* [07] for v 1.0 files, calculate what target compression ratio was based on scale factor */
    if( qmf_version == 1 ) {
        UINT	nBinSize = p_qmf->p_band_bin_size[0];
        if( p_qmf->compress_format == COMPRESS_YUV )
            nBinSize = nBinSize / 2;		/* YUV has Y at 1/2 desired compression rate */
        if( p_qmf->scale_factor >= 1 )
            nCompressionRate = (UINT16)(p_qmf->p_band_bin_size[0] / (UINT) p_qmf->scale_factor);
    }

    /* Attach the file qmf to the end of the tree, currently pointed to by p_qmf */
    p_qmf->p_larger_qmf = p_file_qmf;
    p_file_qmf->p_smaller_qmf = p_qmf;
    p_file_qmf->p_top_qmf = p_top_qmf;

    /*
    **	[02] read file memory image if it is required
    */
    if( bReadMemImage) {
        UINT64	nOffsetPos;

        EcwFileGetPos(hEcwFile, &nOffsetPos );

        p_top_qmf->nHeaderMemImageLen = (UINT) nOffsetPos;
        p_top_qmf->pHeaderMemImage = (UINT8 *) NCSMalloc(p_top_qmf->nHeaderMemImageLen, FALSE);

        if(!p_top_qmf->pHeaderMemImage) {
            return NULL;
        }
        nOffsetPos = 0;
        EcwFileSetPos(hEcwFile, nOffsetPos);

        if( EcwFileRead(hEcwFile, p_top_qmf->pHeaderMemImage, p_top_qmf->nHeaderMemImageLen) ) {
            EcwFileClose(hEcwFile);
            delete_qmf_levels(p_top_qmf);
            return(NULL);
        }

        nOffsetPos = p_top_qmf->nHeaderMemImageLen;
        EcwFileSetPos(hEcwFile, nOffsetPos);
    }
    /*
    **	Unpack block offsets from the file. Once this is done, the first block will
    **	be the next location on disk, so we also record that position for later use.
    **	Note that all block offsets for all levels are stored, so we grab them all,
    **	and write them into the p_top_qmf pointer.  The allocation logic already
    **	pointed larger QMF sections their portion of this array.
    **
    */
    if( !pIntoMemImage ) {
        while( TRUE ) {			// we only loop once. This is handy so we can break on errors
            UINT8	*p_packed = (UINT8*)NULL;
            UINT8	*p_unpacked;
            UINT	unpacked_length;
            UINT	packed_length;
            UINT8 eFormat = (UINT8)ENCODE_RAW;

            if( (error = EcwFileReadUint32(hEcwFile, &packed_length )) != 0 )
                break;
            unpacked_length = get_qmf_tree_nr_blocks(p_top_qmf) * sizeof(p_top_qmf->p_block_offsets[0]);
            error = 1;
            if( packed_length > unpacked_length + 1 )		// sanity check before the malloc
                break;

            
            if( EcwFileRead(hEcwFile, &eFormat, 1) ) {
                error = TRUE;
                break;
            }
            if(eFormat == ENCODE_RAW) {
                UINT64 nPos = 0;
                p_top_qmf->bRawBlockTable = TRUE;
                p_top_qmf->p_block_offsets = (UINT64 *)NULL;

                EcwFileGetPos(hEcwFile, &nPos);
                error = EcwFileSetPos(hEcwFile, nPos + packed_length - 1);
            } else if(bReadOffsets) {
                p_packed = (UINT8 *) NCSMalloc(packed_length + sizeof(UINT64) - 1, FALSE);

                if( !p_packed )
                    break;

                memset(p_packed, 0, sizeof(UINT64) - 1);

                *(p_packed + sizeof(UINT64) - 1) = eFormat;

                if( EcwFileRead(hEcwFile, p_packed + sizeof(UINT64), packed_length - 1) ) {
                    error = TRUE;
                    if(p_packed)
                        NCSFree(p_packed);
                    break;
                }
                                                //[13]
                p_unpacked = NULL;	// [02]
                error = unpack_data(&p_unpacked, p_packed + sizeof(UINT64) - 1, unpacked_length, 1);
                
                if( p_packed )
                    NCSFree(p_packed);
                if( error )
                    break;
    
#ifdef NCSBO_MSBFIRST
                NCSByteSwapRange64((UINT64*)p_unpacked, (UINT64 *)p_unpacked, unpacked_length / 8);
#endif
                p_top_qmf->bRawBlockTable = FALSE;
                p_top_qmf->p_block_offsets = (UINT64 *)p_unpacked;
            } else {
                UINT64 nPos = 0;
                p_top_qmf->bRawBlockTable = FALSE;
                p_top_qmf->p_block_offsets = (UINT64 *)NULL;

                EcwFileGetPos(hEcwFile, &nPos);
                error = EcwFileSetPos(hEcwFile, nPos + packed_length - 1);
            }
            break;			// we only want to run this while once
        }
        EcwFileGetPos(hEcwFile, &(p_top_qmf->file_offset));
    }

    // Now set up File Info structure
    // must set this up so it is always valid for higher levels to rely on
    p_top_qmf->pFileInfo = (ECWFileInfoEx *) NCSMalloc(sizeof(ECWFileInfoEx), FALSE);
    if( !p_top_qmf->pFileInfo ) {
        if( bEcwFileOpen )
            EcwFileClose(hEcwFile);
        delete_qmf_levels(p_top_qmf);
        return(NULL);
    }

    p_top_qmf->pFileInfo->nSizeX = p_top_qmf->p_file_qmf->x_size;
    p_top_qmf->pFileInfo->nSizeY = p_top_qmf->p_file_qmf->y_size;
    p_top_qmf->pFileInfo->nBands = p_top_qmf->p_file_qmf->nr_bands;

    // FIXME!! Add in this information
    p_top_qmf->pFileInfo->nCompressionRate = nCompressionRate;
    p_top_qmf->pFileInfo->eCellSizeUnits = (CellSizeUnits)eCellSizeUnits;
    
    p_top_qmf->pFileInfo->fCellIncrementX = fCellIncrementX;
    p_top_qmf->pFileInfo->fCellIncrementY = fCellIncrementY;
    p_top_qmf->pFileInfo->fOriginX		  = fOriginX;
    p_top_qmf->pFileInfo->fOriginY		  = fOriginY;
    p_top_qmf->pFileInfo->szDatum		  = (char *)NCSMalloc(ECW_MAX_DATUM_LEN, FALSE);
    if(!p_top_qmf->pFileInfo->szDatum) {
        return NULL;
    }
    strcpy(p_top_qmf->pFileInfo->szDatum,szDatum);
    p_top_qmf->pFileInfo->szProjection	  = (char *)NCSMalloc(ECW_MAX_PROJECTION_LEN, FALSE);
    if(!p_top_qmf->pFileInfo->szProjection) {
        return NULL;
    }
    strcpy(p_top_qmf->pFileInfo->szProjection,szProjection);

    p_top_qmf->pFileInfo->pBands = (NCSFileBandInfo*)NCSMalloc(sizeof(NCSFileBandInfo) * p_top_qmf->pFileInfo->nBands, TRUE);
    if(p_top_qmf->compress_format == COMPRESS_YUV) {
        p_top_qmf->pFileInfo->eColorSpace =	NCSCS_sRGB;
    } else {
        p_top_qmf->pFileInfo->eColorSpace = (NCSFileColorSpace)p_top_qmf->compress_format;
    }
    for(b = 0; b < p_top_qmf->pFileInfo->nBands;  b++) {
        p_top_qmf->pFileInfo->pBands[b].nBits = 8;
        p_top_qmf->pFileInfo->pBands[b].bSigned = FALSE;

        switch(p_top_qmf->compress_format) {
            case COMPRESS_UINT8:
                    p_top_qmf->pFileInfo->pBands[b].szDesc = NCS_BANDDESC_Greyscale;
                break;
            case COMPRESS_MULTI:
                break;
            case COMPRESS_YUV:
                    switch(b) {
                        case 0:	
                                p_top_qmf->pFileInfo->pBands[b].szDesc = NCS_BANDDESC_Red;
                            break;
                        case 1:	
                                p_top_qmf->pFileInfo->pBands[b].szDesc = NCS_BANDDESC_Green;
                            break;
                        case 2:	
                                p_top_qmf->pFileInfo->pBands[b].szDesc = NCS_BANDDESC_Blue;
                            break;
                    }
                break;
        }
        if(!p_top_qmf->pFileInfo->pBands[b].szDesc) {
            p_top_qmf->pFileInfo->pBands[b].szDesc = NCS_BANDDESC_Band;
        }
    }
    p_top_qmf->pFileInfo->eCellType = NCSCT_UINT8;

    // Allocate decompression specific buffers, now we have read the offsets in
    if(allocate_qmf_buffers(p_top_qmf) != NCS_SUCCESS) {
        error = 1;
    }

    if( error ) {
        if( bEcwFileOpen )
            EcwFileClose(hEcwFile);
        delete_qmf_levels(p_top_qmf);
        return(NULL);
    }

    p_top_qmf->hEcwFile = hEcwFile;
    p_top_qmf->bEcwFileOpen = bEcwFileOpen;		// [03] point to the file
    return(p_top_qmf);
}

/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	
**
**	erw_decompress_start_region().  Initiate reading a region from a compressed file.
**	The caller specifies the region to read, and the resolution to read it at.
**
**	Call with a pointer to the ERW file to read from, and details about the region
**	to read.
**
**	Returns a pointer to a region structure, or NULL if an error occurred.
**
**	After starting a region, do multiple calls to erw_decompress_read_region_line()
**	to read the lines, then a call to erw_decompress_end_region() to finish reading.
**
**	IMPLEMENTATION:
**	1) Finding the largest level to use
**		The recursive level logic needs a bit of explanation.  First, we find the level
**		we need to provide the level of detail (LOD) that they require. This is simply
**		the level that, for that level's width from that level's start_[x|y] to that
**		level's end_[x|y] is no less than half the requested number_[x|y]. It is no
**		less than half, because a given level generates 2x the level values as output
**		during pyramid reconstruction.
**
**	2) Defining the boundaries of the smaller levels.
**		Once we know the level to generate output for, we then have to work out the start
**		and end x|y values for each smaller level. This is not quite as simple as you might
**		think, as a single output value is generated from two smaller level values.
**		Which input values are used depends on the output value location, specifically
**		if it is odd or even (this is because we use a 3 tap reconstruction filter;
**		see collapse_pyr.c for details).
**		
**		For a given output location O[N], in either X or Y direction, the two input locations needed
**		are I[(N-1)/2] and I[(N-1)/2]+1.  For example, X value O[0] needs I[-1] and I[0],
**		X value O[1] needs I[0] and I[1], X value O[66] needs I[63] and I[62], O[101] needs I[50] and I[51]
**		so on.
**
**		In a two dimensional view, output O[x,y] therefore requires 4 input values:
**		(1)	I[(x-1)/2,(y-1)/2]		(2)	I[(x-1)/2+1,(y-1)/2]	
**		(3)	I[(x-1)/2,(y-1)/2+1]	(4)	I[(x-1)/2+1,(y-1)/2+1]	
**
**	3) There is a difference between finding the level to display, and then levels under that level
**		Note that we use simple N/2 logic to find the level large enough to display (#1) above
**		before using the more complex (#2) above to calculate required smaller level sizes.
**
**	4) The smallest level may be larger than the level of detail required
**		In the normal situation, the level required is subsamples (expanded in detail) to
**		provide the output view. However, it may be that the detail needed is smaller than
**		that provided by the smallest level, in which case supersampling is carried out
**		(more values are decompressed than are actually handed back to the calling routine)
**
**	5) Input value rolling
**		Two inputs are needed to generate two outputs, and input is rolled once for every
**		2nd output generated. This results in output being 2x larger than input, which
**		is what we need.  The input is rolled after even output [x,y] values. This is
**		because of the ((n-1)/2) and ((n-1)/2)+1 input requirement. For example, consider:
**		output X or Y		Needs these 2 inputs
**			10				4 5
**			11				5 6
**			12				5 6
**			13				6 7
**		So it can be seen that *after* even X or Y values we roll the inputs.
**		The initial values set up are such that the first two I values are valid for the first
**		O value. The first roll may happen straight after processing that O value (if O is
**		even), or might happen after processing two O values (if the first O is odd).
**
**	6) Reflection handling
**		The above ignores one final problem: Edges.  Consider a level that is 94 X wide
**		(or 94 Y down) in size. To generate the O[93] value (as this is relative to 0, this
**		is value # 94), we need input values I[46] and I[47].  But input size is only 47 for
**		this smaller level (size level L-1 is level L wize (n+1/2)).  So the I[47] value
**		does not exist.  In the same way, value I[-1] does not exist, but is needed to generate
**		O[0], which requires I[-1] and I[0].
**	
**		In both of these cases, we add in REFLECTION handling.  There are 4 possible reflection
**		values, reflect_start_x, reflect_end_x, reflect_start_y, reflect_end_y. These will be
**		0 (no reflection) or 1 (reflection required on that side).
**
**		It is important to note that the level_[start|end|size]_[x|y] values DO NOT include
**		the reflection counts. So true size of a level, including reflected values,
**		is always (for example):
**			1 + reflect_start_x + reflect_end_x + level_end_x - level_start_x;
**		This means that when a level is asked to return data, it is returned to line + reflect_start,
**		and reflection is patched up after the data is retrieved.
**		One final note of interest:  Consider a level that has output_start_x as follows. This
**		is how the reflection will work for this and smaller levels:
**		output_start_x		level_start_x		relect_start_x
**			3					1					0		# because output 3 needs values 1 and 2
**			1					0					0		# because output 1 needs values 0 and 1
**			0					0					1		# because output 0 needs values 0 and 0
**		So the important point here is that reflection is based on the current level value, AND on
**		the output value required.
**
**	7) True size versus reflected size, and buffer offsets
**		Reflection means we have two start/end/size values for X and Y - with reflection, and
**		without reflection. The _reading_ of line0 and line1 needs to work with all data,
**		including any reflection.  The _writing_ to line0 and line1 needs to be excluding
**		reflection, which is added in after the data is written.  Thus, the values
**		level_[start|end|size]_[x|y] are ALWAYS EXCLUDING the reflect_[start|end]_[x|y] values,
**		so include those values when needed. However, the pointer to the start of line0 and line1
**		always INCLUDE the reflect_[start|end]_[x|y] values. What this means in english:
**		-	To allocate these buffers, use level_size_x + reflect_start_x + reflect_end_x
**		-	To read from the buffers, use line0[0] etc as the first value (which will be the first reflection
**			value if it exists).
**		-	To write to the buffers, use line0 + reflect_start_x as the first location to write to,
**			and only write level_size_x values (which will exclude the reflection).
**	   Just to be safe, buffers are always allocated as level_size_x + 2 just in case.
**
*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/

QmfRegionStruct *erw_decompress_start_region( QmfLevelStruct *p_top_qmf,
                 UINT nr_bands_requested,		// number of bands to read
                 UINT *band_list,				// index into actual band numbers from source file
                 UINT start_x, UINT start_y,
                 UINT end_x, UINT end_y,
                 UINT number_x, UINT number_y)
{
    UINT	x_size, y_size;
    QmfRegionLevelStruct	*p_level;
    QmfRegionStruct			*p_region;
    QmfLevelStruct			*p_qmf;
    UINT	output_level_start_y, output_level_end_y, output_level_size_y;
    UINT	output_level_start_x, output_level_end_x, output_level_size_x;
    UINT		band;
    int	sideband;

    if(!p_top_qmf) {
        return(NULL);
    }

    x_size = p_top_qmf->p_file_qmf->x_size;
    y_size = p_top_qmf->p_file_qmf->y_size;

    // Range check region. Must be inside image, and number to extract can not be
    // larger than start/end region size. So if you want to read from [2,3] to [10,15],
    // then number_x must be <= (1 + 10 - 2) and number_y must be <= [1 + 15 - 3]
    if( start_x > end_x || start_y > end_y
     || end_x >= x_size || end_y >= y_size
     || number_x > (1 + end_x - start_x)
     || number_y > (1 + end_y - start_y)
     || number_x == 0 || number_y == 0) {
        return(NULL);
    }
    // Sanity check band list
    if( nr_bands_requested > p_top_qmf->nr_bands ) {
        return(NULL);
    }
    for( band = 0; band < nr_bands_requested; band++ ) {
        if( band_list[band] >= p_top_qmf->nr_bands ) {
            return(NULL);
        }
    }

    /*
    **	Compute output size, and find the largest level that contains the needed level of detail
    */

    // work out level we need to go down to for data. In other words, we only
    // extract enough data to cover the number_x and number_y size requested.
    // It is possible that they asked for more detail in one dimension than in
    // another dimension (perhaps because of odd aspect pixels), so we check
    // both width and height to decide which level to use

    // these decide the edges of the level to use
    output_level_start_x = start_x; output_level_end_x = end_x;
    output_level_start_y = start_y; output_level_end_y = end_y;
    output_level_size_x  = 1 + output_level_end_x - output_level_start_x;		// start_y and end_y are stated in file units
    output_level_size_y  = 1 + output_level_end_y - output_level_start_y;		// so are x values
    // start searching at the level below the file level	
    p_qmf = p_top_qmf->p_file_qmf->p_smaller_qmf;
    // search for the smallest level that covers the number_[x|y] requested
    // Note that the level_height/width are the output size generated from
    // this level (e.g. 2 x this level size)
    while((output_level_size_y > number_y * 2) && (output_level_size_x > number_x * 2) ) {
        if( !p_qmf->p_smaller_qmf )
            break;	// force usage of the top level
        // scale down to the next level. We always treat odd numbers as scaling down to smaller
        output_level_start_x /= 2;
        output_level_start_y /= 2;
        output_level_end_x = output_level_end_x / 2;
        output_level_end_y = output_level_end_y / 2;
        // These must be recalculated not scaled, as the width/height might go to zero
        output_level_size_x  = 1 + output_level_end_x - output_level_start_x;		// start_y and end_y are stated in file units
        output_level_size_y  = 1 + output_level_end_y - output_level_start_y;		// so are x values
        p_qmf = p_qmf->p_smaller_qmf;
    }

    // We have found the level. However (particularly for the top level), we
    // may we have to super sample
    // the level, extracting just the values they want, while processing the entire
    // level region at the region resolution. We can make an exception where they
    // only want one pixel as we know there will be only one supersample operation,
    // so we just grab the first pixel. Where this pixel is gathered from is
    // debatable.  We pick the center pixel. The result is a big performance improvement.
    // This means single point point profiling, of course, will be much faster.
    if( number_x == 1 ) {
        output_level_start_x += ((output_level_end_x - output_level_start_x)/2);
        output_level_end_x = output_level_start_x;
        output_level_size_x = 1;
    }
    if( number_y == 1 ) {
        output_level_start_y += ((output_level_end_y - output_level_start_y)/2);
        output_level_end_y = output_level_start_y;
        output_level_size_y = 1;
    }


    // Set up the region info and allocate the structure pointers
    p_region = (QmfRegionStruct *) NCSMalloc(sizeof(QmfRegionStruct), FALSE);
    if( !p_region )
        return(NULL);

    p_region->random_value = 0xd4c5c239 ^ (UINT)((output_level_start_x + output_level_start_y * p_qmf->x_size)/* * NCSGetTimeStampMs()*/);//[14]
    p_region->p_top_qmf = p_top_qmf;
    p_region->p_largest_qmf = p_qmf;	// largest level we have to go to
    p_region->start_x = start_x;
    p_region->start_y = start_y;
    p_region->end_x = end_x;
    p_region->end_y = end_y;
    p_region->number_x = number_x;
    p_region->number_y = number_y;
    p_region->read_line = TRUE;			/* have to read the first line */
    p_region->p_p_ll_line = NULL;
    p_region->p_ll_buffer = NULL;
    // Later on, this will enable us to read a sub-set of the bands in the file
    // for now, we MUST read all bands, and extract the subset desired
    // FIXME! be more intelligent in reading bands.  Only YIQ mode (from RGB) needs
    // to read all bands regardless of the actual bands requested. Other band
    // combinations should be able to be read based on just those bands requested.
    // pack.c code will need to be checked - there is some code there that might have
    // to be updated to extract only those bands requested.  For now, read all bands,
    // then pass back only those requested
    p_region->used_bands = p_qmf->p_file_qmf->nr_bands;
    p_region->nr_bands_requested = nr_bands_requested;
    p_region->band_list = band_list;	// caller must alloc the band list, and free it AFTER the region close

    /*
    ** [08] If texture noise is acceptable, and if compression rate high enough to use it,
    **		and if not MULTI mode (e.g. only greyscale or YUV), then turn on texture noise
    ** [10] Only add texture noise if one of the larger QMF levels
    */
    if( pNCSEcwInfo->bNoTextureDither == FALSE ) {
        // Only turn on texture noise if compression rate was high enough to warrant it
        if( ((p_top_qmf->compress_format == COMPRESS_UINT8 && p_top_qmf->pFileInfo->nCompressionRate > 4)
            || (p_top_qmf->compress_format == COMPRESS_YUV && p_top_qmf->pFileInfo->nCompressionRate > 9))
         && ((p_region->p_largest_qmf->level > 2) || (p_top_qmf->nr_levels < 3)) /* [10] */ ) {
            p_region->bAddTextureNoise = TRUE;
            // [11] no longer using srand
        }
        else		
            p_region->bAddTextureNoise = FALSE;
    }
    else
        p_region->bAddTextureNoise = FALSE;

    // now set up increment information
    p_region->start_line = (IEEE4) output_level_start_y;	/**[06]**/
    p_region->current_line = (IEEE4) output_level_start_y;
    p_region->increment_y = (IEEE4) output_level_size_y / (IEEE4) number_y;
    p_region->increment_x = (IEEE4) output_level_size_x / (IEEE4) number_x;
    p_region->nCounter = 0;

    p_region->p_p_ll_line = (IEEE4 **)
        NCSMalloc(sizeof(IEEE4 *) * p_region->used_bands, FALSE);	// Ptr to bands for line of output
    p_region->p_ll_buffer = (IEEE4 *)
        NCSMalloc(sizeof(IEEE4) * output_level_size_x * p_region->used_bands, FALSE);	// final output line, created by generated qmf level
    // now set up region level buffer structures
    p_region->p_levels = (QmfRegionLevelStruct *)
        NCSMalloc(sizeof(QmfRegionLevelStruct) * (p_qmf->level+1) , FALSE);

    if( !(p_region->p_p_ll_line) || !(p_region->p_ll_buffer) || !p_region->p_levels ) {
        if( p_region->p_p_ll_line ) 
            NCSFree(p_region->p_p_ll_line);
        if( p_region->p_ll_buffer )
            NCSFree(p_region->p_ll_buffer);
        if( p_region->p_levels )
            NCSFree(p_region->p_levels);
        if( p_region )
            NCSFree(p_region);
        return(NULL);
    }
    // set up pointers to the output buffer
    for( band = 0; band < p_qmf->p_file_qmf->nr_bands; band++ )
        p_region->p_p_ll_line[band] = p_region->p_ll_buffer + (band * output_level_size_x);

    // set buffer pointers to NULL in case we need to free later; need to know what has been allocated so far
    p_qmf = p_region->p_largest_qmf;
    while(p_qmf) {
        p_region->p_levels[p_qmf->level].buffer_ptr = NULL;
        p_region->p_levels[p_qmf->level].p_x_blocks = NULL;
        p_region->p_levels[p_qmf->level].have_blocks = FALSE;
        p_region->p_levels[p_qmf->level].used_bands = p_region->used_bands;

        p_region->p_levels[p_qmf->level].p_p_line0 = NULL;
        p_region->p_levels[p_qmf->level].p_p_line1 = NULL;
        p_qmf = p_qmf->p_smaller_qmf;
    }

    p_region->pNCSFileView = NULL;	// Back pointer to the NCS FileView, set up by the NCScbmSetFileView

    // NOTE: From now on, we can use use the close_region call during error handling,
    // as the region has been set up well enough to be handled by that call


    // Define and allocate level values and memory for each level.
    // We run two sets of values:
    // output_level_???? and level_????.  The output values are
    // the start/end/size for the output from this level, and
    // the level values are the start/end/size for this level data

    p_qmf = p_region->p_largest_qmf;
    while(p_qmf) {
        UINT	level_start_y, level_end_y, level_size_y;
        UINT	level_start_x, level_end_x, level_size_x;
        UINT	reflect_start_x, reflect_start_y, reflect_end_x, reflect_end_y;
        UINT	start_x_block;
        UINT	last_x_block;

        p_level = &(p_region->p_levels[p_qmf->level]);
        p_level->p_region	= p_region;
        p_level->p_qmf		= p_qmf;
        // set up level information from previous level's output information
        // Must handle reflection where the start or end might be at zero.
        // We only check start reflection based on start_[x|y], as if end_[x|y] is zero
        // and needs left/top reflection, then so will start values be zero. Same logic
        // in reverse for the end - sorting end values out must sort out high start values
        // also.
        if( output_level_start_x ) {
            level_start_x	= (output_level_start_x - 1)/2;		// (N-1)/2
            reflect_start_x = 0;
        }
        else {
            level_start_x = 0;
            reflect_start_x = 1;
        }
        if( output_level_end_x < (p_qmf->p_larger_qmf->x_size-1) ) {
            if( output_level_end_x )
                level_end_x	= (output_level_end_x   - 1)/2+1;	// (N-1)/2+1
            else
                level_end_x = 0;
            reflect_end_x = 0;
        }
        else {
            level_end_x = p_qmf->x_size - 1;
            reflect_end_x = 1;
        }

        if( output_level_start_y ) {
            level_start_y	= (output_level_start_y - 1)/2;		// (N-1)/2
            reflect_start_y = 0;
        }
        else {
            level_start_y = 0;
            reflect_start_y = 1;
        }
        if( output_level_end_y < (p_qmf->p_larger_qmf->y_size-1) ) {
            if( output_level_end_y )
                level_end_y	= (output_level_end_y - 1)/2+1;	// (N-1)/2+1
            else
                level_end_y = 0;
            reflect_end_y = 0;
        }
        else {
            level_end_y = p_qmf->y_size - 1;
            reflect_end_y = 1;
        }

        // these values EXCLUDE the reflection size
        level_size_x	= 1 + level_end_x - level_start_x;
        level_size_y	= 1 + level_end_y - level_start_y;

        // set up values start / end X values etc.

        p_level->reflect_start_x	= (UINT8)reflect_start_x;
        p_level->reflect_end_x		= (UINT8)reflect_end_x;
        p_level->reflect_start_y	= (UINT8)reflect_start_y;
        p_level->reflect_end_y		= (UINT8)reflect_end_y;

        // track level information EXCLUDING reflections
        p_level->level_start_x	= level_start_x;
        p_level->level_end_x	= level_end_x;
        p_level->level_size_x	= level_size_x;
        p_level->level_start_y	= level_start_y;
        p_level->level_end_y	= level_end_y;
        p_level->level_size_y	= level_size_y;

        p_level->output_level_start_x	= output_level_start_x;
        p_level->output_level_end_x		= output_level_end_x;
        p_level->output_level_size_x	= output_level_size_x;
        p_level->output_level_start_y	= output_level_start_y;
        p_level->output_level_end_y		= output_level_end_y;
        p_level->output_level_size_y	= output_level_size_y;
        if( p_level->p_qmf->x_size <= level_end_x
         || p_level->p_qmf->y_size <= level_end_y ) {

            erw_decompress_end_region(p_region);
            return(NULL);
        }			

        // Work out starting and ending block number in the X direction set of blocks, then
        // set up enough pointers to X blocks as needed for a line in this level
        start_x_block	= level_start_x / p_level->p_qmf->x_block_size;
        last_x_block	= level_end_x   / p_level->p_qmf->x_block_size;
        p_level->start_x_block = start_x_block;
        p_level->x_block_count = 1 + last_x_block - start_x_block;

        // set up the pre and post block skip calculations for later quick access
        // This is because the desired X start might be larger than the start of the block,
        // and ditto for the end number.
        p_level->first_block_skip = level_start_x - (start_x_block * p_level->p_qmf->x_block_size);
        // last X block might be smaller than the multiple of x_block_size * (last_x_block+1)
        {
            UINT	last_pixel;
            last_pixel = (last_x_block+1) * p_level->p_qmf->x_block_size - 1;
            if( last_pixel >= p_level->p_qmf->x_size )
                last_pixel = p_level->p_qmf->x_size - 1;
            p_level->last_block_skip = last_pixel - level_end_x;
        }
        // now go and init the line decompression blocks
        if( unpack_init_lines(p_level) ) {
            erw_decompress_end_region(p_region);
            return(NULL);
        }

        // set up multi line ring buffer
        p_level->start_line = level_start_y;		/**[06]**/
        p_level->current_line = level_start_y;
        p_level->start_read_lines = 2 - reflect_start_y;	/**[06]**/
        p_level->read_lines = 2 - reflect_start_y;	// number of lines to read at first, ignoring reflection
        // allocate the line buffers. There are 2 for each sideband, line0 and line1,
        // and this is for each band in the file.
        // NOTE WELL: The buffer for each line must handle -1 and +1 reflection (for edges)
        // or for data gathering outside the area of interest, so these lines are allocated
        // with TWO more values than needed for the level.
        p_level->buffer_ptr =
            (IEEE4 *) NCSMalloc(sizeof(IEEE4) * p_qmf->nr_bands * (level_size_x+2) * 2 * MAX_SIDEBAND, TRUE);	// add 2 to line lengths **[06]** - changed to calloc()
        if( !p_level->buffer_ptr ) {
            erw_decompress_end_region(p_region);
            return(NULL);
        }
        // point into the buffers. We keep line types together, e.g. line0[LL] and line1[LL]
        // This improves CPU caching of lines, resulting in faster execution.
        p_level->p_p_line0 = (IEEE4 **) NCSMalloc( sizeof(IEEE4 *) * p_level->used_bands * MAX_SIDEBAND, FALSE);
        p_level->p_p_line1 = (IEEE4 **) NCSMalloc( sizeof(IEEE4 *) * p_level->used_bands * MAX_SIDEBAND, FALSE);
        p_level->p_p_line1_ll_sideband = (IEEE4 **) NCSMalloc( sizeof(IEEE4 *) * p_level->used_bands, FALSE);
        if( !p_level->p_p_line0 || !p_level->p_p_line1 || !p_level->p_p_line1_ll_sideband) {
            erw_decompress_end_region(p_region);
        }
        for( band = 0; band < p_level->used_bands; band++ ) {
            for( sideband = LL_SIDEBAND; sideband < MAX_SIDEBAND; sideband++ ) {
                p_level->p_p_line0[DECOMP_INDEX] = DECOMP_LEVEL_LINE01(p_level, band, sideband, 0);
                p_level->p_p_line1[DECOMP_INDEX] = DECOMP_LEVEL_LINE01(p_level, band, sideband, 1);
            }
            p_level->p_p_line1_ll_sideband[band] = p_level->p_p_line1[band * MAX_SIDEBAND + LL_SIDEBAND] +
                                                   p_level->reflect_start_x;
        }
        // and set up output values for the next smaller level
        output_level_start_x = level_start_x;	output_level_end_x = level_end_x;
        output_level_start_y = level_start_y;	output_level_end_y = level_end_y;
        output_level_size_x  = level_size_x;
        output_level_size_y	 = level_size_y;

        p_qmf = p_qmf->p_smaller_qmf;
    }

    return(p_region);
}

/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	
**
**	erw_decompress_end_region().  Finished with and deallocate
**	the region structure. Can be called at the true end,
**	or if reading is to be aborted.
**
**	No errors are returned.
**
*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/

void erw_decompress_end_region( QmfRegionStruct *p_region )
{
    int level;
    QmfRegionLevelStruct	*p_level;
    
    if( !p_region )
        return;
    level = p_region->p_largest_qmf->level;
    while(level >= 0) {
        p_level = &p_region->p_levels[level];
        unpack_free_lines(p_level);		// handles freeing anything under the x_blocks pointer
        NCSFree((char *) p_region->p_levels[level].buffer_ptr);
        NCSFree((char *) p_level->p_p_line0);
        NCSFree((char *) p_level->p_p_line1);
        NCSFree((char *) p_level->p_p_line1_ll_sideband);
        p_level->p_p_line1 = NULL;
        --level;
    }

    NCSFree((char *) p_region->p_p_ll_line);
    NCSFree((char *) p_region->p_ll_buffer);
    NCSFree((char *) p_region->p_levels);
    NCSFree((char *) p_region);

}

/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	
**
**	erw_decompress_close(). Closes a ERW file.
**
**	No errors are returned.
**
*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/

void erw_decompress_close( QmfLevelStruct *p_top_qmf )
{
    delete_qmf_levels(p_top_qmf);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// ecw_read.c
//

#define RANDOM_MASK 0xd4c5c239				// [10] for random number mask

// [10] updated to use new inline random number generator
#define ADD_TEXTURE_NOISE(value) {				\
    register UINT	nNoise;						\
    register UINT8	nSign;						\
    nNoise = p_region->random_value;			\
    nNoise = (nNoise & 1) ? (nNoise>>1)^RANDOM_MASK : nNoise >>1;	\
    if( !nNoise ) nNoise = 1;					\
    p_region->random_value = nNoise;			\
    nSign = (UINT8) nNoise;						\
    nNoise &= 0x1f;								\
    if( nNoise > 10 )							\
        nNoise = nNoise & 0x3;					\
    if( (value) > 1.0 )	{						\
        if( nSign & 0x20 )						\
            (value) -= nNoise;					\
        else									\
            (value) += nNoise;					\
    }											\
}

#define WRITE_MULTI_BYTE(pIn)				\
    if( pIn ) {								\
        rescale = *((pIn) + nTrueXoffset);	\
        if( rescale < 0 )					\
            *p_out++ = 0;					\
        else if( rescale > 255 )			\
            *p_out++ = 255;					\
        else								\
            FLT_TO_UINT8(*p_out++, rescale);\
    }										\
    else									\
        *p_out++ = 0;

#define WRITE_RESCALE(pOut)					\
    {										\
        INT nRescale;						\
        FLT_TO_INT32(nRescale, rescale);	\
        if( nRescale < 0 )					\
            *pOut++ = 0;					\
        else if( nRescale > 255 )			\
            *pOut++ = 255;					\
        else								\
            *pOut++ = (UINT8)nRescale;				\
    }

/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	
**
**	erw_decompress_read_region_line_bil().  Reads the next sequentional
**	line from the region, in bil format.
**
**	Returns non-zero if an error
**
**	Because floating point is very slow, we use fixed point scaled
**	integer maths to work out the X increment. The lower 32 bits
**	are the precision; the higher 32 bits the actual offset
**
**  [13] nOutputType is only relevant if the ECW file is COMPRESS_MULTI
**  
*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/

/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	
**
**	erw_decompress_read_region_line().  Reads the next sequentional
**	line from the region.
**
**	Returns non-zero if an error
**
**	The image input is always converted to BGRA triplets
**	(for example a greyscale image will be output to R,G, B and Alpha)
**	
**	Added for platforms that need 32 bit RGBA images to blit from (UNIX X)
**  For now, alpha is not supported and will be 0. Ideally, before texturing
**  occurs, you could set the alpha to be the background color and get transparent images.
**
**	**[12]**
**
*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/

int erw_decompress_read_region_line( QmfRegionStruct *p_region, UINT8 *pPixelBundle)
{
    IEEE8	y_line;   //[18]
    UINT	 nLineY;

    register UINT	x;
    register UINT64 increment_x, x_offset;
    register UINT8 *p_out = pPixelBundle;

    if( !p_region )
        return( 1 );

    y_line = p_region->current_line;
    nLineY = (UINT) y_line;

    if(nLineY == (UINT)p_region->start_line) {		/**[11]**/
        p_region->pNCSFileView->nNextDecodeMissID++;
    }

    while( p_region->read_line ) {
        if( qdecode_qmf_level_line( p_region, p_region->p_largest_qmf->level,
            (nLineY - p_region->read_line) + 1, p_region->p_p_ll_line) )
            return(1);
        p_region->read_line--;
    }

    // copy to output line
    x = p_region->number_x;
    x_offset = 0;		// actually 0 << 32
    
    // This gives us 1/(2^16) accuracy per line, which should be fine
    increment_x = ((UINT64) ((p_region->increment_x) * 0x10000) << 16);

    //	Decode the data. Depends on the format data was encoded in
    switch( p_region->p_top_qmf->compress_format ) {
    default :
    case COMPRESS_UINT8: {		// input file is single band, UINT8, greyscale
            register IEEE4 *p_in = p_region->p_p_ll_line[0];
            FLT_TO_INT_INIT();

            while(x--) {
                register IEEE4 rescale;
                rescale = *(p_in + (UINT) (x_offset >> 32));
                if( p_region->bAddTextureNoise ) 	/**[08]**/
                    ADD_TEXTURE_NOISE(rescale);

                WRITE_RESCALE(p_out);
                WRITE_RESCALE(p_out);
                WRITE_RESCALE(p_out);
                *p_out++ = 0;	// alpha value
                
                x_offset += increment_x;
            }
            FLT_TO_INT_FINI();
        }
        break;

    // They could have requested any number of bands. We always use the first bands
    // requested, into RGB, and zero-fill if there were not enough bands
    // We NEVER add texture noise to MULTI compressed files
    case COMPRESS_MULTI: {
            UINT	band =  p_region->nr_bands_requested;
            register IEEE4 *p_in_red, *p_in_green, *p_in_blue;
            p_in_red   = ( band > 0 ? p_region->p_p_ll_line[p_region->band_list[0]] : NULL);
            p_in_green = ( band > 1 ? p_region->p_p_ll_line[p_region->band_list[1]] : NULL);
            p_in_blue  = ( band > 2 ? p_region->p_p_ll_line[p_region->band_list[2]] : NULL);

            FLT_TO_INT_INIT();			/**[04]**/
            while(x--) {
                register UINT nTrueXoffset = (UINT) (x_offset >> 32);
                register IEEE4 rescale;

                WRITE_MULTI_BYTE(p_in_red)
                WRITE_MULTI_BYTE(p_in_green)
                WRITE_MULTI_BYTE(p_in_blue)
                *p_out++ = 0;	// alpha value

                x_offset += increment_x;
            }
            FLT_TO_INT_FINI();			/**[04]**/
        }
        break;

    // Input must always be RGB 3 band. Converts to YIQ for better compression
    // Think of YIQ mode as RGB mode. The only time you would not use this
    // for a 3 band image is where the image is not color, and there is no
    // correlation between the bands (so the bands are not R,G,B).
    // We are using the JPEG standard YUV here (digital YUV). See:
    //	http://icib.igd.fhg.de/icib/it/iso/is_10918-1/pvrg-descript/chapter2.5.html
    case COMPRESS_YUV: {
            register IEEE4 *p_in_y = p_region->p_p_ll_line[0];	// always YIQ input
            register IEEE4 *p_in_u = p_region->p_p_ll_line[1];
            register IEEE4 *p_in_v = p_region->p_p_ll_line[2];
            register IEEE4 y,u,v;

            if( p_region->nr_bands_requested == 3 ) { // RGB, so optimize for this
                FLT_TO_INT_INIT();			/**[04]**/
                while(x--) {
                    register IEEE4 rescale;
                    register UINT	nTrueXoffset = (UINT) (x_offset >> 32);
                    y = *(p_in_y + nTrueXoffset);
                    u = *(p_in_u + nTrueXoffset);
                    v = *(p_in_v + nTrueXoffset);
                    if( p_region->bAddTextureNoise ) 	/**[08]**/

                    {
                        register UINT	nNoise;						
                        register UINT8	nSign;						
                        nNoise = p_region->random_value;			
                        nNoise = (nNoise & 1) ? (nNoise>>1)^RANDOM_MASK : nNoise >>1;	
                        if( !nNoise ) nNoise = 1;					
                        p_region->random_value = nNoise;			
                        nSign = (UINT8) nNoise;						
                        nNoise &= 0x1f;								
                        if( nNoise > 10 )							
                            nNoise = nNoise & 0x3;					
                        if( (y) > 1.0 )	{						
                            if( nSign & 0x20 )						
                                (y) -= nNoise;					
                            else									
                                (y) += nNoise;					
                        }											
                    }
                    rescale = y + ((IEEE4) 1.402 * v);
                    WRITE_RESCALE(p_out);				// RED

                    rescale = y + ((IEEE4) -0.34414 * u) + ((IEEE4) -0.71414 * v);
                    WRITE_RESCALE(p_out);				// GREEN

                    rescale = y + ((IEEE4) 1.772 * u);
                    WRITE_RESCALE(p_out);				// BLUE

                    *p_out++ = 0;					// ALPHA

                    x_offset += increment_x;
                }
                FLT_TO_INT_FINI();			/**[04]**/

            } else if ( p_region->nr_bands_requested == 1 ) {	// greyscale, so optimize for this
                register UINT band  = p_region->band_list[0];

                FLT_TO_INT_INIT();			/**[04]**/
                while(x--) {
                    register IEEE4 rescale;
                    register UINT	nTrueXoffset = (UINT) (x_offset >> 32);
                    y = *(p_in_y + nTrueXoffset);
                    u = *(p_in_u + nTrueXoffset);
                    v = *(p_in_v + nTrueXoffset);
                    switch( band ) {
                        default :
                        case 0 :	// RED 
                            rescale = y + ((IEEE4) 1.402 * v);
                        break;
                        case 1 :	// GREEN 
                            rescale = y + ((IEEE4) -0.34414 * u) + ((IEEE4) -0.71414 * v);
                        break;
                        case 2 :	// BLUE 
                            rescale = y + ((IEEE4) 1.772 * u);
                        break;
                    }
                    WRITE_RESCALE(p_out);
                    WRITE_RESCALE(p_out);
                    WRITE_RESCALE(p_out);
                    *p_out++ = 0;	// alpha value
                    
                    x_offset += increment_x;
                }
                FLT_TO_INT_FINI();			/**[04]**/
            } else {
                // Some random 2 band combination of bands. We will just ramble through them
                // greyscale, so optimize for this
                // BLUE is set to zero in this mode
                register UINT band0  = p_region->band_list[0];
                register UINT band1  = p_region->band_list[1];

                FLT_TO_INT_INIT();			/**[04]**/
                while(x--) {
                    register IEEE4 rescale;
                    register UINT	nTrueXoffset = (UINT) (x_offset >> 32);
                    y = *(p_in_y + nTrueXoffset);
                    u = *(p_in_u + nTrueXoffset);
                    v = *(p_in_v + nTrueXoffset);
                    
                    switch( band0 ) {
                        default :
                        case 0 :	// RED
                            rescale = y + ((IEEE4) 1.402 * v);
                        break;
                        case 1 :	// GREEN
                            rescale = y + ((IEEE4) -0.34414 * u) + ((IEEE4) -0.71414 * v);
                        break;
                        case 2 :	// BLUE
                            rescale = y + ((IEEE4) 1.772 * u);
                        break;
                    }
                    WRITE_RESCALE(p_out);

                    switch( band1 ) {
                        default :
                        case 0 :	// RED
                            rescale = y + ((IEEE4) 1.402 * v);
                        break;
                        case 1 :	// GREEN 
                            rescale = y + ((IEEE4) -0.34414 * u) + ((IEEE4) -0.71414 * v);
                        break;
                        case 2 :	// BLUE
                            rescale = y + ((IEEE4) 1.772 * u);
                        break;
                    }
                    WRITE_RESCALE(p_out);

                    *p_out++ = 0;		// set BLUE to zero (last UINT8 if RGB, third UINT8 if RGBA)

                    *p_out++ = 0;		// alpha value

                    x_offset += increment_x;
                }
                FLT_TO_INT_FINI();			/**[04]**/
            }
        }
        break;
    }	// end switch compression format

    // work out if next line is duplicate or read
    p_region->nCounter++; //[17]
    y_line = p_region->start_line +  p_region->increment_y * p_region->nCounter;
#ifdef POSIX
    gcc_optimisation_workaround(y_line); //[19]
#endif

    if( (UINT) y_line <= (UINT) p_region->current_line )
        p_region->read_line = 0;	// we don't need to read a line
    else {
        // in a rare case, the user might request 1:1 of X output, but N:1 of Y output.
        // The only way to handle this is to read multiple lines for the next read,
        // and return up the last line read. So we compute the number of lines to read
        // here, rather than just one line (which is what will normally be requested)
        p_region->read_line = (UINT) y_line - (UINT) p_region->current_line;
    }
    
    p_region->current_line = y_line; //[17]
  
   return(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// fileio_decompress.c
//

/*
**	EcwFileOpenForRead()
**	Open file for reading. Returns TRUE if error, otherwise fills in pFile handle,
**	which *might* (in Windows anyway) be NULL and still valid - so you have to
**	track open files using another variable. File is always opened in shared, read, binary, mode.
*/

BOOLEAN EcwFileOpenForRead(char *szFilename, ECWFILE *pFile)
{
    if(NCSFileOpen(szFilename, NCS_FILE_READ, &(pFile->hFile)) == NCS_SUCCESS) {
        return(FALSE);
    } else {
        return(TRUE);
    }
}

/*
**	EcwFileClose()	- closes a ECW file handle that had been opened for reading
*/
BOOLEAN EcwFileClose(ECWFILE hFile)								// Closes file
{
    if(NCSFileClose(hFile.hFile) == NCS_SUCCESS) {
        return(FALSE);
    } else {
        return(TRUE);
    }
}


/*
**	EcwFileRead()
**	Reads nLength bytes into pBuffer from the File handle. Returns FALSE
**	if all went well, or TRUE if something went wrong.
*/
BOOLEAN EcwFileRead(ECWFILE hFile, void *pBuffer, UINT nLength)
{
    if(NCSFileRead(hFile.hFile, pBuffer, nLength, &nLength) == NCS_SUCCESS) {
        return(FALSE);
    } else {
        return(TRUE);
    }
}

/*
**	EcwFileSetPos()
**	Returns the current seek position as a UINT64 value, and FALSE
**	if all went well, or TRUE if something went wrong.  Windows has
**	a particularly nasty way of handling with long file offsets.
*/
BOOLEAN EcwFileSetPos(ECWFILE hFile, UINT64 nOffset)
{
    if(NCSFileSeek(hFile.hFile, nOffset, NCS_FILE_SEEK_START) == (INT64)nOffset) {
        return(FALSE);
    } else {
        return(TRUE);
    }
}


/*
**	EcwFileGetPos()
**	Returns the current seek position as a UINT64 value, and FALSE
**	if all went well, or TRUE if something went wrong.  Windows has
**	a particularly nasty way of handling with long file offsets.
*/
BOOLEAN EcwFileGetPos(ECWFILE hFile, UINT64 *pOffset)
{
    *pOffset = NCSFileTell(hFile.hFile);
    return(FALSE);
}



/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*
**	ERW HEADER MACHINE INDEPENDANT IO routines.
*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/

/*
**	Read/write functions. Data is always written in machine independant format.
**	All functions return non-zero if an error.
**	These are quite slow, so only use them for reading/writing small numbers
**	of header bytes.
**
** NOTE:  DUE TO A COMPLETE COCKUP THE INTS IN THE HEADER ARE STORED ON DISC AS
**        MSB WHILST FLOATS, THE BLOCK TABLE AND THE REST OF THE DATA IS LSB!
*/


BOOLEAN EcwFileReadUint8(ECWFILE hFile, UINT8 *sym)
{
    return( NCSFileReadUINT8_MSB(hFile.hFile, sym) == NCS_SUCCESS ? FALSE : TRUE );
}

BOOLEAN EcwFileReadUint16(ECWFILE hFile, UINT16 *sym16)
{
    return( NCSFileReadUINT16_MSB(hFile.hFile, sym16) == NCS_SUCCESS ? FALSE : TRUE );
}

BOOLEAN EcwFileReadUint32(ECWFILE hFile, UINT *sym32)
{
    return( NCSFileReadUINT32_MSB(hFile.hFile, sym32) == NCS_SUCCESS ? FALSE : TRUE );
}

BOOLEAN EcwFileReadIeee8(ECWFILE hFile, IEEE8 *symd8)
{
    return( NCSFileReadIEEE8_LSB(hFile.hFile, symd8) == NCS_SUCCESS ? FALSE : TRUE );
}

// read a machine independant 4 byte value from a string
UINT sread_int32(UINT8 *p_s)
{
#ifdef NCSBO_MSBFIRST
#ifdef NCS_NO_UNALIGNED_ACCESS
    UINT n;
    memcpy(&n, p_s, sizeof(UINT));
    return(n);
#else
    return(*((UINT*)p_s));
#endif
#else
    #if !defined(_WIN32_WCE)
        return(NCSByteSwap32(*((UINT*)p_s)));
    #else
        UINT8 *ptr = p_s;
        UINT8 p1 = *ptr++;
        UINT8 p2 = *ptr++;
        UINT8 p3 = *ptr++;
        UINT8 p4 = *ptr;

        UINT ps = (p4 << 24) | (p3 << 16) | (p2 << 8) | p1;
        return(NCSByteSwap32(ps));
    #endif
#endif
}

// read a machine independant 2 byte value from a string
UINT16 sread_int16(UINT8 *p_s)
{
#ifdef NCSBO_MSBFIRST
#ifdef NCS_NO_UNALIGNED_ACCESS
    UINT16 n;
    memcpy(&n, p_s, sizeof(UINT16));
    return(n);
#else
    return(*((UINT16*)p_s));
#endif
#else
    return(NCSByteSwap16(*((UINT16*)p_s)));
#endif
}

// Read IEEE8 in machine independant way
void sread_ieee8(IEEE8 *sym, UINT8 *p_s)
{
    memcpy(sym, p_s, sizeof(IEEE8));
#ifdef NCSBO_MSBFIRST
    NCSByteSwapRange64((UINT64 *)sym, (UINT64 *)sym, 1);
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// ncscbm.c
//

int NCScbmFileViewRequestBlocks(NCSFileView *pNCSFileView, QmfRegionStruct *pQmfRegion, NCSEcwBlockRequestMethod eRequest);
UINT8	*NCScbmReadFileBlockLocal_ECW(NCSFile *pNCSFile, NCSBlockId nBlock, UINT *pBlockLength );

/*******************************************************
**	NCScbmSetFileView()	- set bands/extents/size for view area
**
**	Notes:
**	(1)	-	You can alter any or none of the parameters
**	(2)	-	You can't read from the view until at least one SetViewFile call is done
**	(3)	-	Only one thread fiddles with QMF Region at one time
**	(4) -	A refresh callback will be made immediately if all blocks are already available
**
********************************************************/

NCSError	NCScbmSetFileViewEx_ECW(NCSFileView *pNCSFileView,
                UINT nBands,					// number of bands to read
                UINT *pBandList,				// index into actual band numbers from source file
                UINT nTopX, UINT nLeftY,	// Top-Left in image coordinates
                UINT nBottomX, UINT nRightY,// Bottom-Right in image coordinates
                UINT nSizeX, UINT nSizeY,	// Output view size in window pixels
                IEEE8 fTopX, IEEE8 fLeftY,		// Top-Left in world coordinates
                IEEE8 fBottomX, IEEE8 fRightY)	// Bottom-Right in world coordinates
{
    QmfRegionStruct	*pOldQmfRegion;
    NCSCacheMethod	nOldCacheMethod, nNewCacheMethod;

    if (!pNCSEcwInfo) {
        NCSecwInitInternal();
    }

#define MAXRECONNECTCOUNT 4


    if(pNCSFileView->pNCSFile->pTopQmf && pNCSFileView->pNCSFile->pTopQmf->p_file_qmf) {		/**[06]**/
        UINT x_size = pNCSFileView->pNCSFile->pTopQmf->p_file_qmf->x_size;
        UINT y_size = pNCSFileView->pNCSFile->pTopQmf->p_file_qmf->y_size;
        UINT nBand;

        // Range check region. Must be inside image, and number to extract can not be
        // larger than start/end region size. So if you want to read from [2,3] to [10,15],
        // then number_x must be <= (1 + 10 - 2) and number_y must be <= [1 + 15 - 3]
        if( nTopX > nBottomX || nLeftY > nRightY
            || nBottomX >= x_size || nRightY >= y_size) {	/**[06]**/
            return(NCS_REGION_OUTSIDE_FILE);
        }
        if(nSizeX == 0 || nSizeY == 0) {/**[06]**/
            return(NCS_ZERO_SIZE);
        }
        // Sanity check band list
        if( nBands > pNCSFileView->pNCSFile->pTopQmf->nr_bands ) {/**[06]**/
            return(NCS_TOO_MANY_BANDS);
        }
        for( nBand = 0; nBand < nBands; nBand++ ) {/**[06]**/
            if( pBandList[nBand] >= pNCSFileView->pNCSFile->pTopQmf->nr_bands ) {
                return(NCS_INVALID_BAND_NR);
            }
        }
    }

    pOldQmfRegion = pNCSFileView->pQmfRegion;
    nOldCacheMethod = pNCSFileView->nCacheMethod;

    nNewCacheMethod = NCS_CACHE_VIEW;

    // If swapping from View to Non-view mode or vica-versa, must
    // flush the old blocks and close the old QMF structure first

    if( pOldQmfRegion && (nNewCacheMethod != nOldCacheMethod) ) {
        if( nOldCacheMethod == NCS_CACHE_VIEW )
            NCScbmFileViewRequestBlocks(pNCSFileView, pOldQmfRegion, NCSECW_BLOCK_CANCEL);
        erw_decompress_end_region(pOldQmfRegion);
        pOldQmfRegion = NULL;
    }

    // Request blocks for the new region

    pNCSFileView->info.nBands	= nBands;
    // [02] copy from client band list so that they can free it, and we don't have
    //		to track multiple band lists during multiple pending setviews
    {
        UINT	nCopy = 0;
        while( nCopy < nBands ) {
            pNCSFileView->info.pBandList[nCopy] = pBandList[nCopy];
            nCopy++;
        }
    }
    pNCSFileView->info.nTopX		= nTopX;
    pNCSFileView->info.nLeftY	= nLeftY;			// Top-Left in image coordinates
    pNCSFileView->info.nBottomX	= nBottomX;
    pNCSFileView->info.nRightY	= nRightY;			// Bottom-Left in image coordinates
    pNCSFileView->info.nSizeX	= nSizeX;
    pNCSFileView->info.nSizeY	= nSizeY;			// Size of window
    pNCSFileView->info.fTopX	= fTopX;
    pNCSFileView->info.fLeftY	= fLeftY;			// Top-Left in world coordinates
    pNCSFileView->info.fBottomX	= fBottomX;
    pNCSFileView->info.fRightY	= fRightY;			// Bottom-Right in world coordinates
    pNCSFileView->info.nMissedBlocksDuringRead = 0;	// no blocks failed on read so far
    pNCSFileView->bIsRefreshView = FALSE;			// A new view
    pNCSFileView->eCallbackState = NCSECW_VIEW_SET;
    pNCSFileView->nPending = 0;						// nothing pending right now
    pNCSFileView->nCacheMethod = nNewCacheMethod;

    pNCSFileView->pQmfRegion = erw_decompress_start_region( 
                pNCSFileView->pNCSFile->pTopQmf,
                 nBands, pNCSFileView->info.pBandList,
                 nTopX, nLeftY, nBottomX, nRightY,
                 nSizeX, nSizeY);

    if( !pNCSFileView->pQmfRegion ) {
        if( pOldQmfRegion )
            erw_decompress_end_region(pOldQmfRegion);
        pNCSFileView->eCallbackState = NCSECW_VIEW_QUIET;	// error, so back to quiet state
        return(NCS_COULDNT_ALLOC_MEMORY);
    }

    pNCSFileView->pQmfRegion->pNCSFileView = pNCSFileView;

    // Now request blocks that cover the new view
    if( pNCSFileView->nCacheMethod == NCS_CACHE_VIEW )
        NCScbmFileViewRequestBlocks(pNCSFileView, pNCSFileView->pQmfRegion, NCSECW_BLOCK_REQUEST);

    // Now remove the old cache request count for the old view
    // Only VIEW types had block reads cached
    if( pOldQmfRegion && (nOldCacheMethod == NCS_CACHE_VIEW) )
        NCScbmFileViewRequestBlocks(pNCSFileView, pOldQmfRegion, NCSECW_BLOCK_CANCEL);

    // Now we can remove the old set view's QMF structures
    if( pOldQmfRegion )
        erw_decompress_end_region(pOldQmfRegion);

    // Last time a block was received for this file view. Initially set to time of File View
    // Also update last time a Set View was done, to assist cache purging logic
    pNCSFileView->tLastBlockTime = NCSGetTimeStampMs();

    if(pNCSFileView->pNCSFile->bFileIOError) { // if read local block failed //[20]
        return(NCS_FILEIO_ERROR);
    }
    else return(NCS_SUCCESS);
}

NCSError	NCScbmSetFileView_ECW(NCSFileView *pNCSFileView,
                UINT nBands,					// number of bands to read
                UINT *pBandList,				// index into actual band numbers from source file
                UINT nTopX, UINT nLeftY,	// Top-Left in image coordinates
                UINT nBottomX, UINT nRightY,// Bottom-Left in image coordinates
                UINT nSizeX, UINT nSizeY)	// Output view size in window pixels
{
    return(NCScbmSetFileViewEx_ECW(pNCSFileView,
                                   nBands,
                                   pBandList,
                                   nTopX, nLeftY,
                                   nBottomX, nRightY,
                                   nSizeX, nSizeY,
                                   nTopX, nLeftY,
                                   nBottomX, nRightY));
}


/*******************************************************
**	NCScbmFileViewRequestBlocks() - Request blocks for the area
**	covered by the QMF region WHICH MAY BE DIFFERENT
**	from the QMF region pointed to by the File View.
**	If the eRequest == NCSECW_BLOCK_REQUEST, then REQUEST the blocks.
**	If the eRequest == NCSECW_BLOCK_CANCEL, then CANCEL the blocks.
**
**	If the eRequest == NCSECW_BLOCK_REQUEST, then updates Blocks in View and other values
********************************************************/
int NCScbmFileViewRequestBlocks(NCSFileView *pNCSFileView, QmfRegionStruct *pQmfRegion, NCSEcwBlockRequestMethod eRequest)
{
    UINT16					nLevel = 0;
    UINT					nBlocks, nBlocksAvailable, nLevelBlocks;

    nBlocks = nBlocksAvailable = 0;
    // Traverse from smallest to largest level, working out blocks we need
    while( nLevel <= pQmfRegion->p_largest_qmf->level ) {
        QmfRegionLevelStruct	*pLevel = &(pQmfRegion->p_levels[nLevel]);
        QmfLevelStruct			*pQmf = pLevel->p_qmf;

        UINT					nStartXBlock, nEndXBlock, nStartYBlock, nEndYBlock;
        UINT					nRowsOfBlocks;

        nStartXBlock	= pLevel->start_x_block;
        nEndXBlock		= (nStartXBlock + pLevel->x_block_count) - 1;
        nStartYBlock	= pLevel->level_start_y / pQmf->y_block_size;
        nEndYBlock		= pLevel->level_end_y   / pQmf->y_block_size;
        nLevelBlocks = ((nEndXBlock - nStartXBlock)+1) * ((nEndYBlock - nStartYBlock)+1);

        // We now have the rectangle of blocks required for this level.
        // Loop through the rows of blocks, and request the blocks from the actual file
        nRowsOfBlocks = 1 + (nEndYBlock - nStartYBlock);
        while(nRowsOfBlocks--) {
            UINT					nFileBlock;
            UINT					nFileBlockCount;
            // Now compute block number in the file for this level's block

            nFileBlock = pQmf->nFirstBlockNumber +
                            (pQmf->nr_x_blocks * nStartYBlock) + nStartXBlock;
            nFileBlockCount = 1 + (nEndXBlock - nStartXBlock);
            nStartYBlock += 1;		// move on to the next row

            while(nFileBlockCount--) {
                NCSFileCachedBlock	*pBlock;
                pBlock = NCScbmGetCacheBlock(pNCSFileView->pNCSFile, pNCSFileView->pNCSFile->pWorkingCachedBlock,
                                            nFileBlock, eRequest);
                pNCSFileView->pNCSFile->pWorkingCachedBlock = pBlock;
                _ASSERT( pBlock );
                if( !pBlock )
                    return(1);		// Internal logic error
                if( eRequest == NCSECW_BLOCK_REQUEST && pBlock->pPackedECWBlock)
                    nBlocksAvailable += 1;
                nFileBlock += 1;
            }
        }

        nBlocks += nLevelBlocks;
        nLevel++;
    }
    if( eRequest == NCSECW_BLOCK_REQUEST ) {
        pNCSFileView->info.nBlocksInView = nBlocks;
        pNCSFileView->info.nBlocksAvailableAtSetView = pNCSFileView->info.nBlocksAvailable = nBlocksAvailable;
    }
    return(0);
}

/*******************************************************
**	NCScbmGetCacheBlock() - add/remove/find block in cache list
**	NOTE:
**	(1)	This uses the pWorkingCachedBlock pointer in the NCSFile,
**		which assumes that calls into this routine happen in an ascending order
**		most of the time, and that blocks can never be freed while they are needed
**		in a current SetView (so the pWorkingCachedBlock is always valid)
**	(2)	Assumes that the caller has mutexed the list in the case of
**		NCSECW_BLOCK_CANCEL/NCSECW_BLOCK_REQUEST.
**	(3) In the case of NCSECW_BLOCK_RETURN, the list is NOT mutexed, as we
**		want high performance on reads. This works because items are inserted
**		into the list / deleted from the list in a way that preserves
**		the link structure at all times for reads, even outside a mutex.
**		FINAL NOTE: NOW HAVING TO MUTEX FOR NCSECW_BLOCK_RETURN AS WELL
**	(4) The workingpoint pointers are really hints only. They
**		are at the File not FileView level, so multiple file views, running
**		at the same time, introduce inefficiencies. This should not be too
**		significant because (a) at worst it is no worse than starting from the start
**		and (b) most of the accesses (except block reads) are mutexed and (c)
**		the request blocks from server logic have their own working pointer.
**  (5)	IMPORTANT: The FREE BLOCK logic has to note if the working pointer is pointing
**		to a block being deleted, and if so, it must move the working pointers to new,
**		valid blocks. This works OK because the FREE BLOCK from cache only ever removed
**		blocks from cache if they are not currently in use by a file view.
********************************************************/

NCSFileCachedBlock *NCScbmGetCacheBlock(NCSFile *pNCSFile, NCSFileCachedBlock *pWorkingCachedBlock,
                                               NCSBlockId nBlock, NCSEcwBlockRequestMethod eRequest)
{
    NCSFileCachedBlock	*pBlock, *pPreviousBlock;

    // Find the block in the list, if there. Start looking from the pWorkingCached list for speed
    pBlock = pWorkingCachedBlock;

    if( pBlock ) {
        if( pBlock->nBlockNumber > nBlock )
            pBlock = pNCSFile->pFirstCachedBlock;	// gone too far in the list, so start again
    }
    else
        pBlock = pNCSFile->pFirstCachedBlock;

    // now scan the list looking for this block. We finish when:
    //(1)		The list is currently empty	(pBlock will be set to NULL)
    //(2)	or	The block has been found	(bBlock will point to the block, 
    //										 pPreviousBlock will point to previous block or NULL if start of the list)
    //(3)	or	The block is not there		(pBlock will point to the block before this one,
    //										 unless at start of list, in which case will be NULL)
    pPreviousBlock = NULL;

    while( pBlock ) {
        if(pBlock->nBlockNumber == nBlock )
            break;				// found the block
        if(pBlock->nBlockNumber > nBlock ) {
            if( pPreviousBlock ) {
                pBlock = pPreviousBlock;				// go to the previous block
                break;
            }
            else {
                pBlock = pNCSFile->pFirstCachedBlock;	// started too far down the list, so have to
                                                        // wrap back to the start
                if( pBlock->nBlockNumber > nBlock ) {
                    pBlock = NULL;						// if before start of list, return ptr as NULL
                    break;
                }
                pPreviousBlock = NULL;					// else start beginning of list again
                continue;
            }
        }
        pPreviousBlock = pBlock;
        pBlock = pBlock->pNextCachedBlock;
        if( pBlock && pPreviousBlock->nBlockNumber >= pBlock->nBlockNumber )
            return(NULL);			// SERIOUS ERROR - corrupted memory structure

        if( !pBlock ) {
            pBlock = pPreviousBlock;
            break;					// got to the end, and nBlock is > than largest block in the list
        }
    }

    // now process the type of request (return pointer, add block to cache, cancel block from cache)
    switch( eRequest ) {
    case NCSECW_BLOCK_RETURN :
            if( pBlock ) {
                if( pBlock->nBlockNumber == nBlock ) {
                    pBlock->nHitCount += 1;
                    return(pBlock);
                }
                else
                    return( NULL );					// ERROR - we could not find the requested block
            }
            return(NULL);
        break;
    case NCSECW_BLOCK_CANCEL :
        if( !pBlock )
            return( NULL );							// ERROR - should never happen
        if( pBlock->nBlockNumber != nBlock )
            return( NULL );							// ERROR - should never happen

        // Decrement usage count
        pBlock->nUsageCount -= 1;
        if( pBlock->nUsageCount == 0) {
            // indicate we have a block request to cancel. We leave the bRequested
            // set (the CANCEL packet will unset it) to flag this block as one that
            // was requested, that we now want to cancel the request for
            if( pBlock->bRequested ) {
                NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nCancelsXmitPending, 1);
                pNCSFile->nCancelsXmitPending += 1;
            }
            else if( !pBlock->pPackedECWBlock ) {
                pNCSFile->nRequestsXmitPending -= 1;			// No longer need the packet
                NCSEcwStatsDecrement(&pNCSEcwInfo->pStatistics->nRequestsXmitPending, 1);
            }
        }
        return(pBlock);
        break;
    case NCSECW_BLOCK_REQUEST : {
            NCSFileCachedBlock *pNewBlock;

            if( pBlock ) {
                if( pBlock->nBlockNumber == nBlock ) {
                    // block is already in the list. Just increment usage count
                    NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nSetViewBlocksCacheHits, 1);
                    
                    pBlock->nUsageCount += 1;
                    // If usage gone to 1, and no block in memory, and request not
                    // already in progress, request it by incrementing Xmit pending requests by 1
                    // There also exists the situation where (1) a request went out (2) the
                    // request was flagged as to be canceled (3) the block is re-requested before
                    // the cancel could be xmitted. In this case, we have to decrement the cancel
                    // request count.
                    if( (pBlock->nUsageCount == 1) && !pBlock->pPackedECWBlock ) {
                        if( pBlock->bRequested ) {
                            pNCSFile->nCancelsXmitPending -= 1;		// cancel the cancel operation
                            NCSEcwStatsDecrement(&pNCSEcwInfo->pStatistics->nCancelsXmitPending, 1);
                        }
                        else {
                            pNCSFile->nRequestsXmitPending += 1;	// request the packet
                            NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nRequestsXmitPending, 1);
                        }
                    }

                    return(pBlock);
                }
            }

            // else allocate a new block
            pNewBlock = (NCSFileCachedBlock*)NCSPoolAlloc(pNCSFile->pBlockCachePool, TRUE);
            if( !pNewBlock )
                return( NULL );						// ERROR - out of memory
            pNewBlock->nBlockNumber = nBlock;
            pNewBlock->nUsageCount = 1;
            pNewBlock->bRequested = FALSE;
            // make sure we add the item carefully, so READS won't break without Mutex's
            if( pBlock ) {
                pNewBlock->pNextCachedBlock = pBlock->pNextCachedBlock;
                pBlock->pNextCachedBlock = pNewBlock;
            }
            else {
                    // insert at start of list. If list is NULL, sets pNextCachedBlock to NULL
                    pNewBlock->pNextCachedBlock = pNCSFile->pFirstCachedBlock;
                    pNCSFile->pFirstCachedBlock = pNewBlock;
            }
            if( 1 ) {
                UINT8 *pPackedECWBlock;
                UINT nPackedECWBlockLength;

                pPackedECWBlock = NCScbmReadFileBlockLocal_ECW(pNCSFile, nBlock, &nPackedECWBlockLength );
                if( pPackedECWBlock && nPackedECWBlockLength != 0 && align_ecw_block(pNCSFile, nBlock, 
                                                                                   &pNewBlock->pPackedECWBlock, 
                                                                                   &pNewBlock->nPackedECWBlockLength, 
                                                                                   pPackedECWBlock, 
                                                                                   nPackedECWBlockLength) == 0) {
                    NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nPackedBlocksCacheSize, pNewBlock->nPackedECWBlockLength);
                }
                
                NCSFree(pPackedECWBlock);//[19]
                
                if(nPackedECWBlockLength != 0) {
                    pNewBlock->pUnpackedECWBlock = NULL;
                    pNewBlock->nUnpackedECWBlockLength = 0;
                } else {
                    NCSPoolFree(pNCSFile->pBlockCachePool, pNewBlock);
                    pNewBlock = NULL;
                }
            }
            NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nSetViewBlocksCacheMisses, 1);

            return(pNewBlock);
        }
        break;
    }

    return( NULL);
}

/*******************************************************
**	NCScbmReadViewBlock()	- Read a block for the view
**
**	Notes:
**	-	If block is not available, and view cachemode is
**		NCS_CACHE_VIEW, a fake "zero" block is returned,
**		otherwise reading will wait (blocking IO) until
**		the block becomes available.
**	-	If error, a NULL pointer is returned
**	-	You MUST call NCScbmFreeViewBlock() to finish with
**		using the block - don't try freeing it yourself!
********************************************************/
UINT8	*NCScbmReadViewBlock(QmfRegionLevelStruct	*pQmfRegionLevel,
                      UINT nBlockX, UINT nBlockY)
{
    UINT	nBlock;
    UINT8	*pECWBlock = NULL;

    QmfRegionStruct *pRegion = pQmfRegionLevel->p_region;
    QmfLevelStruct	*pQmfLevel = pQmfRegionLevel->p_qmf;
    NCSFileView		*pNCSFileView = pRegion->pNCSFileView;
    NCSFile			*pNCSFile = pNCSFileView->pNCSFile;
    NCSFileCachedBlock	*pNCSBlock;

    nBlock = pQmfLevel->nFirstBlockNumber + (pQmfLevel->nr_x_blocks * nBlockY) + nBlockX;

    // Reading from cache.
    // WARNING!! There could be a problem with this code given it allocates
    // a new ECW block, if multiple threads are running through the file at the
    // same time.  This code most likely has to be mutexed. The WorkingCachedBlock
    // is OK - it is intended as a hint pointer, and multiple threads will make
    // it not as efficient anyway. The main problem is the allocation of the packed block.
    // For this reason, despite MUTEX's being slow, we have to mutex the block read.
    // Sigh.  A possible improvement might be to only MUTEX if there is more than file view
    // currently open on the file.
    // [03] Latest version: now only mutex's if adding an unpacked block

    if( pNCSFileView->nCacheMethod == NCS_CACHE_VIEW ) {
        UINT nReadUnpackedBlocksCacheHits = 0;
        UINT nUnpackedBlocksCacheSize = 0;

        pNCSBlock = pNCSFile->pWorkingCachedBlock = 
            NCScbmGetCacheBlock(pNCSFileView->pNCSFile,
                                pNCSFile->pWorkingCachedBlock, nBlock, NCSECW_BLOCK_RETURN);

        if( !pNCSBlock ) {
            // ERROR - internal logic has broken, as this should never happen
            // So just try and return a zero-block instead
            NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nReadBlocksCacheMisses, 1);

            pNCSFileView->info.nMissedBlocksDuringRead += 1;		// [02]
            _ASSERT( pNCSBlock );
            if( pQmfLevel->level )
                return( pNCSFileView->pNCSFile->pLevelnZeroBlock );
            else
                return( pNCSFileView->pNCSFile->pLevel0ZeroBlock );
        }

        pECWBlock = pNCSBlock->pUnpackedECWBlock;
        if( pECWBlock ) {
                // unpacked block cache hit
            nReadUnpackedBlocksCacheHits += 1;
        }
        else {
            // Unpack the block into cache. The unpack might fail, in which case we still
            // use the packed block. First, we make sure our cache size has not grown too large -
            // if so, we use the packed block size and don't attempt an unpack

            pECWBlock = pNCSBlock->pPackedECWBlock;		 // [02] try the unpack if the block is there
            /** [24] Change to caching logic, see inequality below */
            if( pECWBlock && (pNCSEcwInfo->pStatistics->nUnpackedBlocksCacheSize + pNCSEcwInfo->pStatistics->nPackedBlocksCacheSize)
                   < pNCSEcwInfo->pStatistics->nMaximumCacheSize /** [24] ((pNCSEcwInfo->pStatistics->nMaximumCacheSize / 3)*2)*/) {
                // we have a packed block, and enough RAM to be OK to unpack it
                UINT8	*pUnpackedECWBlock = NULL;
                UINT	nUnpackedLength = 0;

                // [03] now ONLY mutex when adding unpacked blocks. We now mutex here, and
                // [03] then if the unpacked block is still not there, we can add it in,
                // [03] otherwise just grab the unpacked block that popped up since we did the
                // [03] unpacked block test a few lines ago

                if( pNCSBlock->pUnpackedECWBlock ) {			// [03]
                    // [03] the unpacked block came into being after our out-of-mutex test, so use it
                    pECWBlock = pNCSBlock->pUnpackedECWBlock;	// [03]
                    nReadUnpackedBlocksCacheHits += 1;
                }												// [03]
                else {
                    //__try {
                        if( unpack_ecw_block(pQmfLevel, nBlockX, nBlockY, &pUnpackedECWBlock, &nUnpackedLength,pECWBlock) == 0 ) {
                            pECWBlock = pUnpackedECWBlock;
                            pNCSBlock->pUnpackedECWBlock = pUnpackedECWBlock;
                            pNCSBlock->nUnpackedECWBlockLength = nUnpackedLength;
                            nUnpackedBlocksCacheSize += nUnpackedLength;
                        }
                    /*} __except (EXCEPTION_EXECUTE_HANDLER) {
                        pECWBlock = NULL;
                        pNCSBlock->pUnpackedECWBlock = NULL;
                        pNCSBlock->nUnpackedECWBlockLength = 0;

                        if(!pNCSFile->bIsCorrupt) {
                            pNCSFile->bIsCorrupt = TRUE;
                        }
                    }*/
                }
            }
        }

        if( pECWBlock ) {
            // there was a block already loaded
            NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nReadBlocksCacheHits, 1);
            NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nReadUnpackedBlocksCacheHits, nReadUnpackedBlocksCacheHits);
            NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nUnpackedBlocksCacheSize, nUnpackedBlocksCacheSize);
        }
        else {
            // If no ECW block loaded yet, return a fake ZERO block
            NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nReadBlocksCacheMisses, 1);
            NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nReadUnpackedBlocksCacheHits, nReadUnpackedBlocksCacheHits);
            NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nUnpackedBlocksCacheSize, nUnpackedBlocksCacheSize);

            pNCSFileView->info.nMissedBlocksDuringRead += 1;			// [02]
            if( pQmfLevel->level )
                return( pNCSFileView->pNCSFile->pLevelnZeroBlock );
            else
                return( pNCSFileView->pNCSFile->pLevel0ZeroBlock );
        }
        return(pECWBlock);
    }

    // Read block directly from local file, not cache
    NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nReadBlocksCacheBypass, 1);

    {
        UINT	nBlockLength = 0;
        UINT8 *pPackedBlock = NCScbmReadFileBlockLocal_ECW(pNCSFileView->pNCSFile, nBlock, &nBlockLength );
        UINT8 *pAlignedBlock = (UINT8*)NULL;
        UINT nAlignedLength = 0;

        if(pPackedBlock && nBlockLength/**[23]**/) {
            align_ecw_block(pNCSFile, nBlock, &pAlignedBlock, &nAlignedLength, pPackedBlock, nBlockLength);
            NCSFree(pPackedBlock);
        }
        return(pAlignedBlock);
    }
}

/*******************************************************
**	NCScbmFreeViewBlock() - Request freeing a block.
**	This routine will ONLY free the block if the current
**	NCSFileView is DONT cache, in which case the block
**	is immediatelly freed. Otherwise, it is not freed -
**	the SetView() logic will later free the block at the
**	termination of the SetView() if the overall block
**	usage has gone to zero.
**
**	IMPORTANT!! This is the ECWBlock being freed here,
**	not the pNCSBlock, which can potentially point to
**	a packed *and* an unpacked ECW block. This all works
**	because this free routine is really only here for
**	freeing non-cached blocks - cached blocks never get
**	free for a view until that view shuts down.
********************************************************/

void NCScbmFreeViewBlock(QmfRegionLevelStruct	*pQmfRegionLevel, UINT8 *pECWBlock)
{

    QmfRegionStruct *pRegion = pQmfRegionLevel->p_region;
    NCSFileView		*pNCSFileView = pRegion->pNCSFileView;

    if( pECWBlock ) {
        if( !pNCSFileView )
            NCSFree(pECWBlock);		// should never happen, but just in case someone uses the old ECW calls
        else if( pNCSFileView->nCacheMethod == NCS_CACHE_DONT )
            NCSFree(pECWBlock);		// the ECW block bypassed the cache system, so free it
    }
}

/*******************************************************
**	NCScbmReadFileBlockLocal()	- Read a physical block for the file
**
**	Notes:
**	-	Always reads the requested block from the local file
********************************************************/
UINT8	*NCScbmReadFileBlockLocal_ECW(NCSFile *pNCSFile, NCSBlockId nBlock, UINT *pBlockLength )
{
    UINT8	*pECWBlock = NULL;
    UINT64	offset = 0;
    UINT	length = 0;

    if(NCScbmGetFileBlockSizeLocal_ECW(pNCSFile, nBlock, &length, &offset)) {
        QmfLevelStruct	*pTopQmf = pNCSFile->pTopQmf;		// we always go relative to the top level
        UINT  nPaddedLength = 1;
#ifdef POSIX
        /**[25]**/
        // align nPaddedLength on a word boundary
        // change submitted by Bill Binko 
        if (length % 4 == 0)
        {
            nPaddedLength = length;
        }
        else
        {
            nPaddedLength = (((int)(length/4)) + 1)*4;
        }
        *pBlockLength = length;
#else		
        while(nPaddedLength < length) {
            nPaddedLength *= 2;
        }
        *pBlockLength = length;
#endif

        pECWBlock = (UINT8*)NCSMalloc((UINT)nPaddedLength /*length*/, FALSE);

        if( !pECWBlock ) {
            return( NULL ); 
        }

        if( EcwFileSetPos(pTopQmf->hEcwFile, offset) ) {
            //[20] start...
            pNCSFile->bFileIOError = TRUE;
            //[20] ...end
            NCSFree(pECWBlock);//[19]
            return( NULL );
        }

        if( EcwFileRead(pTopQmf->hEcwFile, pECWBlock, length) ) {
            //[20] start...
            pNCSFile->bFileIOError = TRUE;
            //[20] ...end
            NCSFree(pECWBlock);//[19]
            return(NULL);
        }
    } 

    return(pECWBlock);
}

BOOLEAN NCScbmGetFileBlockSizeLocal_ECW(NCSFile *pNCSFile, NCSBlockId nBlock, UINT *pBlockLength, UINT64 *pBlockOffset )
{
    UINT64	offset = 0;
    UINT	length = 0;

    QmfLevelStruct	*pTopQmf = pNCSFile->pTopQmf;		// we always go relative to the top level


    // Read block directly from local file, not cache
    if(pTopQmf->p_block_offsets) {
        UINT64	*p_block_offset = pTopQmf->p_block_offsets + nBlock;	// will go up to higher levels if need be
        offset = *p_block_offset++;							// get offset to block
        length = (UINT) (*p_block_offset - offset);		// get length
        offset += pTopQmf->file_offset;// add offset to first block in file
    } else if(pTopQmf->bRawBlockTable) {
        INT i;
        
        for(i = 0; i < (INT)pNCSFile->nOffsetCache; i++) {
            if(pNCSFile->pOffsetCache[i].nID == nBlock) {
                pNCSFile->pOffsetCache[i].tsLastUsed = NCSGetTimeStampMs();
                length = pNCSFile->pOffsetCache[i].nLength;
                offset = pNCSFile->pOffsetCache[i].nOffset;
                break;
            }
        }
        if(length == 0 || offset == 0) {
            UINT64 offset2;
            EcwFileSetPos(pTopQmf->hEcwFile, pTopQmf->nHeaderMemImageLen + sizeof(UINT) + sizeof(UINT8) + sizeof(UINT64) * nBlock);
        //[22]	NCSFileReadUINT64_LSB(pTopQmf->hEcwFile.hFile, &offset);
        //[22]	NCSFileReadUINT64_LSB(pTopQmf->hEcwFile.hFile, &offset2);
            EcwFileRead(pTopQmf->hEcwFile, &offset, sizeof(offset));		//[22]
            EcwFileRead(pTopQmf->hEcwFile, &offset2, sizeof(offset2));		//[22]
#ifdef NCSBO_MSBFIRST														//[22]
            offset = NCSByteSwap64(offset);									//[22]
            offset2 = NCSByteSwap64(offset2);								//[22]
#endif																		//[22]
            length = (UINT)(offset2 - offset);
            offset += pTopQmf->file_offset;

            if(pNCSFile->nOffsetCache < pNCSEcwInfo->nMaxOffsetCache) {
                NCSFileBlockOffsetEntry Entry;
                Entry.nID = nBlock;
                Entry.nLength = length;
                Entry.nOffset = offset;
                Entry.tsLastUsed = NCSGetTimeStampMs();
                NCSArrayAppendElement(NCSFileBlockOffsetEntry, pNCSFile->pOffsetCache, pNCSFile->nOffsetCache, &Entry);
            } else {
                INT nLFU = 0;
                for(i = 0; i < (INT)pNCSFile->nOffsetCache; i++) {
                    if((pNCSFile->pOffsetCache[i].tsLastUsed < pNCSFile->pOffsetCache[nLFU].tsLastUsed) ||
                       ((pNCSFile->pOffsetCache[i].tsLastUsed == pNCSFile->pOffsetCache[nLFU].tsLastUsed) && 
                        (pNCSFile->pOffsetCache[i].nID > pNCSFile->pOffsetCache[nLFU].nID))) {
                        nLFU = i;
                    }
                }
                pNCSFile->pOffsetCache[nLFU].nID = nBlock;
                pNCSFile->pOffsetCache[nLFU].nLength = length;
                pNCSFile->pOffsetCache[nLFU].nOffset = offset;
                pNCSFile->pOffsetCache[nLFU].tsLastUsed = NCSGetTimeStampMs();
            }
        }
    }
    if(pBlockLength) {
        *pBlockLength = length;
    }
    if(pBlockOffset) {
        *pBlockOffset = offset;
    }


    return(TRUE);
}

/******************************************************
**	NCScbmReadViewLineRGB() - read a block from an ECW file in RGB format
** Returns NCSECW_READ_OK if no error on the read.
********************************************************/
NCSEcwReadStatus NCScbmReadViewLineRGBA_ECW( NCSFileView *pNCSFileView, UINT *pRGBTriplets)
{
    if( erw_decompress_read_region_line(pNCSFileView->pQmfRegion, (UINT8 *)pRGBTriplets) )
        return(NCSECW_READ_FAILED);
    else
        return(NCSECW_READ_OK);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// ncscbmopen.c
//

int NCScbmFileViewRequestBlocks(NCSFileView *pNCSFileView, QmfRegionStruct *pQmfRegion, NCSEcwBlockRequestMethod eRequest);
UINT8 *NCScbmConstructZeroBlock(QmfLevelStruct *p_qmf, UINT *pLength);

/*******************************************************
**	NCScbmOpenFileView() -	Opens a file view
**
**	Notes:
**	-	File could be local or remote
**	-	Blocks until file can be opened
**	-	The RefreshCallback can be NULL or a client refresh
**		callback function to call when blocks have arrived
**	Special notes for the RefreshCallback:
**	(1)	You must set this for each FileView that you want to have refresh
**		calls received for.  If you don't set it, the FileView
**		will instead revert to the blocking timer when blocks are
**		requested.
**
**	Returns:	NCSError (cast to int).	FIXME: change return type to NCSError, along 
**				with return types for all SDK public funcs, and ship NCSError.h in SDK
**
********************************************************/

NCSError NCScbmOpenFileView_ECW(char *szUrlPath, NCSFileView **ppNCSFileView)
{
    NCSFile	*pNCSFile;
    NCSFileView *pNCSFileView;
    NCSError nError;

    if (!pNCSEcwInfo) {
        NCSecwInitInternal();
    }

    // Lock to add File View list
    *ppNCSFileView = NULL;			// [02] make sure is NULL in case of error

    nError = NCSecwOpenFile(&pNCSFile, szUrlPath, TRUE, TRUE);

    if( nError == NCS_SUCCESS ) {

        pNCSFileView = (NCSFileView *) NCSMalloc( sizeof(NCSFileView), FALSE);
        if( pNCSFileView ) {
            // If no client block caching memory pool allocated yet, allocate it, and the zero blocks

            if( !pNCSFile->pBlockCachePool ) {
                // create the memory block pool
                pNCSFile->pBlockCachePool = NCSPoolCreate(sizeof(NCSFileCachedBlock), NCSECW_CACHED_BLOCK_POOL_SIZE);
            } 
            if(!pNCSFile->pLevel0ZeroBlock) {
                // create the fake 0zero blocks
                pNCSFile->pLevel0ZeroBlock = NCScbmConstructZeroBlock(pNCSFile->pTopQmf, NULL);
            }
            if(!pNCSFile->pLevelnZeroBlock) {
                // create the fake nzero blocks
                pNCSFile->pLevelnZeroBlock = NCScbmConstructZeroBlock(pNCSFile->pTopQmf->p_larger_qmf, NULL);
            }
            
            if( pNCSFile->pBlockCachePool && pNCSFile->pLevel0ZeroBlock && pNCSFile->pLevelnZeroBlock) {

                NCSEcwStatsIncrement(&pNCSEcwInfo->pStatistics->nFileViewsOpen, 1);

                // caching methodology not determined until the first setview
                pNCSFileView->eCallbackState = NCSECW_VIEW_QUIET;
                pNCSFileView->nCacheMethod = NCS_CACHE_INVALID;
                pNCSFileView->bIsRefreshView = FALSE;

                pNCSFileView->pPrevNCSFileView = NULL;
                pNCSFileView->pQmfRegion = NULL;
                pNCSFileView->pNCSFile = pNCSFile;

                pNCSFileView->info.nBlocksInView = 0;				// total number of blocks that cover the view area
                pNCSFileView->info.nBlocksAvailable = 0;				// current number of blocks available at this instant
                pNCSFileView->info.nBlocksAvailableAtSetView = 0;	// Blocks that were available at the time of the SetView
                pNCSFileView->info.nMissedBlocksDuringRead = 0;

                pNCSFileView->info.pClientData = NULL;
                pNCSFileView->info.nTopX	= pNCSFileView->info.nLeftY = 0;
                pNCSFileView->info.nBottomX = pNCSFileView->info.nRightY = 0;
                pNCSFileView->info.nSizeX	= pNCSFileView->info.nSizeY = 0;
                pNCSFileView->info.fTopX	= pNCSFileView->info.fLeftY = 0.0;
                pNCSFileView->info.fBottomX	= pNCSFileView->info.fRightY = 0.0;
                pNCSFileView->info.nBands	= 0;					// [02]
                // [02] we keep our own master BandList array, and copy into that list from,
                //		the SetView calls. This way we don't have to track the client bandlist
                pNCSFileView->info.pBandList = (UINT*)NCSMalloc( sizeof(UINT) * pNCSFile->pTopQmf->p_file_qmf->nr_bands, FALSE);
                if(pNCSFileView->info.pBandList) {

                    pNCSFileView->pending.pBandList = (UINT*)NCSMalloc( sizeof(UINT) * pNCSFile->pTopQmf->p_file_qmf->nr_bands, FALSE);
                    if(pNCSFileView->pending.pBandList) {

                        pNCSFileView->nPending = 0;
                        pNCSFileView->nCancelled = 0;

                        pNCSFileView->nNextDecodeMissID = 0;	/**[09]**/

                        pNCSFileView->pNextNCSFileView = pNCSFile->pNCSFileViewList;
                        if( pNCSFile->pNCSFileViewList )
                            pNCSFile->pNCSFileViewList->pPrevNCSFileView = pNCSFileView;
                        pNCSFile->pNCSFileViewList = pNCSFileView;

                        *ppNCSFileView = pNCSFileView;

                        return NCS_SUCCESS;		
                    } else {
                        nError = NCS_COULDNT_ALLOC_MEMORY;
                    }
                    NCSFree(pNCSFileView->info.pBandList);
                } else {
                    nError = NCS_COULDNT_ALLOC_MEMORY;
                }
            } else {
                nError = NCS_FILE_NO_MEMORY;
            }
            NCSFree(pNCSFileView);
        } else {
            nError = NCS_FILE_NO_MEMORY;
        }
        NCSecwCloseFile(pNCSFile);
    } 
    
    return nError;
}


/*******************************************************
**	NCSecwConstructZeroBlock()	- Allocates and returns a zero-block
**
**	Notes:
********************************************************/
 
UINT8 *NCScbmConstructZeroBlock(QmfLevelStruct *p_qmf,UINT *pLength)
{
    UINT	nSidebands;
    UINT8	*pZeroBlock, *pZeroBlockSideband;
    UINT8	*pZeroBlock32;
    UINT nLength = 0;

    if( p_qmf->level )
        nSidebands = p_qmf->nr_sidebands - 1;
    else
        nSidebands = p_qmf->nr_sidebands;
    nSidebands = nSidebands * p_qmf->nr_bands;
    // we need room for N-1 UINT's of sidebands, and N bytes of compression (zero block flags)
    nLength = (sizeof(UINT) * (nSidebands-1)) + nSidebands  * sizeof(EncodeFormat);
    pZeroBlock = (UINT8*)NCSMalloc(nLength , FALSE);/**[13]**/
    if(pLength) {
        *pLength = nLength;
    }
    if( !pZeroBlock )
        return( NULL );
    pZeroBlock32 = pZeroBlock;
    pZeroBlockSideband = pZeroBlock + (sizeof(UINT) * (nSidebands - 1));
    *((EncodeFormat*)pZeroBlockSideband) = ENCODE_ZEROS;	// one more entry than offsets [13]
    pZeroBlockSideband += sizeof(EncodeFormat); /**[13]**/

    while(--nSidebands) {
        *pZeroBlock32++ = 0;	// 0xFF000000
        *pZeroBlock32++ = 0;	// 0x00FF0000
        *pZeroBlock32++ = 0;	// 0x0000FF00
        *pZeroBlock32++ = sizeof(EncodeFormat);	// 0x000000FF [13]

        *((EncodeFormat*)pZeroBlockSideband) = ENCODE_ZEROS;	/**[13]**/
        pZeroBlockSideband += sizeof(EncodeFormat);				/**[13]**/
        //*pZeroBlockSideband++ = ENCODE_ZEROS; [13]
    }
    return(pZeroBlock);
}


/*******************************************************
**	NCScbmCloseFileView() -	Closes a previously opened file view
**
**	Notes:
**	-	File could be local or remote
**	-	Blocks until file can be opened
********************************************************/

NCSError NCScbmCloseFileViewEx_ECW(NCSFileView *pNCSFileView,			/**[07]**/ /**[11]**/
                          BOOLEAN bFreeCachedFile)				/**[07]**/
{
    NCSFile *pNCSFile;

    if (!pNCSEcwInfo) {
        NCSecwInitInternal();
    }
 
    if( pNCSFileView ) {					//[14] Check if files are already closed
        pNCSFile = pNCSFileView->pNCSFile;

        NCSEcwStatsDecrement(&pNCSEcwInfo->pStatistics->nFileViewsOpen, 1);

        NCScbmCloseFileViewCompletely(&(pNCSFile->pNCSFileViewList), pNCSFileView);

        if((pNCSFile->nUsageCount == 1) && bFreeCachedFile) {	/**[07]**/
            pNCSFile->bValid = FALSE;							/**[07]**/
        }														/**[07]**/
        NCSecwCloseFile(pNCSFile);
    }

    return(NCS_SUCCESS);		// FileView was closed
}

NCSError	NCScbmCloseFileView_ECW(NCSFileView *pNCSFileView) /**[11]**/
{
    return(NCScbmCloseFileViewEx_ECW(pNCSFileView, FALSE));	/**[07]**/
}

/*
**	Flush file view from cache. Does NOT mutex - caller must do the mutex
*/

int	NCScbmCloseFileViewCompletely(NCSFileView **ppNCSFileViewList, NCSFileView *pNCSFileView)
{

    if(pNCSFileView->pQmfRegion) {
        // Mark blocks as no longer in use
        if(pNCSFileView->nCacheMethod == NCS_CACHE_VIEW)
            NCScbmFileViewRequestBlocks(pNCSFileView, pNCSFileView->pQmfRegion, NCSECW_BLOCK_CANCEL);
        // shut down the view
        erw_decompress_end_region(pNCSFileView->pQmfRegion);
        pNCSFileView->pQmfRegion = NULL;
    }

    if(pNCSFileView->info.pBandList) {	/**[06]**/
        NCSFree(pNCSFileView->info.pBandList);			// [02] we keep a local band list for the view
        pNCSFileView->info.pBandList = NULL;
    }
    if(pNCSFileView->pending.pBandList) {	/**[06]**/
        NCSFree(pNCSFileView->pending.pBandList);		// [02] and for the pending view
        pNCSFileView->pending.pBandList = NULL;
    }
        
    // Remove this file view from the view List
    if( *ppNCSFileViewList == pNCSFileView )
        *ppNCSFileViewList = pNCSFileView->pNextNCSFileView;
    if( pNCSFileView->pNextNCSFileView )
        pNCSFileView->pNextNCSFileView->pPrevNCSFileView = pNCSFileView->pPrevNCSFileView;
    if( pNCSFileView->pPrevNCSFileView )
        pNCSFileView->pPrevNCSFileView->pNextNCSFileView = pNCSFileView->pNextNCSFileView;
    NCSFree(pNCSFileView);
    return(0);
}

/*******************************************************
**	NCScbmGetViewFileInfo_ECW() -	Returns the information about a file opened using OpenView
**
**	Notes:
**	-	Just returns information about the file, not the current SetView in this view into that file
********************************************************/
NCSError NCScbmGetViewFileInfo_ECW(NCSFileView *pNCSFileView, NCSFileViewFileInfo **ppNCSFileViewFileInfo) /**[11]**/
{
    if( !pNCSFileView || !pNCSFileView->pNCSFile || !pNCSFileView->pNCSFile->pTopQmf ) {
        *ppNCSFileViewFileInfo = NULL;
        return(NCS_INVALID_PARAMETER); /*[15]*/
    }
    *ppNCSFileViewFileInfo = (NCSFileViewFileInfo *) pNCSFileView->pNCSFile->pTopQmf->pFileInfo;
    return(NCS_SUCCESS);
}

/*******************************************************
**	NCScbmGetViewFileInfoEx() -	Returns the information about a file opened using OpenView
**
**	Notes:
**	-	Just returns information about the file, not the current SetView in this view into that file
********************************************************/
NCSError NCScbmGetViewFileInfoEx_ECW(NCSFileView *pNCSFileView, NCSFileViewFileInfoEx **ppNCSFileViewFileInfo) /**[11]**/
{
    if( !pNCSFileView || !pNCSFileView->pNCSFile || !pNCSFileView->pNCSFile->pTopQmf ) {
        *ppNCSFileViewFileInfo = NULL;
        return(NCS_INVALID_PARAMETER); /*[15]*/
    }
    *ppNCSFileViewFileInfo = (NCSFileViewFileInfoEx *) pNCSFileView->pNCSFile->pTopQmf->pFileInfo;
    return(NCS_SUCCESS);
}

/*******************************************************
**	NCScbmGetViewFileInfo_ECW() -	Returns the information about the current SetView
**
**	Notes:
**	-	Only use this inside a callback, to determine which SetView is currently being processed
********************************************************/
NCSError NCScbmGetViewInfo_ECW(NCSFileView *pNCSFileView, NCSFileViewSetInfo **ppNCSFileViewSetInfo) /**[11]**/
{
    *ppNCSFileViewSetInfo = &(pNCSFileView->info);
    return(NCS_SUCCESS);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// qmf_util.c
//

QmfLevelStruct *new_qmf_level(UINT nBlockSizeX, UINT nBlockSizeY,
                              UINT16 level, UINT x_size, UINT y_size, UINT nr_bands,
                    QmfLevelStruct *p_smaller_qmf, QmfLevelStruct *p_larger_qmf)
{

    QmfLevelStruct *p_qmf;
    UINT16		band;

    p_qmf = (QmfLevelStruct *) NCSMalloc(sizeof(QmfLevelStruct), TRUE);
    if( p_qmf ) {

        p_qmf->version = ERSWAVE_VERSION;
        p_qmf->nr_sidebands = MAX_SIDEBAND;
        p_qmf->blocking_format = BLOCKING_LEVEL;
        p_qmf->compress_format = COMPRESS_UINT8;

        p_qmf->level = level;
        p_qmf->x_size = x_size;
        p_qmf->y_size = y_size;
        p_qmf->nr_bands = (UINT16)nr_bands;
        p_qmf->x_block_size = (UINT16)nBlockSizeX;
        p_qmf->y_block_size = (UINT16)nBlockSizeY;
        p_qmf->nr_x_blocks = QMF_LEVEL_NR_X_BLOCKS(p_qmf);
        p_qmf->nr_y_blocks = QMF_LEVEL_NR_Y_BLOCKS(p_qmf);
        p_qmf->tmp_file = NCS_NULL_FILE_HANDLE;
        {
            ECWFILE null_file = NULL_ECWFILE;
            p_qmf->hEcwFile = null_file;			// [05] Input ECW file pointer (note - NULL might be a valid Windows handle (!))
        }
        p_qmf->p_band_bin_size = (UINT *) NCSMalloc( nr_bands * sizeof(UINT), FALSE);
        if( !p_qmf->p_band_bin_size ) {
            NCSFree((char *) p_qmf);
            return(NULL);
        }
        for(band=0; band < nr_bands; band++) {
            p_qmf->p_band_bin_size[band] = 1;		// no bin size defined yet
        }

        if( p_smaller_qmf ) {
            p_smaller_qmf->p_larger_qmf = p_qmf;
            p_qmf->p_smaller_qmf = p_smaller_qmf;
        }

        if( p_larger_qmf ) {
            p_larger_qmf->p_smaller_qmf = p_qmf;
            p_qmf->p_larger_qmf = p_larger_qmf;
        }
    }
    return(p_qmf);
}

/**************************************************************************/

// Allocate the qmf buffers for all levels. This must be done
// after the qmf level structures have been allocated for all
// levels. Only enough buffers are allocated at each level
// to hold the working lines needed for that level.
// TOP QMF is always the smallest QMF
//
// Output generation:
//	To generate a single set of sideband (LL,LH,HH and LL) lines,
//	enough LOWPASS and HIGHPASS lines are needed to cover the size
//	of the filter bank.  A set of (1 of each) LOWPASS and HIGHPASS lines
//	is generated by sampling a single INPUT line multiplied by a horizontal
//	filter tap.  The INPUT is sampled every second value for LOWPASS and HIGHPASS,
//	with the HIGHPASS offset by 1 across in the sampling.  As this is a horizontal sampling,
//	so we only need one INPUT line to generate a pair (1 of each) of
//	LOWPASS and HIGHPASS lines.  So LOWPASS uses INPUT [0,2,4,...]
//	and HIGHPASS uses INPUT [1,3,5,...]
//
//	A set (1 of each) LL, LH, HL and HH lines is generated by sampling FILTER_TAP+1
//	LOWPASS and HIGHPASS lines.  We need FILTER_TAP+1 because (a) this is a vertical
//	sampling and (b) the HL and HH lines are offset by one down. So we need to keep
//	FILTER_SIZE+1 LOWPASS and HIGHPASS lines around for this level to generate
//	the LL, LH, HL and HH lines.
// 
//	LL and LH use input lines [0,2,5, .. ,FILTER_SIZE-1] and HL and HH use
//	input lines [1,3,5, ... ,FILTER_SIZE].
//	As the LOWPASS/HIGHPASS lines are used as input to vertical filters,
//	there is no left/right reflection on these, so they can be the size of
//	this level (which, note well, might be 1 more than (p_qmf->p_larger_qmf->x_size*2),
//	as we round up when defining the level sizes, in order to not lose data.
//
//	We also allocate enough quantized output lines for each sideband to be compressed,
//	for the number of y lines to be buffered before being written to output.
//
// Input generation:
//	A given QMF level will call the next larger QMF level, asking for an INPUT line
//	(which is actually the LL output from the called larger level).  As we are doing
//	horizontal filter taps on the INPUT line, we need to have left/right reflection.
//	Also, this level might be ((larger level size * 2)+1) in size.  So we can allocate
//	the INPUT line size to be:	((p_qmf->x_size) + FILTER_SIZE-1).
//	As the INPUT line buffer is allocated by the current level for requesting data from
//	a larger level, the largest level (the file), does not need one of these, as it
//	is never requesting input from a still larger level.
//
// Errors:
//	If we had an error (most likely out of memory), the calling routine should delete
//	the entire QMF tree, as it is in an undefined state (but safe to delete)
//

NCSError allocate_qmf_buffers(QmfLevelStruct *p_top_qmf)
{
    QmfLevelStruct	*p_qmf = p_top_qmf;
    UINT	nr_blocks;

    // Allocate top level items
    // allocate space for offsets for all blocks for all levels, at the top level only
    nr_blocks = get_qmf_tree_nr_blocks(p_top_qmf);

    // [02] set up block number offsets
    {
        UINT	nFirstBlockNumber = 0;
        while( p_qmf->p_larger_qmf ) {
            p_qmf->nFirstBlockNumber = nFirstBlockNumber;
            nFirstBlockNumber += (p_qmf->nr_x_blocks * p_qmf->nr_y_blocks);
            p_qmf = p_qmf->p_larger_qmf;
        }
        p_qmf = p_top_qmf;
    }

    // Allocate decompression specific items
    if( p_qmf->p_block_offsets ) {		// [03] block offsets table is now optional
        while(p_qmf) {
            if( p_qmf->p_larger_qmf ) {
                if( p_qmf != p_top_qmf ) {
                    p_qmf->p_a_block = p_top_qmf->p_a_block;
                    p_qmf->p_block_offsets = p_qmf->p_smaller_qmf->p_block_offsets + 
                        (p_qmf->p_smaller_qmf->nr_x_blocks * p_qmf->p_smaller_qmf->nr_y_blocks);
                }
            }
            p_qmf = p_qmf->p_larger_qmf;
        }
    }
    
    return(NCS_SUCCESS);
}

/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*
**	delete all qmf levels
*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/
void delete_qmf_levels(QmfLevelStruct *p_qmf )
{
    if( !p_qmf )
        return;

    if( p_qmf->p_top_qmf ) { 	// [03]
        // free the file memory image if present
        NCSFree( p_qmf->p_top_qmf->pHeaderMemImage );
        // [05] close file handle if it is open, and in decompress mode

        if( p_qmf->p_top_qmf->bEcwFileOpen ) {
            (void) EcwFileClose( p_qmf->p_top_qmf->hEcwFile );

            {
                ECWFILE null_file = NULL_ECWFILE;
                p_qmf->p_top_qmf->hEcwFile = null_file;			// [05] Input ECW file pointer (note - NULL might be a valid Windows handle (!))
            }

            p_qmf->p_top_qmf->bEcwFileOpen = FALSE;
        }
        
        // Free the FileInfo structures
        if( p_qmf->p_top_qmf->pFileInfo ) {
            NCSFreeFileInfoEx(p_qmf->p_top_qmf->pFileInfo);
            NCSFree( p_qmf->p_top_qmf->pFileInfo );
            p_qmf->p_top_qmf->pFileInfo = NULL;
        }
    }

    // start at one end
    while(p_qmf->p_larger_qmf)
        p_qmf = p_qmf->p_larger_qmf;

    /*
    **	At file level. Deallocate and close files
    */

    // and delete all layers
    while(p_qmf) {
        QmfLevelStruct *p_next_qmf = p_qmf->p_smaller_qmf;

        // Free data allocated at the top level but pointed to by larger levels
        if( !p_qmf->p_smaller_qmf ) {
            if( p_qmf->p_top_qmf->bRawBlockTable == TRUE ) {		//[14]
                if( p_qmf->p_block_offsets ) {
                    NCSFree(((char *) p_qmf->p_block_offsets)-sizeof(UINT64));		//[14]
                }
            }														//[14]
            else NCSFree((char *) p_qmf->p_block_offsets);		//[14]
            NCSFree((char *) p_qmf->p_a_block);
        }

        NCSFree((char *) p_qmf->p_band_bin_size);

        // free data held for each band for each level (this is compression only)
        if( p_qmf->p_bands ) {
            UINT16	band;
            for( band = 0; band < p_qmf->nr_bands; band++ ) {
                QmfLevelBandStruct *p_band = p_qmf->p_bands + band;
                int y;

                // don't deallocate the p_p_[lo|hi]__lines, just the block pointed to by them
                NCSFree((char *) p_band->p_low_high_block );
                NCSFree((char *) p_band->p_input_ll_line);

                NCSFree((char *) p_band->p_quantized_output_ll_block);
                NCSFree((char *) p_band->p_quantized_output_lh_block);
                NCSFree((char *) p_band->p_quantized_output_hl_block);
                NCSFree((char *) p_band->p_quantized_output_hh_block);

                
                for(y = 0; y < p_qmf->y_block_size; y++) {
                    if(p_band->p_p_ll_lengths) NCSFree(p_band->p_p_ll_lengths[y]);
                    if(p_band->p_p_p_ll_segs) NCSFree(p_band->p_p_p_ll_segs[y]);
                    if(p_band->p_p_lh_lengths) NCSFree(p_band->p_p_lh_lengths[y]);
                    if(p_band->p_p_p_lh_segs) NCSFree(p_band->p_p_p_lh_segs[y]);
                    if(p_band->p_p_hl_lengths) NCSFree(p_band->p_p_hl_lengths[y]);
                    if(p_band->p_p_p_hl_segs) NCSFree(p_band->p_p_p_hl_segs[y]);
                    if(p_band->p_p_hh_lengths) NCSFree(p_band->p_p_hh_lengths[y]);
                    if(p_band->p_p_p_hh_segs) NCSFree(p_band->p_p_p_hh_segs[y]);
                }
                NCSFree(p_band->p_p_ll_lengths);
                NCSFree(p_band->p_p_p_ll_segs);
                NCSFree(p_band->p_p_lh_lengths);
                NCSFree(p_band->p_p_p_lh_segs);		
                NCSFree(p_band->p_p_hl_lengths);
                NCSFree(p_band->p_p_p_hl_segs);			
                NCSFree(p_band->p_p_hh_lengths);
                NCSFree(p_band->p_p_p_hh_segs);
            
            }	/* end band loop */
            NCSFree((char *) p_qmf->p_bands);			/**[07]**/
        }

        // Free the index pointer that was pointing to the p_input_ll_line for each band
        NCSFree((char *) p_qmf->p_p_input_ll_line);

        // clean up temporary file
        if( p_qmf->tmp_file != NCS_NULL_FILE_HANDLE) {
            (void) NCSFileClose( p_qmf->tmp_file);
        }
        NCSFree((char *) p_qmf->tmp_fname);
        NCSFree(p_qmf);
        p_qmf = p_next_qmf;
    }
}

/*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*
**	Returns the number of blocks+1 in all levels of the QMF tree
**	(excluding file level). Assumes qmf_tree is correct - no error
**	checking is carried out.
**
**	NOTE: 1 more than the actual number of blocks is returned. This
**	is because we have to write one more at the end, indicating the
**	seek location to the end of the file, so it can be used to
**	compute offsets for the final block.
**
*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*	*/

UINT get_qmf_tree_nr_blocks( QmfLevelStruct *p_top_qmf )
{
    UINT nr_blocks;
    nr_blocks = 0;
    while(p_top_qmf->p_larger_qmf) {
        nr_blocks += (p_top_qmf->nr_x_blocks * p_top_qmf->nr_y_blocks);
        p_top_qmf = p_top_qmf->p_larger_qmf;
    }
    return(nr_blocks + 1);	// return one more than true number of blocks
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// unpack.c
//

typedef	uint2 qssymbol;	/* [01] use uint2 for 8 bit symbols, or uint4 for 16 bit symbols */

typedef struct {
    int n,             /* number of symbols */
        left,          /* symbols to next rescale */
        nextleft,      /* symbols with other increment */
        rescale,       /* intervals between rescales */
        targetrescale, /* should be interval between rescales */
        incr,          /* increment per update */
        searchshift;   /* shift for lt_freq before using as index */
        qssymbol *cf,	/*array of cumulative frequencies */
        *newf,			/* array for collecting ststistics */
        *search;		/* structure for searching on decompression */
} qsmodel;

/* initialisation of qsmodel                           */
/* m   qsmodel to be initialized                       */
/* n   number of symbols in that model                 */
/* lg_totf  base2 log of total frequency count         */
/* rescale  desired rescaling interval, should be < 1<<(lg_totf+1) */
/* init  array of int's to be used for initialisation (NULL ok) */
/* compress  set to 1 on compression, 0 on decompression */
static void initqsmodel( qsmodel *m, int n, int lg_totf, int rescale,
   int *init, int compress );

/* reinitialisation of qsmodel                         */
/* m   qsmodel to be initialized                       */
/* init  array of int's to be used for initialisation (NULL ok) */
static void resetqsmodel( qsmodel *m, int *init);


/* deletion of qsmodel m                               */
static void deleteqsmodel( qsmodel *m );


/* retrieval of estimated frequencies for a symbol     */
/* m   qsmodel to be questioned                        */
/* sym  symbol for which data is desired; must be <n   */
/* sy_f frequency of that symbol                       */
/* lt_f frequency of all smaller symbols together      */
/* the total frequency is 1<<lg_totf                   */
static __inline int qsgetfreq( qsmodel *m, int sym, int *lt_f );


/* find out symbol for a given cumulative frequency    */
/* m   qsmodel to be questioned                        */
/* lt_f  cumulative frequency                          */
static __inline int qsgetsym( qsmodel *m, int lt_f );


/* update model                                        */
/* m   qsmodel to be updated                           */
/* sym  symbol that occurred (must be <n from init)    */
static __inline void qsupdate( qsmodel *m, int sym );

typedef UINT code_value;       /* Type of an rangecode value       */
                                /* must accomodate 32 bits          */
/* it is highly recommended that the total frequency count is less  */
/* than 1 << 19 to minimize rounding effects.                       */
/* the total frequency count MUST be less than 1<<23                */

typedef UINT freq; 

/* make the following private in the arithcoder object in C++	    */

typedef struct {
    UINT low,           /* low end of interval */
           range,         /* length of interval */
           help;          /* bytes_to_follow resp. intermediate value */
    unsigned char buffer;/* buffer for input/output */
/* the following is used only when encoding */
    UINT bytecount;     /* counter for outputed bytes  */
/* insert fields you need for input/output below this line! */
/*	FILE *iofile;	*/	/* file to read or write */
    UINT8	*p_packed;	/* pointer for output / input to/from memory instead of disk */
} rangecoder;


/* Start the decoder                                         */
/* rc is the range coder to be used                          */
/* returns the char from start_encoding or EOF               */
static int start_decoding( rangecoder *rc );

/* Calculate culmulative frequency for next symbol. Does NO update!*/
/* rc is the range coder to be used                          */
/* tot_f is the total frequency                              */
/* or: totf is 1<<shift                                      */
/* returns the <= culmulative frequency                      */
static __inline freq decode_culfreq( rangecoder *rc, freq tot_f );
static __inline freq decode_culshift( rangecoder *ac, freq shift );

/* Update decoding state                                     */
/* rc is the range coder to be used                          */
/* sy_f is the interval length (frequency of the symbol)     */
/* lt_f is the lower end (frequency sum of < symbols)        */
/* tot_f is the total interval length (total frequency sum)  */
static __inline void decode_update( rangecoder *rc, freq sy_f, freq lt_f, freq tot_f);
#define decode_update_shift(rc,f1,f2,f3) decode_update((rc),(f1),(f2),(freq)1<<(f3));

/* Decode a byte/short without modelling                     */
/* rc is the range coder to be used                          */
static unsigned char decode_byte(rangecoder *rc);
static unsigned short decode_short(rangecoder *rc);


/* Finish decoding                                           */
/* rc is the range coder to be used                          */
static void done_decoding( rangecoder *rc );

#ifdef USE_SSE
#include <xmmintrin.h>
#endif // USE_SSE

/* SIZE OF RANGE ENCODING CODE VALUES. */

#define CODE_BITS 32
#define Top_value ((code_value)1 << (CODE_BITS-1))


/* all IO is done by these macros - change them if you want to */
/* no checking is done - do it here if you want it             */
/* cod is a pointer to the used rangecoder                     */
#ifdef NEVER_OLD_FILEIO
//	#define outbyte(cod,x) fputc(x,*(cod)->iofile)
//	#define inbyte(cod)    fgetc((cod)->iofile)
#endif
#define outbyte(cod,x) *(cod)->p_packed++ = (UINT8)(x);
#define inbyte(cod)    *(cod)->p_packed++;


#define SHIFT_BITS (CODE_BITS - 9)
#define EXTRA_BITS ((CODE_BITS-2) % 8 + 1)
#define Bottom_value (Top_value >> 8)

static char coderversion[]="rangecode 1.1 (c) 1997, 1998 Michael Schindler";/* Start the encoder                                           */

/* I do the normalization before I need a defined state instead of */
/* after messing it up. This simplifies starting and ending.       */
static __inline void enc_normalize( rangecoder *rc )
{   while(rc->range <= Bottom_value)     /* do we need renormalisation?  */
    {   if (rc->low < 0xff<<SHIFT_BITS)  /* no carry possible --> output */
        {   outbyte(rc,rc->buffer);
            for(; rc->help; rc->help--)
                outbyte(rc,0xff);
            rc->buffer = (unsigned char)(rc->low >> SHIFT_BITS);
        } else if (rc->low & Top_value) /* carry now, no future carry */
        {   outbyte(rc,rc->buffer+1);
            for(; rc->help; rc->help--)
                outbyte(rc,0);
            rc->buffer = (unsigned char)(rc->low >> SHIFT_BITS);
        } else                           /* passes on a potential carry */
            rc->help++;
        rc->range <<= 8;
        rc->low = (rc->low<<8) & (Top_value-1);
        rc->bytecount++;
    }
}

/* Start the decoder                                         */
/* rc is the range coder to be used                          */
/* returns the char from start_encoding or EOF               */
static int start_decoding( rangecoder *rc )
{   int c = inbyte(rc);
    if (c==EOF)
        return EOF;
    rc->buffer = inbyte(rc);
    rc->low = rc->buffer >> (8-EXTRA_BITS);
    rc->range = 1 << EXTRA_BITS;
    return c;
}

/* [04] converted to a macro for speed */
#define DEC_NORMALIZE( rc )												\
    while (rc->range <= Bottom_value) {									\
        rc->low = (rc->low<<8) | ((rc->buffer<<EXTRA_BITS)&0xff);		\
        rc->buffer = inbyte(rc);										\
        rc->low |= rc->buffer >> (8-EXTRA_BITS);						\
        rc->range <<= 8;												\
    }



/* Calculate culmulative frequency for next symbol. Does NO update!*/
/* rc is the range coder to be used                          */
/* tot_f is the total frequency                              */
/* or: totf is 1<<shift                                      */
/* returns the culmulative frequency                         */
static __inline freq decode_culfreq( rangecoder *rc, freq tot_f )
{   freq tmp;
    DEC_NORMALIZE(rc);
    rc->help = rc->range/tot_f;
    tmp = rc->low/rc->help;
    return (tmp>=tot_f ? tot_f-1 : tmp);
}

static __inline freq decode_culshift( rangecoder *rc, freq shift )
{   freq tmp;
    DEC_NORMALIZE(rc);
    rc->help = rc->range>>shift;
    tmp = rc->low/rc->help;
    return (tmp>>shift ? (1<<shift)-1 : tmp);
}


/* Update decoding state                                     */
/* rc is the range coder to be used                          */
/* sy_f is the interval length (frequency of the symbol)     */
/* lt_f is the lower end (frequency sum of < symbols)        */
/* tot_f is the total interval length (total frequency sum)  */
static __inline void decode_update( rangecoder *rc, freq sy_f, freq lt_f, freq tot_f)
{   code_value tmp;
    tmp = rc->help * lt_f;
    rc->low -= tmp;
    if (lt_f + sy_f < tot_f)
        rc->range = rc->help * sy_f;
    else
        rc->range -= tmp;
}


/* Decode a byte/short without modelling                     */
/* rc is the range coder to be used                          */
static unsigned char decode_byte(rangecoder *rc)
{   unsigned char tmp = (unsigned char)decode_culshift(rc,8);
    decode_update( rc,1,tmp,1<<8);
    return tmp;
}

static unsigned short decode_short(rangecoder *rc)
{   unsigned short tmp = (unsigned short)decode_culshift(rc,16);
    decode_update( rc,1,tmp,(freq)1<<16);
    return tmp;
}


/* Finish decoding                                           */
/* rc is the range coder to be used                          */
static void done_decoding( rangecoder *rc )
{
    rc;// Keep compiler happy
// [03] The dec_normalize is longer needed as we don't stream input from a file
//	dec_normalize(rc);      /* normalize to use up all bytes */
}




/*
  qsmodel.c     headerfile for quasistatic probability model

  (c) Michael Schindler
  1997, 1998
  http://www.compressconsult.com/ or http://eiunix.tuwien.ac.at/~michael
  michael@compressconsult.com        michael@eiunix.tuwien.ac.at

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.  It may be that this
  program violates local patents in your country, however it is
  belived (NO WARRANTY!) to be patent-free here in Austria.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston,
  MA 02111-1307, USA.

  Qsmodel is a quasistatic probability model that periodically
  (at chooseable intervals) updates probabilities of symbols;
  it also allows to initialize probabilities. Updating is done more
  frequent in the beginning, so it adapts very fast even without
  initialisation.

  it provides function for creation, deletion, query for probabilities
  and symbols and model updating.

  for usage see example.c
*/

/* default tablesize 1<<TBLSHIFT */
#define TBLSHIFT 7

/* rescale frequency counts */
static __inline void dorescale( qsmodel *m)
{   int i, cf, missing;
    if (m->nextleft)  /* we have some more before actual rescaling */
    {   m->incr++;
        m->left = m->nextleft;
        m->nextleft = 0;
        return;
    }
    if (m->rescale < m->targetrescale)  /* double rescale interval if needed */
    {   m->rescale <<= 1;
        if (m->rescale > m->targetrescale)
            m->rescale = m->targetrescale;
    }
    cf = missing = m->cf[m->n];  /* do actual rescaling */
    for(i=m->n-1; i; i--)
    {   int tmp = m->newf[i];
        cf -= tmp;
        m->cf[i] = (qssymbol)cf;
        tmp = tmp>>1 | 1;
        missing -= tmp;
        m->newf[i] = (qssymbol)tmp;
    }
#ifdef NCS_BUILD_WITH_STDERR_DEBUG_INFO
    if (cf!=m->newf[0])
    {   fprintf(stderr,"BUG: rescaling left %d total frequency\n",cf);
        deleteqsmodel(m);
        exit(1);
    }
#endif // NCS_BUILD_WITH_STDERR_DEBUG_INFO
    m->newf[0] = m->newf[0]>>1 | 1;
    missing -= m->newf[0];
    m->incr = missing / m->rescale;
    m->nextleft = missing % m->rescale;
    m->left = m->rescale - m->nextleft;
    if (m->search != NULL)
    {   i=m->n;
        while (i)
        {   int start, end;
            end = (m->cf[i]-1) >> m->searchshift;
            i--;
            start = m->cf[i] >> m->searchshift;
            while (start<=end)
            {   m->search[start] = (qssymbol)i;
                start++;
            }
        }
    }
}


/* initialisation of qsmodel                           */
/* m   qsmodel to be initialized                       */
/* n   number of symbols in that model                 */
/* lg_totf  base2 log of total frequency count         */
/* rescale  desired rescaling interval, should be < 1<<(lg_totf+1) */
/* init  array of int's to be used for initialisation (NULL ok) */
/* compress  set to 1 on compression, 0 on decompression */
static void initqsmodel( qsmodel *m, int n, int lg_totf, int rescale, int *init, int compress )
{   m->n = n;
    m->targetrescale = rescale;
    m->searchshift = lg_totf - TBLSHIFT;
    if (m->searchshift < 0)
        m->searchshift = 0;
    m->cf = (qssymbol*)NCSMalloc((n+1)*sizeof(qssymbol), FALSE);		/* [01] was uint2 */
    m->newf = (qssymbol*)NCSMalloc((n+1)*sizeof(qssymbol), FALSE);	/* [01] was uint2 */
    m->cf[n] = (qssymbol)(1<<lg_totf);
    m->cf[0] = 0;
    if (compress)
        m->search = NULL;
    else
    {   m->search = (qssymbol*)NCSMalloc(((1<<TBLSHIFT)+1)*sizeof(qssymbol), FALSE);	/* [01] was uint2 */
        m->search[1<<TBLSHIFT] = (qssymbol)(n-1);
    }
    resetqsmodel(m, init);
}


/* reinitialisation of qsmodel                         */
/* m   qsmodel to be initialized                       */
/* init  array of int's to be used for initialisation (NULL ok) */
static void resetqsmodel( qsmodel *m, int *init)
{   int i, end, initval;
    m->rescale = m->n>>4 | 2;
    m->nextleft = 0;
    if (init == NULL)
    {   initval = m->cf[m->n] / m->n;
        end = m->cf[m->n] % m->n;
        for (i=0; i<end; i++)
            m->newf[i] = (qssymbol)(initval+1);
        for (; i<m->n; i++)
            m->newf[i] = (qssymbol)initval;
    } else
        for(i=0; i<m->n; i++)
            m->newf[i] = (qssymbol)init[i];
    dorescale(m);
}


/* deletion of qsmodel m                               */
static void deleteqsmodel( qsmodel *m )
{   NCSFree(m->cf);
    NCSFree(m->newf);
    if (m->search != NULL)
        NCSFree(m->search);
}


/* retrieval of estimated frequencies for a symbol     */
/* m   qsmodel to be questioned                        */
/* sym  symbol for which data is desired; must be <n   */
/* sy_f frequency of that symbol                       */
/* lt_f frequency of all smaller symbols together      */
/* the total frequency is 1<<lg_totf                   */
static __inline int qsgetfreq( qsmodel *m, int sym, int *lt_f )
{   return(m->cf[sym+1] - (*lt_f = m->cf[sym]));
}	


/* find out symbol for a given cumulative frequency    */
/* m   qsmodel to be questioned                        */
/* lt_f  cumulative frequency                          */
static __inline int qsgetsym( qsmodel *m, int lt_f )
{   int lo, hi;
    qssymbol *tmp;	/* [01] was uint2 */
    tmp = m->search+(lt_f>>m->searchshift);
    lo = *tmp;
    hi = *(tmp+1) + 1;
    while (lo+1 < hi )
    {   int mid = (lo+hi)>>1;
        if (lt_f < m->cf[mid])
            hi = mid;
        else
            lo = mid;
    }
    return lo;
}


/* update model                                        */
/* m   qsmodel to be updated                           */
/* sym  symbol that occurred (must be <n from init)    */
static __inline void qsupdate( qsmodel *m, int sym )
{   if (m->left <= 0)
        dorescale(m);
    m->left--;
    m->newf[sym] = m->newf[sym] + ((qssymbol)m->incr);
}

#define MAX_RANGE	256		/* number of symbols used in range encoding */

#define MAX_INT8	 127	/* used for difference encoding */
#define	MIN_INT8	-127
#define	MAX_INT16	 32767
#define	MIN_INT16	-32767


#define MIN_ZERO_MEMSET	16	/* number of IEEE4 values that are faster to clear in a loop rather than memset */


#ifdef NCS_NO_UNALIGNED_ACCESS
#	define		READ_UINT16(p) (((UINT16)p[0]) | ((UINT16)p[1] << 8))
#else
#	ifdef NCSBO_MSBFIRST
#	define		READ_UINT16(p) NCSByteSwap16(*((UINT16*)p))
#	else
#	define		READ_UINT16(p) *((UINT16*)p);
#	endif
#endif

/*
**	For the unpack_line() set of routines, we have some local information we allocate
**	while reading a block, and free at the end. This is so we can hide the unpacking
**	from the decompression routine, which only cares about data in a line by line basis
**	not as blocks, so we don't want to contaminate that code with the fact that the block
**	routines work with multiple Y values in a block.
*/
// there is one UnpackBandStruct for each band within each UnpackLineStruct
typedef struct {
    UINT8	*p_packed[MAX_SIDEBAND];	/* ptr into currect location of packed data being unpacked */
    INT16	prev_val[MAX_SIDEBAND];		/* used for ENCODE_RANGE8; previous difference value */
                                        /* [10] also used for ENCODE_RUN_ZERO; zero run count outstanding */
    NCSHuffmanState HuffmanState[MAX_SIDEBAND];

    EncodeFormat encode_format[MAX_SIDEBAND];
    // Range encoder specific values
    rangecoder rc[MAX_SIDEBAND];
    qsmodel qsm[MAX_SIDEBAND];
} UnpackBandStruct;

// there is one UnpackLineStruct for each block being unpacked
typedef struct {
    // There are (nr_bands * MAX_SIDEBAND) of the two following values
    UINT8	*p_compressed_x_block;	// pointer to current packed block for this unpack block
    UnpackBandStruct *p_bands;	/* ptr to array, one for each band */
    UINT16	nr_sidebands;		/* number of sidebands in this block */
    UINT	nr_bands;			/* number of bands in this block */
    UINT16	first_sideband;		/* LL for 0 level, LH for others */
    UINT	line_length;		/* line length in bytes */
    UINT	valid_line;			/* number of bytes between pre-skip and post-skip */
    UINT	pre_skip;			/* amount to pre-read for each line	 */
    UINT	post_skip;			/* amount to post-read for each line */
} UnpackLineStruct;



/*
**	Some of these are called a lot, so are inline where possible
*/

/*******************************************************************************
**	unpack_ecw_block() unpacks an entire block, and returns a newly allocated
**	unpacked block of data. This routine is optional. It is used only by NCS
**	caching, where that library notices that it is worth keeping an unpacked
**	block in memory, rather than unpacking the block on the fly, for performance
**	reasons.
**	This routine is never used where the fileview is so large that caching is
**	not set for the file view.
**
**	NOTES:
**		Despite what NCS might want, this routine MAY decide not to unpack the
**		block. Most likely because:
**		-	The output block would be huge
**		-	Memory is too low
**		In these cases, NCS should continue to use the packed block.
**
**	Returns zero if no error
**
**	See NCS routines for full details
**
*******************************************************************************/

int unpack_ecw_block(QmfLevelStruct *pQmfLevel, UINT nBlockX, UINT nBlockY,
                     UINT8 **ppUnpackedECWBlock, UINT	*pUnpackedLength,
                     UINT8 *pPackedBlock)
{
    UINT8	*pUnpackedBlock, *pUnpackedBand, *pPackedBand;
    UINT	nSidebands;
    UINT	nSidebandLength;
    UINT	nSideband, nBlockSizeX, nBlockSizeY;
    UINT	nPackedOffset = 0;
    UINT	nUnpackedOffset;

    *ppUnpackedECWBlock = NULL;

    if( pQmfLevel->level )
        nSidebands = pQmfLevel->nr_sidebands - 1;
    else
        nSidebands = pQmfLevel->nr_sidebands;

    if( nBlockX != (pQmfLevel->nr_x_blocks - 1) )
        nBlockSizeX = pQmfLevel->x_block_size;
    else
        nBlockSizeX = (pQmfLevel->x_size - (nBlockX * pQmfLevel->x_block_size));
    if( nBlockY != (pQmfLevel->nr_y_blocks - 1) )
        nBlockSizeY = pQmfLevel->y_block_size;
    else
        nBlockSizeY = (pQmfLevel->y_size - (nBlockY * pQmfLevel->y_block_size)); 

    nSidebands = nSidebands * pQmfLevel->nr_bands;
    nSidebandLength = nBlockSizeX * nBlockSizeY * sizeof(INT16);
    nSidebandLength += sizeof(EncodeFormat);		// we want to add one for the leading COMPRESSION TYPE flag
    // we need room for N-1 UINT's of sidebands, and N bytes of compression type flags,
    // and N unpacked blocks.

    *pUnpackedLength = (sizeof(UINT) * (nSidebands-1)) + (nSidebands * nSidebandLength);
    pUnpackedBlock = (UINT8*)NCSMalloc( *pUnpackedLength , FALSE);
    if( !pUnpackedBlock )
        return(1);				// out of memory, so don't attempt the unpack

    // Now unpack. Final structure is:
    //	0:		UINT Offset to sideband 1 relative to offset to sideband 0
    //	1:		UINT Offset of sideband 2 relative to offset to sideband 0
    //	....
    //	N-1:	UINT Offset of sideband N relative to offset to sideband 0
    //			EncodeFormat	sideband 0 COMPRESSION TYPE
    //	<block> <unpacked INT16 block>
    //			EncodeFormat	sideband 1 COMPRESSION TYPE
    //	<block> <unpacked INT16 block>
    //
    //	and so on for all unpacked blocks
    //
    pUnpackedBand = pUnpackedBlock + (sizeof(UINT) * (nSidebands-1));
    pPackedBand   = pPackedBlock + (sizeof(UINT) * (nSidebands-1));
    nSideband = nSidebands;

    *ppUnpackedECWBlock = pUnpackedBlock;	// will reuse the unpacked block pointer

    // now go and pack the block. Start offsets with offset to sideband 1
    nUnpackedOffset = nSidebandLength;

    while(nSideband--) {
        // Length will be the same for all bands, as we unpack to uncompressed format
        if( nSideband ) {
            *pUnpackedBlock++ = (UINT8)((nUnpackedOffset >> 24) & 0xff);	// 0xFF000000
            *pUnpackedBlock++ = (UINT8)((nUnpackedOffset >> 16) & 0xff);	// 0x00FF0000
            *pUnpackedBlock++ = (UINT8)((nUnpackedOffset >>  8) & 0xff);	// 0x0000FF00
            *pUnpackedBlock++ =  (UINT8)(nUnpackedOffset        & 0xff);	// 0x000000FF
        }

        // now go and unpack the block itself
        *((EncodeFormat*)pUnpackedBand) = ENCODE_RAW;
        pUnpackedBand += sizeof(EncodeFormat);

        if( unpack_data(&pUnpackedBand,&pPackedBand[nPackedOffset], nSidebandLength - sizeof(EncodeFormat), sizeof(EncodeFormat)) ) {
            // ERROR during unpack
            NCSFree(*ppUnpackedECWBlock);
            *ppUnpackedECWBlock = NULL;
            return(1);
        }

        // and work out where the next packed block is
        if( nSideband ) {
            nPackedOffset  = ((UINT) *pPackedBlock++) << 24;
            nPackedOffset |= ((UINT) *pPackedBlock++) << 16;
            nPackedOffset |= ((UINT) *pPackedBlock++) << 8;
            nPackedOffset |= ((UINT) *pPackedBlock++);

            pUnpackedBand += (nSidebandLength-sizeof(EncodeFormat));	// was incremented by ENCODE byte already, so just add block length
            nUnpackedOffset += nSidebandLength;
        }
    }
    return(0);
}

/*******************************************************************************
**	align_ecw_block() convert the raw block data to an aligned format
*******************************************************************************/

int align_ecw_block( NCSFile *pNCSFile, NCSBlockId nBlockID, UINT8 **ppAlignedECWBlock, UINT	*pAlignedLength,
                     UINT8 *pPackedBlock, UINT nPackedLength)
{
    UINT8	*pAlignedBlock, *pAlignedBand, *pPackedBand;
    UINT	nSidebands;
    UINT	nSideband;
    UINT	nPackedOffset = 0;
    UINT nNextPackedOffset = 0;
    UINT nAddOffset = 0;
    QmfLevelStruct *pQmfLevel = pNCSFile->pTopQmf;
    while(pQmfLevel && (pQmfLevel->nFirstBlockNumber + pQmfLevel->nr_x_blocks * pQmfLevel->nr_y_blocks) <= nBlockID) {
        pQmfLevel = pQmfLevel->p_larger_qmf;
    }
    if(!pQmfLevel) {
        return(1);
    }
    *ppAlignedECWBlock = NULL;

    if( pQmfLevel->level )
        nSidebands = pQmfLevel->nr_sidebands - 1;
    else
        nSidebands = pQmfLevel->nr_sidebands;

    nSidebands = nSidebands * pQmfLevel->nr_bands;
    // we need room for N-1 UINT's of sidebands, and N bytes of compression type flags,
    // and N unpacked blocks.

    *pAlignedLength = nPackedLength + 2*nSidebands;
    pAlignedBlock = (UINT8*)NCSMalloc( *pAlignedLength , FALSE);
    if( !pAlignedBlock )
        return(1);				// out of memory, so don't attempt the unpack

    // Now unpack. Final structure is:
    //	0:		UINT Offset to sideband 1 relative to offset to sideband 0
    //	1:		UINT Offset of sideband 2 relative to offset to sideband 0
    //	....
    //	N-1:	UINT Offset of sideband N relative to offset to sideband 0
    //			EncodeFormat	sideband 0 COMPRESSION TYPE
    //	<block> <packed INT16 block>
    //			EncodeFormat	sideband 1 COMPRESSION TYPE
    //	<block> <packed INT16 block>
    //
    //	and so on for all unpacked blocks
    //
    pAlignedBand = pAlignedBlock + (sizeof(UINT) * (nSidebands-1));
    pPackedBand   = pPackedBlock + (sizeof(UINT) * (nSidebands-1));
    nSideband = nSidebands;

    *ppAlignedECWBlock = pAlignedBlock;	// will reuse the unpacked block pointer

    // now go and pack the block. Start offsets with offset to sideband 1

    while(nSideband--) {
        BOOLEAN bEven = FALSE;

        if( nSideband ) {
            // Read in the offset to the NEXT block
            nNextPackedOffset  = ((UINT) *pPackedBlock++) << 24;
            nNextPackedOffset |= ((UINT) *pPackedBlock++) << 16;
            nNextPackedOffset |= ((UINT) *pPackedBlock++) << 8;
            nNextPackedOffset |= ((UINT) *pPackedBlock++);
            
            if(((nNextPackedOffset - nPackedOffset) & 0x1) == 0) {
                bEven = TRUE;
                nAddOffset += 2;
            } else {
                nAddOffset++;
            }
            // Add on the alignment factor (+1 per iteration through loop)
            *pAlignedBlock++ = (UINT8)(((nNextPackedOffset + nAddOffset) >> 24) & 0xff);	// 0xFF000000
            *pAlignedBlock++ = (UINT8)(((nNextPackedOffset + nAddOffset) >> 16) & 0xff);	// 0x00FF0000
            *pAlignedBlock++ = (UINT8)(((nNextPackedOffset + nAddOffset) >>  8) & 0xff);	// 0x0000FF00
            *pAlignedBlock++ =  (UINT8)((nNextPackedOffset + nAddOffset)        & 0xff);	// 0x000000FF
        }
        // Copy the EncodeFormat, changing from a UINT8 to a UINT16 so the encoded data is UINT16 aligned
        *(EncodeFormat*)pAlignedBand = *pPackedBand++;
        // Increment the AlignedBand pointer by sizeof(UINT16)
        pAlignedBand += sizeof(EncodeFormat);
        // Copy the encoded data, the length being the difference betweem the current and the last offset, 
        // minus 1 for the UINT8 encodeformat.
        memcpy(pAlignedBand, pPackedBand, nSideband ? (nNextPackedOffset - nPackedOffset - 1) : (nPackedLength - (nNextPackedOffset + sizeof(UINT) * (nSidebands-1) + 1)));

        // and work out where the next packed block is
        if( nSideband ) {
            pAlignedBand += nNextPackedOffset - nPackedOffset - (bEven ? 0 : 1);
            pPackedBand += nNextPackedOffset - nPackedOffset - 1;
            nPackedOffset  = nNextPackedOffset;
        }
    }
    return(0);
}

/*******************************************************************************
**	unpack_data() unpacks previously packed data, and returns a newly allocated
**	unpacked array of data.
**
**	See file header for full details
**
*******************************************************************************/

int	unpack_data(UINT8 **p_raw,
                UINT8 *p_packed, UINT raw_length,
                UINT8 nSizeOfEncodeFormat)
{
    UINT8	*p_unpacked;

    if( !p_packed || !raw_length ) {
        return(1);
    }

    // Allocate the unpack array if caller did not allocate it
    if( *p_raw )		// [06]
        p_unpacked = *p_raw;
    else {
        p_unpacked = (UINT8 *) NCSMalloc( raw_length , FALSE);
        if( !p_unpacked )
            return(1);
    }

    switch( (nSizeOfEncodeFormat == sizeof(EncodeFormat)) ? (*(EncodeFormat*)p_packed) : *p_packed ) {
        default :
            return(1);	
        break;
        case ENCODE_ZEROS :
            memset(p_unpacked, 0, raw_length);
        break;
        case ENCODE_RAW :
            memcpy(p_unpacked, p_packed + nSizeOfEncodeFormat, raw_length);
        break;

        case ENCODE_RUN_ZERO : {
            register UINT	nWordCount = raw_length / 2;
            register UINT16	nZeroRun = 0;
            register INT16 *p_output = (INT16 *) p_unpacked;

            p_packed += nSizeOfEncodeFormat;

            while(nWordCount--) {
                register	UINT16	nValue;
                nValue = READ_UINT16(p_packed);p_packed += sizeof(UINT16);//*pZeroPacked++;

                if( nValue & RUN_MASK ) {
                    register	UINT16 nZero;
                    // If a zero value, have to update word skip count
                    nZeroRun = (nValue & MAX_RUN_LENGTH) - 1;

                    if( nZeroRun >= nWordCount ) {
                        nZero = (UINT16)nWordCount + 1;	// already decrememented by 1
                        nZeroRun = (UINT16)(nZeroRun - nWordCount);
                        nWordCount = 0;			// Force exit of the while() loop later
                    }
                    else {
                        nZero = nZeroRun + 1;	// already decremented by 1
                        nWordCount -= nZeroRun;		// else mark off these zeros as read
                        nZeroRun = 0;
                    }
                    // Set memory to zero. Use memclr if a large amount of memory
                    if( nZero < MIN_ZERO_MEMSET ) {
                        while(nZero--)
                            *p_output++ = 0;
                    }
                    else {
                        memset(p_output, 0, nZero * sizeof(INT16));
                        p_output += nZero;
                    }
                }
                else {
                    if( nValue & SIGN_MASK )
#ifdef NCSBO_MSBFIRST
                        *p_output++  = NCSByteSwap16(((INT16) (nValue & VALUE_MASK)) * -1);
#else
                        *p_output++  = ((INT16) (nValue & VALUE_MASK)) * -1;
#endif
                    else
#ifdef NCSBO_MSBFIRST
                        *p_output++  = NCSByteSwap16((INT16) nValue);
#else
                        *p_output++  = ((INT16) nValue);
#endif
                }
            }
        }
        break;

        case ENCODE_HUFFMAN : {
                p_packed += nSizeOfEncodeFormat;		// skip past encoding information
                unpack_huffman(p_packed, (INT16 *) p_unpacked, raw_length);
            }
        break;

        case ENCODE_RANGE : {
            // range encoder variables
            register int ch, syfreq;
            int ltfreq;
            rangecoder rc;
            qsmodel qsm;
            register UINT	count = raw_length;
            register UNALIGNED UINT8	*p_output = p_unpacked;

            memset( &rc, 0, sizeof(rc));
            memset( &qsm, 0, sizeof(qsm));

            /* init the model the same as in the compressor */
            rc.p_packed = p_packed + nSizeOfEncodeFormat;	/* read encoded stream from this memory */
            initqsmodel(&qsm,257,12,2000,NULL,0);
            start_decoding(&rc);
            while(count--) {
                // unpack a byte
                ltfreq = decode_culshift(&rc,12);
                ch = qsgetsym(&qsm, ltfreq);
                *p_output++ = (UINT8) ch;
                syfreq = qsgetfreq(&qsm,ch,&ltfreq);
                decode_update( &rc, syfreq, ltfreq, 1<<12);
                qsupdate(&qsm,ch);
            }
            syfreq = qsgetfreq(&qsm,256,&ltfreq);
            decode_update( &rc, syfreq, ltfreq, 1<<12);
            done_decoding(&rc);
            deleteqsmodel(&qsm);
        }
        break;
        // Similar to range encoding, but first does 8 bit differencing
        case ENCODE_RANGE8 : {
            // range encoder variables
            register int ch, syfreq;
            int ltfreq;
            rangecoder rc;
            qsmodel qsm;
            register UINT	count = raw_length;
            register UNALIGNED INT16	*p_output = (INT16 *) p_unpacked;
            register INT16	prev;

            /* init the model the same as in the compressor */
            rc.p_packed = p_packed + nSizeOfEncodeFormat;	/* read encoded stream from this memory */
            initqsmodel(&qsm,257,12,2000,NULL,0);
            start_decoding(&rc);

            // unpack first value as 16 bits, top 8 bits first
            // upper 8 bits
            ltfreq = decode_culshift(&rc,12);
            ch = qsgetsym(&qsm, ltfreq);
            prev = (INT16)((ch & 0xff) << 8);
            syfreq = qsgetfreq(&qsm,ch,&ltfreq);
            decode_update( &rc, syfreq, ltfreq, 1<<12);
            qsupdate(&qsm,ch);
            // lower 8 bits
            ltfreq = decode_culshift(&rc,12);
            ch = qsgetsym(&qsm, ltfreq);
            prev |= (ch & 0xff);
            syfreq = qsgetfreq(&qsm,ch,&ltfreq);
            decode_update( &rc, syfreq, ltfreq, 1<<12);
            qsupdate(&qsm,ch);

            count >>= 1;				// [06] must loop through 16 bit values
            while(count--) {
                register	INT8	diff;
                // unpack a byte
                ltfreq = decode_culshift(&rc,12);
                ch = qsgetsym(&qsm, ltfreq);
                // don't check for end of symbol run, as 256 is same as a negative number
                diff = (INT8) ch;		// we have to force it to signed first
                prev = prev + diff;		// then we can add it safely
#ifdef NCSBO_MSBFIRST
                *p_output++ = NCSByteSwap16(prev);
#else
                *p_output++ = prev;
#endif
                syfreq = qsgetfreq(&qsm,ch,&ltfreq);
                decode_update( &rc, syfreq, ltfreq, 1<<12);
                qsupdate(&qsm,ch);
            }
            syfreq = qsgetfreq(&qsm,256,&ltfreq);
            decode_update( &rc, syfreq, ltfreq, 1<<12);
            done_decoding(&rc);
            deleteqsmodel(&qsm);
        }
        break;
    }
    if( !*p_raw )		// [06]
        *p_raw = p_unpacked;
    return(0);
}


/*
**	unpack_init_lines()	  call once per level. Allocates memory structures for unpacking lines
**	unpack_start_lines()  sets up decompression ready to unpack sequential lines out of
**						  a packed block.
**	unpack_line()		  unpacks one line of previously packed QMF level data, to the
**						  indicated previously allocated output line.
**	unpack_finish_lines() completes decompression, enabling the decompressor to free
**						  any tables or memory it might have set up.
**	unpack_free_lines()	  Call once per level. Frees memory structures used to unpack lines.
**
**	NOTES:
**
**	unpack_line() will pre-read and throw away pre-read bytes, then unpack the desired section,
**	then will post read and throw away post-read bytes.
**
**	It keeps various compression states between calls, so these must not be deleted
**	between calls.
**
**	All lengths for these routines are in symbol size, not bytes
**
**
**	See file header for full details
**
*/

/*
**	unpack_init_lines() - call this once per level.
**
**	Allocates enough p_info blocks to hold the blocks to be read line by line for this level.
**	Also sets up the start and end of line skip values for these blocks.
**
*/
int unpack_init_lines( QmfRegionLevelStruct *p_level )
{

    UnpackLineStruct *p_info;
    UINT	next_x_block, x_block_count;

    // set up a pointer to an array of the compression info structures
    p_info = (UnpackLineStruct *) NCSMalloc(sizeof(UnpackLineStruct) * p_level->x_block_count, FALSE);
    if( !p_info )
        return(1);

    /*
    ** To reduce malloc's, we allocate all memory needed for all compression info band structures
    ** using the first block (so it always points to the start of the arrays), and
    ** then only free using the first compression structure. This is harder to follow,
    ** but much faster.
    */
    // [12] because read can be aborted at any time, must set the structure to zero
    // [12] to ensure free's are correct.
    p_info->p_bands = (UnpackBandStruct *) 
            NCSMalloc(sizeof(UnpackBandStruct) * p_level->x_block_count * p_level->used_bands, TRUE);
    if( !p_info->p_bands ) {
        NCSFree(p_info);
        return(1);
    }

    // Now point to the correct array locations, set up skip values, etc
    for(next_x_block = p_level->start_x_block, x_block_count = 0;
                x_block_count < p_level->x_block_count;
                x_block_count++, next_x_block++ ) {
        UINT	x_block_size;

        // point to the appropriate band. We point to the first band for the block,
        // the rest are offset from that band
        p_info[x_block_count].p_bands = p_info[0].p_bands + (x_block_count * p_level->used_bands);

        if( next_x_block != (p_level->p_qmf->nr_x_blocks - 1) )
            x_block_size = p_level->p_qmf->x_block_size;
        else
            x_block_size = (p_level->p_qmf->x_size - (next_x_block * p_level->p_qmf->x_block_size));
        p_info[x_block_count].p_compressed_x_block = NULL;
        p_info[x_block_count].first_sideband = (p_level->p_qmf->level ? LH_SIDEBAND : LL_SIDEBAND );
        p_info[x_block_count].nr_sidebands = p_level->p_qmf->p_top_qmf->nr_sidebands;
        p_info[x_block_count].nr_bands = p_level->used_bands;
        p_info[x_block_count].line_length = x_block_size;
        p_info[x_block_count].pre_skip = (next_x_block == p_level->start_x_block ? p_level->first_block_skip : 0);
        p_info[x_block_count].post_skip = (x_block_count == p_level->x_block_count - 1 ? p_level->last_block_skip  : 0);
        p_info[x_block_count].valid_line = x_block_size - (p_info[x_block_count].pre_skip + p_info[x_block_count].post_skip);
    }
    p_level->p_x_blocks = (void *) p_info;
    return(0);
}

void unpack_free_lines(  QmfRegionLevelStruct *p_level  )
{
    UnpackLineStruct *p_info;

    p_info = (UnpackLineStruct *) p_level->p_x_blocks;
    if( !p_info )
        return;
    // wrap up any unfinished decompression
    unpack_finish_lines( p_level );
    // free the block allocations. They were all allocated and pointed as a 
    // single array for all bands, all blocks.
    
    NCSFree(p_info->p_bands);
    NCSFree(p_level->p_x_blocks);
    p_level->p_x_blocks = NULL;
}

/*
**	This stops any compression in progress for these blocks, and frees the compressed blocks,
**	but leaves the info structures still allocated for the next set of X block.
*/

void unpack_finish_lines( QmfRegionLevelStruct *p_level  )
{
    UnpackLineStruct *p_info;
    UINT	x_block_count;

    p_info = (UnpackLineStruct *) p_level->p_x_blocks;
    for(x_block_count = 0; x_block_count < p_level->x_block_count; x_block_count++ ) {
        // Free any open part-way compressions or opened compressed blocks
        if( p_info->p_compressed_x_block) {
            register UINT	band;
            register UnpackBandStruct *p_bands;
            p_bands = p_info->p_bands;
            for(band = 0; band < p_info->nr_bands; band++ ) {
                register int sideband;
                // We must free information depending on what type of compression was used
                // for each sideband
                for(sideband = p_info->first_sideband; sideband < p_info->nr_sidebands; sideband++ ) {
                    switch( p_bands->encode_format[sideband] ) {
                        default :
                            // this is an error
                            _ASSERT(FALSE);
                            ;
                        break;
                        case ENCODE_ZEROS :
                        case ENCODE_RAW :
                        case ENCODE_RUN_ZERO :
                            // do nothing
                            ;
                        case ENCODE_HUFFMAN :
                            unpack_huffman_fini_state(&(p_bands->HuffmanState[sideband]));
                        break;
                        case ENCODE_RANGE :
                        case ENCODE_RANGE8 :
                            // free the range encoder memory
                            done_decoding(&(p_bands->rc[sideband]));
                            deleteqsmodel(&(p_bands->qsm[sideband]));
                        break;
                    }
                    p_bands->encode_format[sideband] = ENCODE_INVALID;	/* just in case */
                }
                p_bands++;
            }
            NCScbmFreeViewBlock(p_level, p_info->p_compressed_x_block );
            p_info->p_compressed_x_block = NULL;
        }
        p_info++;
    }
}

/*
**	This sets up unpacking for the specified block. It pre-reads the
**	indicated number of lines_to_skip.  It does NOT free p_packed_block
**	if there was an error. However, if all went well, the block is pointed
**	to by the info structures, and will later be freed by the unpack_finish_lines() call.
*/

int unpack_start_line_block( QmfRegionLevelStruct *p_level, UINT x_block_count,
                            UINT8 *p_packed_block, UINT	lines_to_skip)
{

    UnpackLineStruct *p_info;
    int	sideband;
    UINT		*p_offset;
    UINT		offset;
    UINT		band;
    UnpackBandStruct	*p_band;
    UINT8		*p_base_of_block = p_packed_block;

    p_info = (UnpackLineStruct *) p_level->p_x_blocks;
    p_info += x_block_count;		// point to the currect block to work on

    p_offset = (UINT *) p_base_of_block;
    offset = 0;

    // there nr_bands * (nr_sidebands-1) offsets, as we know
    // the first one is straight after the offset array.
    // Note the offset is QMF bands, not level bands.
    p_packed_block = p_base_of_block + (sizeof(UINT) * 
                ((p_level->p_qmf->nr_bands * 
                    (p_info->nr_sidebands - p_info->first_sideband)) - 1));
    // extract the offsets for each sideband and point to them
    // and skip the leading bytes
    for(band=0; band < p_info->nr_bands; band++ ) {
        p_band = p_info->p_bands + band;
        for(sideband = p_info->first_sideband; sideband < p_info->nr_sidebands; sideband++ ) {
            UINT8	*p_packed_sideband;
            p_packed_sideband = p_packed_block + offset;
            p_band->encode_format[sideband] = *((EncodeFormat*)p_packed_sideband);
            p_band->p_packed[sideband] = p_packed_sideband + sizeof(EncodeFormat);	// skip over encoding format
            switch( p_band->encode_format[sideband] ) {
                default :
                    return(1);
                break;
                case ENCODE_ZEROS :
                    ;	// nothing to do
                break;

                case ENCODE_RAW :
                    // skip over lines_to_skip
                    p_band->p_packed[sideband] += 
                            (p_info->line_length * lines_to_skip * sizeof(UINT16));
                break;

                case ENCODE_RUN_ZERO : {
                    // [10] The problem here is the zero-run count.
                    // We have to keep track of outstanding zero's
                    register	UINT	nWordSkip;
                    register	UINT16	nZeroRun = 0;	// outstanding zero count
                    register	UINT16	*pZeroPacked = (UINT16 *) p_band->p_packed[sideband];
                    nWordSkip = (p_info->line_length * lines_to_skip);
                    while(nWordSkip--) {
                        register	UINT16	nValue;
#ifdef NCSBO_MSBFIRST
                        nValue = NCSByteSwap16(*pZeroPacked++);
#else
                        nValue = *pZeroPacked++;
#endif
                        if( nValue & RUN_MASK ) {
                            // If a zero value, have to update word skip count
                            nZeroRun = (nValue & MAX_RUN_LENGTH) - 1;	// already read 1 word
                            if( nZeroRun >= nWordSkip ) {
                                nZeroRun = (UINT16)(nZeroRun - nWordSkip);
                                break;					// found more zero's than we wanted, so break
                            }
                            nWordSkip -= nZeroRun;		// else mark off these zeros as read
                            nZeroRun = 0;
                        }
                    }
                    p_band->prev_val[sideband] = nZeroRun;				/* keep track of outstanding zero's */
                    p_band->p_packed[sideband] = (UINT8 *) pZeroPacked;	/* update packed pointer */
                }
                break;

                case ENCODE_HUFFMAN : {
                    // [10] The problem here is the zero-run count.
                    // We have to keep track of outstanding zero's
                    register	UINT	nWordSkip;
                    register	UINT16	nZeroRun = 0;	// outstanding zero count

                    unpack_huffman_init_state(&(p_band->HuffmanState[sideband]), &(p_band->p_packed[sideband]));

                    nWordSkip = (p_info->line_length * lines_to_skip);
                    while(nWordSkip--) {
                        register	NCSHuffmanSymbol *pSymbol;

                        pSymbol = unpack_huffman_symbol(&(p_band->p_packed[sideband]), &(p_band->HuffmanState[sideband]));
                        
                        if(unpack_huffman_symbol_zero_run(pSymbol)) {
                            nZeroRun = unpack_huffman_symbol_zero_length(pSymbol);
                            if( nZeroRun >= nWordSkip ) {
                                nZeroRun = (UINT16)(nZeroRun - nWordSkip);
                                break;					// found more zero's than we wanted, so break
                            }
                            nWordSkip -= nZeroRun;		// else mark off these zeros as read
                            nZeroRun = 0;
                        }
                    }
                    p_band->prev_val[sideband] = nZeroRun;				/* keep track of outstanding zero's */
                }
                break;

                case ENCODE_RANGE : {
                    UINT	skip_bytes;
                    // range encoder variables
                    register int ch, syfreq;
                    int	ltfreq;
                    rangecoder *p_rc;
                    qsmodel *p_qsm;

                    /* init the model the same as in the compressor */
                    p_rc  = &(p_band->rc[sideband]);
                    p_qsm = &(p_band->qsm[sideband]);
                    p_rc->p_packed = p_band->p_packed[sideband];	/* read encoded stream from this memory */
                    initqsmodel(p_qsm,257,12,2000,NULL,0);
                    start_decoding(p_rc);

                    skip_bytes = (p_info->line_length * lines_to_skip * sizeof(UINT16));
                    while(skip_bytes--) {
                        // Read and throw away some data
                        ltfreq = decode_culshift(p_rc,12);
                        ch = qsgetsym(p_qsm, ltfreq);
                        syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                        decode_update( p_rc, syfreq, ltfreq, 1<<12);
                        qsupdate(p_qsm,ch);
                    }
                }
                break;

                case ENCODE_RANGE8 : {
                    UINT	skip_bytes;
                    // range encoder variables
                    register int ch, syfreq;
                    int ltfreq;
                    register rangecoder *p_rc;
                    register qsmodel *p_qsm;
                    register	INT16	prev;

                    // We first retrieve the 16 bit start value, then if skipping we
                    // keep track of the previous value, for future use
                    /* init the model the same as in the compressor */
                    p_rc  = &(p_band->rc[sideband]);
                    p_qsm = &(p_band->qsm[sideband]);
                    p_rc->p_packed = p_band->p_packed[sideband];	/* read encoded stream from this memory */
                    initqsmodel(p_qsm,257,12,2000,NULL,0);
                    start_decoding(p_rc);
            
                    // unpack start value for difference stream. It is 16 bits long
                    ltfreq = decode_culshift(p_rc,12);
                    ch = qsgetsym(p_qsm, ltfreq);
                    prev = (INT16)((ch & 0xff) << 8);
                    syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                    decode_update(p_rc, syfreq, ltfreq, 1<<12);
                    qsupdate(p_qsm,ch);
                    // lower 8 bits
                    ltfreq = decode_culshift(p_rc,12);
                    ch = qsgetsym(p_qsm, ltfreq);
                    prev |= (ch & 0xff);
                    syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                    decode_update(p_rc, syfreq, ltfreq, 1<<12);
                    qsupdate(p_qsm,ch);
        
                    // we skip in 8 bit units, as they were encoded as 8 bit differences
                    skip_bytes = (p_info->line_length * lines_to_skip);
                    while(skip_bytes--) {
                        register	INT8	diff;
                        // unpack a byte
                        ltfreq = decode_culshift(p_rc,12);
                        ch = qsgetsym(p_qsm, ltfreq);
                        diff = (INT8) ch;		// we have to force it to signed first
                        prev = prev + diff;		// then we can add it safely
                        syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                        decode_update(p_rc, syfreq, ltfreq, 1<<12);
                        qsupdate(p_qsm,ch);
                    }
                    p_band->prev_val[sideband] = prev;		/* keep track of our previous value */
                }
                break;
            }
            if(!((band == p_info->nr_bands -1) && (sideband == p_info->nr_sidebands - 1))) {
                /*
                ** Don't read offset if finished [13]
                */
                offset = sread_int32((UINT8 *) p_offset);
                p_offset++;
            }
        }	/* end sideband for loop */
    }	/* end band for loop */
    p_info->p_compressed_x_block = p_base_of_block;
    return(0);
}

/*
**	Unpack one line of the set of X blocks into the p_level->line1[] structure.
**	We must consider reflection on the left size.
*/

int	unpack_line(QmfRegionLevelStruct *p_level )
{
    UINT			x_block_count;
    UnpackLineStruct *p_info;
    UINT			band;
    int		sideband;

    // Loop through each block for the input line
    for(x_block_count = 0; x_block_count < p_level->x_block_count; x_block_count++ ) {
        UINT	block_offset;
        p_info = (UnpackLineStruct *) p_level->p_x_blocks;
        // while pointing to block 0, compute offset, as first block is a partial block
        if( x_block_count )
            block_offset = p_level->reflect_start_x + p_info->valid_line +
                            ((x_block_count - 1) * p_level->p_qmf->x_block_size);
        else
            block_offset = p_level->reflect_start_x;
        p_info = p_info + x_block_count;
        // Loop through each band for each block of the input line
        for( band=0; band < p_level->used_bands; band++ ) {
            UnpackBandStruct *p_band;
            p_band = p_info->p_bands + band;
            // Loop through each sideband for each band of each block of the input line
            for( sideband = p_info->first_sideband;	sideband < p_info->nr_sidebands; sideband++ ) {
                register IEEE4	*p_output;
                register IEEE4  fBinSize;

                p_output = p_level->p_p_line1[DECOMP_INDEX] + block_offset;

                // As of V2.0, p_qmf->scale_factor no longer has any meaning. It is always
                // 1. However, to be backward compatible with V1.0 files, we divide by scale_factor
                // to sort out scaling.

                fBinSize = ((IEEE4) p_level->p_qmf->p_band_bin_size[band]) /
                                    p_level->p_qmf->scale_factor;

                switch( p_band->encode_format[sideband] ) {
                    default :
                        return(1);
                    break;

                    case ENCODE_ZEROS :
                        if( p_info->valid_line )	// just to be safe, check for non-zero
                            memset(p_output, 0, p_info->valid_line * sizeof(INT));
                    break;

                    case ENCODE_RAW : {
                        register UNALIGNED INT16	*p_quantized;	// must be an INT16,do not change
                        register UINT	count;

                        p_quantized = (INT16 *) p_band->p_packed[sideband];
                        p_quantized += p_info->pre_skip;
                        count = p_info->valid_line;
                        
                        if(p_level->p_qmf->scale_factor == 1.0) {
                            INT nBinSize = (INT)p_level->p_qmf->p_band_bin_size[band];
                            while(count--) {
#ifdef NCSBO_MSBFIRST
                                *p_output++  = (IEEE4)(((INT16)NCSByteSwap16(*p_quantized++)) * nBinSize);
#else
                                *p_output++  = (IEEE4)(*p_quantized++ * nBinSize);
#endif							
                            }
                        } else {
                            while(count--) {
#ifdef NCSBO_MSBFIRST
                                *p_output++  = ((IEEE4) (INT16)NCSByteSwap16(*p_quantized++)) * fBinSize;
#else
                                *p_output++  = ((IEEE4) (*p_quantized++)) * fBinSize;
#endif							
                            }
                        }
                        p_band->p_packed[sideband] += 
                                    p_info->line_length * sizeof(UINT16);
                    }
                    break;

                    case ENCODE_RUN_ZERO : {
                        // [10] The problem here is the zero-run count.
                        // We have to keep track of outstanding zero's
                        register	UINT	nWordCount   = p_info->pre_skip;
                        register	UINT16	nZeroRun     = p_band->prev_val[sideband];	// outstanding zero count
                        register	UNALIGNED UINT16	*pZeroPacked = (UINT16 *) p_band->p_packed[sideband];

                        // PRESKIP: Have to pre-skip some values. Might have outstanding zeros to consider
                        if( nZeroRun && nWordCount ) {
                            // adjust by pre-zero run count
                            if( nZeroRun >= nWordCount ) {
                                nZeroRun  = nZeroRun - (UINT16)nWordCount;	// Note this limits block size to 2^15
                                nWordCount = 0;
                            }
                            else {
                                nWordCount -= nZeroRun;
                                nZeroRun = 0;
                            }
                        }
                        // now sorted out leading zeros, so now read-skip others, again maybe with
                        // an outstanding run zero at the end of the run.
                        while(nWordCount--) {
                            register	UINT16	nValue;
#ifdef NCSBO_MSBFIRST
                            nValue = NCSByteSwap16(*pZeroPacked++);
#else
                            nValue = *pZeroPacked++;
#endif
                            if( nValue & RUN_MASK ) {
                                // If a zero value, have to update word skip count
                                nZeroRun = (nValue & MAX_RUN_LENGTH) - 1;	// already read 1 word
                                if( nZeroRun >= nWordCount ) {
                                    nZeroRun = (UINT16)(nZeroRun - nWordCount);
                                    break;					// found more zero's than we wanted, so break
                                }
                                else {
                                    nWordCount -= nZeroRun;		// else mark off these zeros as read
                                    nZeroRun = 0;
                                }
                            }
                        }

                        // QUANTIZE: Now read the values actually needed. Again maybe with a run zero at the end
                        // now sorted out leading zeros, so now read-skip others, again maybe with
                        // an outstanding run zero at the end of the run.
                        nWordCount = p_info->valid_line;
                        if( nZeroRun && nWordCount ) {
                            register	UINT16 nZero;
                            // adjust by pre-zero run count
                            if( nZeroRun >= nWordCount ) {
                                nZero = (UINT16)nWordCount;
                                nZeroRun  = nZeroRun -(UINT16)nWordCount;	// Note this limits block size to 2^15
                                nWordCount = 0;
                            }
                            else {
                                nZero = nZeroRun;
                                nWordCount -= nZeroRun;
                                nZeroRun = 0;
                            }
                            // Set memory to zero. Use memclr if a large amount of memory
                            if( nZero < MIN_ZERO_MEMSET ) {
                                while(nZero--)
                                    *p_output++ = 0;
                            }
                            else {
                                memset(p_output, 0, nZero * sizeof(IEEE4));
                                p_output += nZero;
                            }
                        }
                        while(nWordCount--) {
                            register	UINT16	nValue;
#ifdef NCSBO_MSBFIRST
                            nValue = NCSByteSwap16(*pZeroPacked++);
#else
                            nValue = *pZeroPacked++;
#endif
                            if( nValue & RUN_MASK ) {
                                register	UINT16 nZero;
                                // If a zero value, have to update word skip count
                                nZeroRun = (nValue & MAX_RUN_LENGTH) - 1;
                                if( nZeroRun >= nWordCount ) {
                                    nZero = (UINT16)nWordCount + 1;	// already decrememented by 1
                                    nZeroRun = (UINT16)(nZeroRun - nWordCount);
                                    nWordCount = 0;			// Force exit of the while() loop later
                                }
                                else {
                                    nZero = nZeroRun + 1;	// already decremented by 1
                                    nWordCount -= nZeroRun;		// else mark off these zeros as read
                                    nZeroRun = 0;
                                }
                                // Set memory to zero. Use memclr if a large amount of memory
                                if( nZero < MIN_ZERO_MEMSET ) {
                                    while(nZero--)
                                        *p_output++ = 0;
                                }
                                else {
                                    memset(p_output, 0, nZero * sizeof(IEEE4));
                                    p_output += nZero;
                                }
                            }
                            else {
                                if( nValue & SIGN_MASK )
                                    *p_output++  = ((IEEE4) (nValue & VALUE_MASK)) * fBinSize * -1;
                                else
                                    *p_output++  = ((IEEE4) nValue) * fBinSize;
                            }
                        }


                        // POSTSKIP: Have to post-skip some values. Might have outstanding zeros to consider
                        nWordCount = p_info->post_skip;
                        if( nZeroRun && nWordCount ) {
                            // adjust by pre-zero run count
                            if( nZeroRun >= nWordCount ) {
                                nZeroRun  = nZeroRun -(UINT16)nWordCount;	// Note this limits block size to 2^15
                                nWordCount = 0;
                            }
                            else {
                                nWordCount -= nZeroRun;
                                nZeroRun = 0;
                            }
                        }
                        // now sorted out leading zeros, so now read-skip others, again maybe with
                        // an outstanding run zero at the end of the run.
                        while(nWordCount--) {
                            register	UINT16	nValue;
#ifdef NCSBO_MSBFIRST
                            nValue = NCSByteSwap16(*pZeroPacked++);
#else
                            nValue = *pZeroPacked++;
#endif
                            if( nValue & RUN_MASK ) {
                                // If a zero value, have to update word skip count
                                nZeroRun = (nValue & MAX_RUN_LENGTH) - 1;	// already read 1 word
                                if( nZeroRun >= nWordCount ) {
                                    nZeroRun = (UINT16)(nZeroRun - nWordCount);
                                    break;					// found more zero's than we wanted, so break
                                }
                                nWordCount -= nZeroRun;		// else mark off these zeros as read
                                nZeroRun = 0;
                            }
                        }


                        p_band->prev_val[sideband] = nZeroRun;				/* keep track of outstanding zero's */
                        p_band->p_packed[sideband] = (UINT8 *) pZeroPacked;	/* update packed pointer */
                    }
                    break;

                    case ENCODE_HUFFMAN : {
                        // [10] The problem here is the zero-run count.
                        // We have to keep track of outstanding zero's
                        register	UINT	nWordCount   = p_info->pre_skip;
                        register	UINT16	nZeroRun     = p_band->prev_val[sideband];	// outstanding zero count

                        // PRESKIP: Have to pre-skip some values. Might have outstanding zeros to consider
                        if( nZeroRun && nWordCount ) {
                            // adjust by pre-zero run count
                            if( nZeroRun >= nWordCount ) {
                                nZeroRun  = nZeroRun -(UINT16)nWordCount;	// Note this limits block size to 2^15
                                nWordCount = 0;
                            }
                            else {
                                nWordCount -= nZeroRun;
                                nZeroRun = 0;
                            }
                        }
                        // now sorted out leading zeros, so now read-skip others, again maybe with
                        // an outstanding run zero at the end of the run.
                        while(nWordCount--) {
                            register	NCSHuffmanSymbol *pSymbol;
                            pSymbol = unpack_huffman_symbol(&(p_band->p_packed[sideband]), &(p_band->HuffmanState[sideband]));
                            if(unpack_huffman_symbol_zero_run(pSymbol)) {
                                nZeroRun = unpack_huffman_symbol_zero_length(pSymbol);
                                if( nZeroRun >= nWordCount ) {
                                    nZeroRun = (UINT16)(nZeroRun - nWordCount);
                                    break;					// found more zero's than we wanted, so break
                                }
                                else {
                                    nWordCount -= nZeroRun;		// else mark off these zeros as read
                                    nZeroRun = 0;
                                }
                            }
                        }

                        // QUANTIZE: Now read the values actually needed. Again maybe with a run zero at the end
                        // now sorted out leading zeros, so now read-skip others, again maybe with
                        // an outstanding run zero at the end of the run.
                        nWordCount = p_info->valid_line;
                        if( nZeroRun && nWordCount ) {
                            register	UINT16 nZero;
                            // adjust by pre-zero run count
                            if( nZeroRun >= nWordCount ) {
                                nZero = (UINT16)nWordCount;
                                nZeroRun  = nZeroRun -(UINT16)nWordCount;	// Note this limits block size to 2^15
                                nWordCount = 0;
                            }
                            else {
                                nZero = nZeroRun;
                                nWordCount -= nZeroRun;
                                nZeroRun = 0;
                            }
                            // Set memory to zero. Use memclr if a large amount of memory
                            if( nZero < MIN_ZERO_MEMSET ) {
                                while(nZero--)
                                    *p_output++ = 0;
                            }
                            else {
                                memset(p_output, 0, nZero * sizeof(IEEE4));
                                p_output += nZero;
                            }
                        }
                        while(nWordCount--) {
                            register	NCSHuffmanSymbol *pSymbol;
                            pSymbol = unpack_huffman_symbol(&(p_band->p_packed[sideband]), &(p_band->HuffmanState[sideband]));
                            if(unpack_huffman_symbol_zero_run(pSymbol)) {
                                register	UINT16 nZero;
                                nZeroRun = unpack_huffman_symbol_zero_length(pSymbol);
                                // If a zero value, have to update word skip count
                                if( nZeroRun >= nWordCount ) {
                                    nZero = (UINT16)nWordCount + 1;	// already decrememented by 1
                                    nZeroRun = (UINT16)(nZeroRun - nWordCount);
                                    nWordCount = 0;			// Force exit of the while() loop later
                                }
                                else {
                                    nZero = nZeroRun + 1;	// already decremented by 1
                                    nWordCount -= nZeroRun;		// else mark off these zeros as read
                                    nZeroRun = 0;
                                }
                                // Set memory to zero. Use memclr if a large amount of memory
                                if( nZero < MIN_ZERO_MEMSET ) {
                                    while(nZero--)
                                        *p_output++ = 0;
                                }
                                else {
                                    memset(p_output, 0, nZero * sizeof(IEEE4));
                                    p_output += nZero;
                                }
                            }
                            else {
#ifdef NCSBO_MSBFIRST
                                *p_output++  = ((IEEE4) (INT16)NCSByteSwap16(unpack_huffman_symbol_value(pSymbol))) * fBinSize;
#else
                                *p_output++  = unpack_huffman_symbol_value(pSymbol) * fBinSize;
#endif
                            }
                        }


                        // POSTSKIP: Have to post-skip some values. Might have outstanding zeros to consider
                        nWordCount = p_info->post_skip;
                        if( nZeroRun && nWordCount ) {
                            // adjust by pre-zero run count
                            if( nZeroRun >= nWordCount ) {
                                nZeroRun  = nZeroRun -(UINT16)nWordCount;	// Note this limits block size to 2^15
                                nWordCount = 0;
                            }
                            else {
                                nWordCount -= nZeroRun;
                                nZeroRun = 0;
                            }
                        }
                        // now sorted out leading zeros, so now read-skip others, again maybe with
                        // an outstanding run zero at the end of the run.
                        while(nWordCount--) {
                            register	NCSHuffmanSymbol *pSymbol;
                            pSymbol = unpack_huffman_symbol(&(p_band->p_packed[sideband]), &(p_band->HuffmanState[sideband]));
                            if(unpack_huffman_symbol_zero_run(pSymbol)) {
                                nZeroRun = unpack_huffman_symbol_zero_length(pSymbol);
                                if( nZeroRun >= nWordCount ) {
                                    nZeroRun = (UINT16)(nZeroRun - nWordCount);
                                    break;					// found more zero's than we wanted, so break
                                }
                                nWordCount -= nZeroRun;		// else mark off these zeros as read
                                nZeroRun = 0;
                            }
                        }

                        p_band->prev_val[sideband] = nZeroRun;				/* keep track of outstanding zero's */
                    }
                    break;


                    case ENCODE_RANGE : {
                            register UINT	count;
                            // range encoder variables
                            register int ch;
                            register int syfreq;
                            int ltfreq;
                            register rangecoder *p_rc;
                            register qsmodel *p_qsm;

                            p_rc  = &(p_band->rc[sideband]);
                            p_qsm = &(p_band->qsm[sideband]);

                            // Throw away any leading unwanted symbols
                            count = p_info->pre_skip * 2;		// two bytes per symbols
                            while(count--) {
                                // Read and throw away some data
                                ltfreq = decode_culshift(p_rc,12);
                                ch = qsgetsym(p_qsm, ltfreq);
                                syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                                decode_update( p_rc, syfreq, ltfreq, 1<<12);
                                qsupdate(p_qsm,ch);
                            }

                            // read each symbol and write it to output
                            count = p_info->valid_line;

                            while(count--) {
                                register INT16	squant;		// MUST be 16 bits, so sign gets extended correctly

                                // bottom 8 bits
                                ltfreq = decode_culshift(p_rc,12);
                                ch = qsgetsym(p_qsm, ltfreq);
                                if (ch==256) { /* check for unexpected end-of-file */
                                    return(1);
                                }
                                squant = (INT16)(ch & 0x00ff);
                                syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                                decode_update( p_rc, syfreq, ltfreq, 1<<12);
                                qsupdate(p_qsm,ch);
                                // top 8 bits
                                ltfreq = decode_culshift(p_rc,12);
                                ch = qsgetsym(p_qsm, ltfreq);
                                squant |= (ch << 8) & 0xff00;
                                syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                                decode_update( p_rc, syfreq, ltfreq, 1<<12);
                                qsupdate(p_qsm,ch);

                                *p_output++ = ((IEEE4) squant) * fBinSize;
                            }


                            // Throw away any trailing unwanted symbols
                            count = p_info->post_skip * 2;		// two bytes per symbols
                            while(count--) {
                                // Read and throw away some data
                                ltfreq = decode_culshift(p_rc,12);
                                ch = qsgetsym(p_qsm, ltfreq);
                                syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                                decode_update( p_rc, syfreq, ltfreq, 1<<12);
                                qsupdate(p_qsm,ch);
                            }
                    }
                    break;

                    case ENCODE_RANGE8 : {
                            register UINT	count;
                            // range encoder variables
                            register int ch;
                            register INT16	prev;
                            register INT8	diff;
                            register int	syfreq;
                            int ltfreq;
                            register rangecoder *p_rc;
                            register qsmodel *p_qsm;

                            p_rc  = &(p_band->rc[sideband]);
                            p_qsm = &(p_band->qsm[sideband]);
                            prev  = p_band->prev_val[sideband];

                            // Throw away any leading unwanted symbols
                            count = p_info->pre_skip;		// ONE byte per symbols
                            while(count--) {
                                // unpack a byte
                                ltfreq = decode_culshift(p_rc,12);
                                ch = qsgetsym(p_qsm, ltfreq);
                                diff = (INT8) ch;		// we have to force it to signed first
                                prev = prev + diff;		// then we can add it safely
                                syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                                decode_update(p_rc, syfreq, ltfreq, 1<<12);
                                qsupdate(p_qsm,ch);
                            }

                            // read each symbol and write it to output
                            count = p_info->valid_line;

                            while(count--) {
                                ltfreq = decode_culshift(p_rc,12);
                                ch = qsgetsym(p_qsm, ltfreq);
                                if (ch==256) { /* check for unexpected end-of-file */
                                    return(1);
                                }
                                diff = (INT8) ch;		// we have to force it to signed first
                                syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                                decode_update( p_rc, syfreq, ltfreq, 1<<12);
                                qsupdate(p_qsm,ch);
                                prev = prev + (INT16) diff;		// then we can add it safely
                                *p_output++ = ((IEEE4) prev) * fBinSize;
                            }


                            // Throw away any trailing unwanted symbols
                            count = p_info->post_skip;		// ONE byte per symbols
                            while(count--) {
                                // unpack a byte
                                ltfreq = decode_culshift(p_rc,12);
                                ch = qsgetsym(p_qsm, ltfreq);
                                diff = (INT8) ch;		// we have to force it to signed first
                                prev = prev + diff;		// then we can add it safely
                                syfreq = qsgetfreq(p_qsm,ch,&ltfreq);
                                decode_update(p_rc, syfreq, ltfreq, 1<<12);
                                qsupdate(p_qsm,ch);
                            }
                            p_band->prev_val[sideband] = prev;
                    }
                    break;
                }	/* end subband loop */
            }
        }	/* end band loop */
    }	/* end block loop */
    return(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// HuffmanEncoder.cpp
//

/**
 * CNCSHuffmanCoder class - ECW Huffman Coder/Decoder.
 * 
 * @author       Simon Cope
 * @version      $Revision: 1.4 $ $Author: simon $ $Date: 2005/01/17 09:07:54 $ 
 */
class CNCSHuffmanCoder {
public:
    /*
    **	Huffman structures and definitions
    */
    class CTree;

    class CCodeNode {  
    public:
        typedef struct {
            class CCodeNode *m_p0Child;
            class CCodeNode *m_p1Child;
        } A;
        typedef union {
            A				m_P;
            class CCodeNode *m_Children[2];
        } U;
        U m_Children;

        NCSHuffmanSymbol m_Symbol;
        UINT	m_nFrequency;
        class CCodeNode	*m_pNext;  
        UINT 	m_nCode;
        UINT8	m_nCodeBits;
        bool	m_bInHistogram;

        CCodeNode();
        CCodeNode(UINT8 **ppPacked, UINT &nNodes);
        virtual ~CCodeNode();

        CCodeNode *Unpack(UINT8 **ppPacked, UINT &nNodes);
        void SetCode(UINT nCode, UINT8 nCodeBits);
    };
    class CTree: public CCodeNode {
    public:
        CCodeNode *m_pRoot;

        CTree();
        CTree(UINT8 **ppPacked);
        virtual ~CTree();

        NCSError Unpack(UINT8 **ppPacked);
    protected:
        CCodeNode *UnpackNode(UINT8 **ppPacked, UINT &nNodes);
    };

    CNCSHuffmanCoder();
    virtual ~CNCSHuffmanCoder();

    NCSError UnPack(UINT8 *pPacked, INT16 *pUnPacked, UINT nRawLength);
private:
    CTree *m_pTree;
};

/*const UINT NUM_SYMBOL_VALUES = 65536;
const UINT16 SIGN_MASK = NCS_HUFFMAN_SIGN_MASK;
const UINT16 RUN_MASK = NCS_HUFFMAN_RUN_MASK;
const INT16 MAX_BIN_VALUE = 16383;
const INT16 MIN_BIN_VALUE = -16383;
const UINT16 VALUE_MASK = NCS_HUFFMAN_VALUE_MASK;
const UINT16 MAX_RUN_LENGTH = NCS_HUFFMAN_MAX_RUN_LENGTH;
const UINT8 SMALL_SYMBOL = 0x40;
const UINT8 SMALL_SHIFT = 10;
*/

#define SMALL_SYMBOL 0x40
#define SMALL_SHIFT  10

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNCSHuffmanCoder::CCodeNode::CCodeNode() 
{
    m_Children.m_P.m_p0Child = NULL;
    m_Children.m_P.m_p1Child = NULL;

    memset(&m_Symbol, 0, sizeof(m_Symbol));

    m_nFrequency = 0;
    m_pNext = (CCodeNode*)NULL;  
    m_nCode = 0;
    m_nCodeBits = 0;
    m_bInHistogram = false;
}

CNCSHuffmanCoder::CCodeNode::CCodeNode(UINT8 **ppPacked, 
                                       UINT &nNodes) 
{
    Unpack(ppPacked, nNodes);
}

CNCSHuffmanCoder::CCodeNode::~CCodeNode() 
{
    delete m_Children.m_P.m_p0Child;
    delete m_Children.m_P.m_p1Child;
}

CNCSHuffmanCoder::CCodeNode *CNCSHuffmanCoder::CCodeNode::Unpack(UINT8 **ppPacked, 
                                                                 UINT &nNodes) 
{
    nNodes--;
    if(nNodes == 0) {
        return(NULL);
    }
    UINT8 nByte = (*(*ppPacked)++);

    if( nByte == 0 ) {
        m_Children.m_P.m_p0Child = new CCodeNode(ppPacked, nNodes);
        m_Children.m_P.m_p1Child = new CCodeNode(ppPacked, nNodes);
        memset(&m_Symbol, 0, sizeof(m_Symbol));
    }
    else {
        m_Children.m_P.m_p0Child = NULL;
        m_Children.m_P.m_p1Child = NULL;

        UINT16 nValue;
        if( nByte & SMALL_SYMBOL ) {
            nValue = (((UINT16)(nByte & 0x30)) << SMALL_SHIFT) | (nByte & 0x0f);
        }
        else {
            nValue = ((UINT16)**ppPacked) | (((UINT16)(*((*ppPacked) + 1))) << 8);
            *ppPacked += 2;
        }
        if(nValue & NCS_HUFFMAN_RUN_MASK) {
            m_Symbol.bZeroRun = TRUE;
            m_Symbol.nValue = (nValue & NCS_HUFFMAN_MAX_RUN_LENGTH) - 1;
        } else {
            m_Symbol.bZeroRun = FALSE;

            if(nValue & SIGN_MASK) {
#ifdef NCSBO_MSBFIRST
                m_Symbol.nValue = (INT16)NCSByteSwap16(((INT16)(nValue & VALUE_MASK)) * -1);
#else
                m_Symbol.nValue = ((INT16)(nValue & VALUE_MASK)) * -1;
#endif
            } else {
#ifdef NCSBO_MSBFIRST
                m_Symbol.nValue = (INT16)NCSByteSwap16((INT16)nValue);
#else
                m_Symbol.nValue = ((INT16)nValue);
#endif
            }
        }
    }
    return (this);
}

void CNCSHuffmanCoder::CCodeNode::SetCode(UINT nCode, UINT8 nCodeBits)
{
    if(m_Children.m_P.m_p0Child) {
        m_Children.m_P.m_p0Child->SetCode((nCode << 1), nCodeBits + 1);
        m_Children.m_P.m_p1Child->SetCode((nCode << 1) | 0x1, nCodeBits + 1);
    } else {
        m_nCode = nCode;
        m_nCodeBits = nCodeBits;
    }
}

CNCSHuffmanCoder::CTree::CTree() 
{
    m_pRoot = NULL;
}

CNCSHuffmanCoder::CTree::CTree(UINT8 **ppPacked) 
{
    m_pRoot = NULL;
    Unpack(ppPacked);
}

CNCSHuffmanCoder::CTree::~CTree()
{
    delete m_pRoot;
}

NCSError CNCSHuffmanCoder::CTree::Unpack(UINT8 **ppPacked)
{
    UINT nNodes = (((UINT)**ppPacked) | (((UINT)(*((*ppPacked) + 1))) << 8)) + 1;
    *ppPacked += 2;
    m_pRoot = new CCodeNode(ppPacked, nNodes);
    if(m_pRoot) {
        return(NCS_SUCCESS);
    } else {
        return(NCS_COULDNT_ALLOC_MEMORY);
    }
}

// Constructor
CNCSHuffmanCoder::CNCSHuffmanCoder()
{
    m_pTree = NULL;
}

// Destructor
CNCSHuffmanCoder::~CNCSHuffmanCoder()
{
    delete m_pTree;
}

// UnParse the marker out to the stream.
NCSError CNCSHuffmanCoder::UnPack(UINT8 *pPacked,
                                   INT16 *pUnPacked,
                                   UINT nRawLength)
{
    register UINT	nWordCount = nRawLength / 2;
    register INT16 *pOutput = (INT16*)pUnPacked;
    register UINT nBitsUsed = 0;

    m_pTree = new CTree(&pPacked);

    if(!m_pTree) {
        return(NCS_COULDNT_ALLOC_MEMORY);
    }
    
    while(nWordCount--) {
        // Decode a packed Huffman value
        CCodeNode *pNode = m_pTree->m_pRoot;
        while (pNode->m_Children.m_P.m_p0Child != NULL) {
            pNode = pNode->m_Children.m_Children[(pPacked[nBitsUsed >> 3] >> (nBitsUsed & 0x7)) & 0x1];
            nBitsUsed++;
        }
                 
        if(pNode->m_Symbol.bZeroRun) {
            register UINT16 nZero;
            register UINT16	nZeroRun = pNode->m_Symbol.nValue;
            
            if( nZeroRun >= nWordCount ) {
                nZero = (UINT16)nWordCount + 1;	
                nWordCount = 0;			
            } else {
                nZero = nZeroRun + 1;
                nWordCount -= nZeroRun;		
            }
            memset(pOutput, 0, nZero * sizeof(INT16));
            pOutput += nZero;
        } else {
            *pOutput++ = pNode->m_Symbol.nValue;
        }
    }
    delete m_pTree;
    m_pTree = NULL;
    return(NCS_SUCCESS);
}

NCSError unpack_huffman(UINT8 *pPacked, 
                                  INT16 *pUnPacked,
                                  UINT nRawLength)
{
    CNCSHuffmanCoder HC;
    return HC.UnPack(pPacked, pUnPacked, nRawLength);
    //return NCS_SUCCESS;
}

void unpack_huffman_init_state(NCSHuffmanState *pState, UINT8 **ppPacked)
{
    pState->pTree = (void*)new CNCSHuffmanCoder::CTree(ppPacked);
    pState->nBitsUsed = 0;
}

void unpack_huffman_fini_state(NCSHuffmanState *pState)
{
    delete (CNCSHuffmanCoder::CTree*)pState->pTree;
    pState->pTree = (void*)NULL;
    pState->nBitsUsed = 0;
}

NCSHuffmanSymbol *unpack_huffman_symbol(UINT8 **ppPacked,
                                                   NCSHuffmanState *pState)
{
    register UINT nBitsUsed = pState->nBitsUsed;
    register CNCSHuffmanCoder::CCodeNode *pNode = ((CNCSHuffmanCoder::CTree*)pState->pTree)->m_pRoot;
    register UINT8 *pEncoded = *ppPacked;

    while (pNode->m_Children.m_P.m_p0Child != 0) {
        pNode = pNode->m_Children.m_Children[(pEncoded[nBitsUsed >> 3] >> (nBitsUsed & 0x7)) & 0x1];
        nBitsUsed++;
    }

    pState->nBitsUsed = nBitsUsed;
    return(&pNode->m_Symbol);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
//
// Stubs.
//

int NCScbmOpenFileView(char *szUrlPath, void **ppNCSFileView)
{
    NCSError Error;
    NCSFileView *pECWFileView = NULL;

    Error = NCScbmOpenFileView_ECW(szUrlPath, &pECWFileView);
    *ppNCSFileView = (void *)pECWFileView;
    return (int)Error;
}

int NCScbmGetViewFileInfo(void *pNCSFileView, NCSFileViewFileInfo **ppNCSFileViewFileInfo)
{
    return (int)NCScbmGetViewFileInfo_ECW((NCSFileView *)pNCSFileView, ppNCSFileViewFileInfo);
}

int NCScbmSetFileView(void *pNCSFileView,
                UINT nBands,					// number of bands to read
                UINT *pBandList,				// index into actual band numbers from source file
                UINT nTopX, UINT nLeftY,	// Top-Left in image coordinates
                UINT nBottomX, UINT nRightY,// Bottom-Left in image coordinates
                UINT nSizeX, UINT nSizeY)	// Output view size in window pixels
{
    return (int)NCScbmSetFileViewEx_ECW((NCSFileView *)pNCSFileView,
                               nBands,
                               pBandList,
                               nTopX, nLeftY,
                               nBottomX, nRightY,
                               nSizeX, nSizeY,
                               nTopX, nLeftY,
                               nBottomX, nRightY);
}

NCSEcwReadStatus NCScbmReadViewLineRGBA( void *pNCSFileView, UINT *pRGBTriplets)
{
    if( erw_decompress_read_region_line(((NCSFileView *)pNCSFileView)->pQmfRegion, (UINT8 *)pRGBTriplets) )
        return(NCSECW_READ_FAILED);
    else
        return(NCSECW_READ_OK);
}

int NCScbmCloseFileView(void *pNCSFileView)
{
    return (int)NCScbmCloseFileView_ECW((NCSFileView *)pNCSFileView);
}
