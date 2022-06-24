#pragma once

/** 
 *	@enum
 *  Enumerated type for the cell sizes supported by the SDK.
 */
typedef enum {
	/** Invalid cell units */
	ECW_CELL_UNITS_INVALID	=	0,
	/** Cell units are standard meters */
	ECW_CELL_UNITS_METERS	=	1,
	/** Degrees */
	ECW_CELL_UNITS_DEGREES	=	2,
	/** US Survey feet */
	ECW_CELL_UNITS_FEET		=	3,
	/** Unknown cell units */
	ECW_CELL_UNITS_UNKNOWN	=	4
} CellSizeUnits;

/** 
 *	@struct
 *	Structure containing file metadata for the ECW interface.
 *	SDK method NCScbmGetViewFileInfo returns a pointer to this file info structure for a view
 */
typedef struct {
	/** Dataset cells in X direction */
	UINT	nSizeX;
	/** Dataset cells in Y direction */
	UINT	nSizeY;			
	/** Number of bands in the file, e.g. 3 for a RGB file */
	UINT16	nBands;
	/** Target compression rate, e,g, 20 == 20:1 compression.  May be zero */
	UINT16	nCompressionRate;
	/** Units used for pixel size */
	CellSizeUnits	eCellSizeUnits;
	/** Increment in eCellSizeUnits in X direction.  May be negative.  Never zero */
	IEEE8	fCellIncrementX;
	/** Increment in eCellSizeUnits in Y direction.  May be negative.  Never zero */
	IEEE8	fCellIncrementY;
	/** World X origin for top left corner of top left cell, in eCellSizeUnits */
	IEEE8	fOriginX;
	/** World Y origin for top left corner of top left cell, in eCellSizeUnits */
	IEEE8	fOriginY;
	/** ER Mapper style Datum name string, e.g. "RAW" or "NAD27".  Never NULL */
	char	*szDatum;
	/** ER Mapper style Projection name string, e.g. "RAW" or "GEODETIC".  Never NULL */
	char	*szProjection;
} NCSFileViewFileInfo;

/** 
 *	Initialise the SDK libraries.  Should not be called directly unless you are directly including 
 *	the SDK code rather than linking against the DLL. 
 */
extern void NCSecwInit(void);		// DO NOT call if linking against the DLL
/** 
 *	Initialise the SDK libraries.  Should not be called directly unless you are directly including 
 *	the SDK code rather than linking against the DLL. 
 */
extern void NCSecwShutdown(void);	// DO NOT call if linking against the DLL

/** 
 *	Opens a file view.  After calling this function, call GetViewFileInfo to obtain file metadata
 *
 *	@param[in]	szUrlPath			The location of the file on which to open a view
 *	@param[out]	ppNCSFileView		The NCSFileView structure to initialise
 *	@return							NCSError value, NCS_SUCCESS or any applicable error code
 */
extern int NCScbmOpenFileView(char *szUrlPath, void **ppNCSFileView);

/** 
 *	Populates a structure with information about an open image file.  Use this version when dealing with ECW files only.
 *
 *	@param[in]	pNCSFileView			The file view open on the file whose metadata is being obtained
 *	@param[out]	ppNCSFileViewFileInfo	A pointer to a pointer to the NCSFileViewFileInfo struct to populate with the metadata
 *	@return								NCSError value, NCS_SUCCESS or any applicable error code
 */
extern int NCScbmGetViewFileInfo(void *pNCSFileView, NCSFileViewFileInfo **ppNCSFileViewFileInfo);

/**	
 *	Closes a file view.  This can be called at any time after NCScbmOpenFileView is called to clean up an open file view.
 *
 *	@param[in]	pNCSFileView		The file view to close
 *	@return							NCSError value, NCS_SUCCESS or any applicable error code
 */
extern int NCScbmCloseFileView(void *pNCSFileView);

/** 
 *	Sets the extents and band content of an open file view, and the output view size.  This function can be called at 
 *	any time after a successful call to NCScbmOpenFileView.  In progressive mode, multiple calls to NCScbmSetFileView 
 *	can be made, even if previous SetViews are still being processed, enhancing client interaction with the view.  After 
 *	the call to NCScbmSetFileView, the band list array pBandList can be freed if so desired.  It is used only during the 
 *  processing of the call, and not afterward.
 *
 *	@param[in]	pNCSFileView			The open file view to set
 *	@param[in]	pBandList				An array of integers specifying which bands of the image to read, and in which order
 *	@param[in]	nTLX					Left edge of the view in dataset cells
 *	@param[in]	nTLY					Top edge of the view in dataset cells
 *	@param[in]	nBRX				Right edge of the view in dataset cells
 *	@param[in]	nBRY					Bottom edge of the view in dataset cells
 *	@param[in]	nSizeX					Width of the view to be constructed from the image subset
 *	@param[in]	nSizeY					Height of the view to be constructed from the image subset
 *	@return								NCSError value, NCS_SUCCESS or any applicable error code
 */
extern int NCScbmSetFileView(void *pNCSFileView,
										   UINT nBands,					
										   UINT *pBandList,				
										   UINT nTLX, UINT nTLY,	
										   UINT nBRX, UINT nBRY,
										   UINT nSizeX, UINT nSizeY);	

/** 
 *	@enum
 *  Enumerated type for the return status from read line routines.
 *	The application should treat CANCELLED operations as non-fatal,
 *	in that they most likely mean this view read was cancelled for
 *	performance reasons.
 */
typedef enum {
	/** Successful read */
	NCSECW_READ_OK			= 0,
	/** Read failed due to an error */
	NCSECW_READ_FAILED		= 1,
	/** Read was cancelled, either because a new SetView arrived or a 
	    library shutdown is in progress */
	NCSECW_READ_CANCELLED	= 2	
} NCSEcwReadStatus;

/**
 *	Read line by line in RGBA format.  Samples are read into a buffer of UINT values, each value comprising
 *	the four bytes of a red-green-blue-alpha sample.  Alpha values will be zero if the input file is in ECW
 *	format as this format does not 'understand' alpha channels.  SDK programmers wanting to compress and 
 *	decompress data in four bands are advised to use multiband compression and NCScbmReadViewLineBil(Ex) to 
 *	handle their data.
 *
 *	@param[in]	pNCSFileView			The open file view from which to read view lines
 *	@param[out]	pRGBA					The buffer of packed UINT values.
 *	@return								NCSEcwReadStatus value, NCSECW_READ_OK or any applicable error code
 */
NCSEcwReadStatus NCScbmReadViewLineRGBA( void *pNCSFileView, UINT *pRGBA);
