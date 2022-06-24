#pragma once

#include "pch.h"
#include "NCSDefs.h"
#include "ECW.h"

/** 
 *	@struct
 *	Struct containing metadata for a specific band in the file.
 */
typedef struct {
	/** Bit depth used in band, including sign bit */
	UINT8	nBits;
	/** Whether band data is signed */
	BOOLEAN	bSigned;
	/** ASCII description of band, e.g. "Red" or "Band1" */
	const char *szDesc;
} NCSFileBandInfo;

/** 
 *	@defgroup banddescs
 *	These are the allowable ASCII descriptions in the current implementation.
 *  @{
 */
/** @def */
#define NCS_BANDDESC_Red							"Red"
/** @def */
#define NCS_BANDDESC_Green							"Green"
/** @def */
#define NCS_BANDDESC_Blue							"Blue"
/** @def */
#define NCS_BANDDESC_All							"All"
/** @def */
#define NCS_BANDDESC_RedOpacity						"RedOpacity"
/** @def */
#define NCS_BANDDESC_GreenOpacity					"GreenOpacity"
/** @def */
#define NCS_BANDDESC_BlueOpacity					"BlueOpacity"
/** @def */
#define NCS_BANDDESC_AllOpacity						"AllOpacity"
/** @def */
#define NCS_BANDDESC_RedOpacityPremultiplied		"RedOpacityPremultiplied"
/** @def */
#define NCS_BANDDESC_GreenOpacityPremultiplied		"GreenOpacityPremultiplied"
/** @def */
#define NCS_BANDDESC_BlueOpacityPremultiplied		"BlueOpacityPremultiplied"
/** @def */
#define NCS_BANDDESC_AllOpacityPremultiplied		"AllOpacityPremultiplied"
/** @def */
#define NCS_BANDDESC_Greyscale						"Grayscale"
/** @def */
#define NCS_BANDDESC_GreyscaleOpacity				"GrayscaleOpacity"
/** @def */
#define NCS_BANDDESC_GreyscaleOpacityPremultiplied	"GrayscaleOpacityPremultiplied"
/** @def */
#define NCS_BANDDESC_Band							"Band #%d"
/*@}*/

/** 
 *	@enum
 *	The color space used by a compressed file.
 *	For compatibility with ECW, these values cannot be changed or reordered.
 */
typedef enum {
	/** No color space */
	NCSCS_NONE						= 0,
	/** Greyscale image */
	NCSCS_GREYSCALE					= 1,	// Greyscale
	/** Luminance-chrominance color space */
	NCSCS_YUV						= 2,	// YUV - JPEG Digital, JP2 ICT
	/** Multiband image */
	NCSCS_MULTIBAND					= 3,	// Multi-band imagery
	/** sRGB color space */
	NCSCS_sRGB						= 4,	// sRGB
	/** Modified luminance-chrominance color space */
	NCSCS_YCbCr						= 5		// YCbCr - JP2 ONLY, Auto-converted to sRGB
} NCSFileColorSpace;

/** 
 *	@struct
 *	Extended file metadata structure for the JPEG 2000 interface.
 *	This structure is derived from a compressed JPEG 2000 file.
 *	It is important to note that the information contained within it 
 *	is informative, not normative.  For example, although the file may 
 *	contain metadata that indicates the image it contains is to be rotated,
 *	the SDK will not rotate that image itself.
 *	The SDK function NCScbmGetViewFileInfoEx() returns a pointer to this 
 *	file info structure for a given view.
 */
typedef struct {		
	/** Dataset cells in X direction */	
	UINT	nSizeX;
	/** Dataset cells in X direction */
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
	/** Clockwise rotation of image in degrees */
	IEEE8	fCWRotationDegrees;
	/** Color space of image */
	NCSFileColorSpace eColorSpace;
	/** Cell type of image samples */
	NCSEcwCellType eCellType;
	/** A pointer to an array of band information structures for each band of the image */
	NCSFileBandInfo *pBands;
} NCSFileViewFileInfoEx;

/** 
 *	@struct
 *	Information about an open view into a compressed image file.
 *	This structure contains updated information about the extents and processing status
 *	of an open file view.  NCScbmGetViewSetInfo() will return a pointer to this structure 
 *	for a file view
 */
typedef struct {
	/** Client data */
	void	*pClientData;
	/** Number of bands to read */
	UINT nBands;				
	/** Array of band indices being read from the file - the size of this array is nBands */
	UINT *pBandList;
	/** Top left of the view in image coordinates */
	UINT nTopX, nLeftY;
	/** Bottom right of the view in image coordinates */
	UINT nBottomX, nRightY;
	/** Size of the view in pixels */
	UINT nSizeX, nSizeY;			
	/** Number of file blocks within the view area */
	UINT nBlocksInView;
	/** Number of these file blocks that are currently available */
	UINT nBlocksAvailable;
	/** Blocks of the file that were available at the time of the corresponding SetView */
	UINT nBlocksAvailableAtSetView;
	/** Number of blocks that were missed during the read of this view */
	UINT nMissedBlocksDuringRead;
	/** Top left of the view in world coordinates (if using SetViewEx) */
	IEEE8  fTopX, fLeftY;
	/** Bottom right of the view in world coordinates (if using SetViewEx) */ /*[02]*/
	IEEE8  fBottomX, fRightY;	
} NCSFileViewSetInfo;

typedef struct qmf_level_struct QmfLevelStruct;

/** 
 *	@typedef
 *	This type definition promotes properly transparent usage of the SDK structures.
 */
typedef struct NCSFileStruct NCSFile;
/** 
 *	@typedef
 *	This type definition promotes properly transparent usage of the SDK structures.
 */
typedef struct NCSFileViewStruct NCSFileView;

extern void NCSFreeFileInfoEx(NCSFileViewFileInfoEx *pDst);

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
NCSEcwReadStatus NCScbmReadViewLineRGBA( NCSFileView *pNCSFileView, UINT *pRGBA);

/**	
 *	Purge the memory cache being used by the SDK for the current file view.
 *
 *	@param[in]	pView		The file view which should have its cache purged
 */

extern void NCScbmPurgeCache(NCSFileView *pView);

extern NCSError NCScbmOpenFileView_ECW(char *szUrlPath, NCSFileView **ppNCSFileView);
extern NCSError NCScbmCloseFileView_ECW(NCSFileView *pNCSFileView);
extern NCSError NCScbmCloseFileViewEx_ECW(NCSFileView *pNCSFileView, BOOLEAN bFreeCachedFile);	/**[03]**/
extern NCSError NCScbmGetViewFileInfo_ECW(NCSFileView *pNCSFileView, NCSFileViewFileInfo **ppNCSFileViewFileInfo);
extern NCSError NCScbmGetViewFileInfoEx_ECW(NCSFileView *pNCSFileView, NCSFileViewFileInfoEx **ppNCSFileViewFileInfo);
extern NCSError NCScbmGetViewInfo_ECW(NCSFileView *pNCSFileView, NCSFileViewSetInfo **ppNCSFileViewSetInfo);
extern NCSError NCScbmSetFileView_ECW(NCSFileView *pNCSFileView,
				UINT nBands,					// number of bands to read
				UINT *pBandList,				// index into actual band numbers from source file
				UINT nTLX, UINT nTLY,	// Top-Left in image coordinates
				UINT nBRX, UINT nBRY,// Bottom-Left in image coordinates
				UINT nSizeX, UINT nSizeY);	// Output view size in window pixels

extern NCSError NCScbmSetFileViewEx_ECW(NCSFileView *pNCSFileView,		/*[02]*/
				UINT nBands,					// number of bands to read
				UINT *pBandList,				// index into actual band numbers from source file
			    UINT nTLX, UINT nTLY,	// Top-Left in image coordinates
				UINT nBRX, UINT nBRY,// Bottom-Right in image coordinates
				UINT nSizeX, UINT nSizeY,	// Output view size in window pixels
				IEEE8 fWorldTLX, IEEE8 fWorldTLY,		// Top-Left in world coordinates
				IEEE8 fWorldBRX, IEEE8 fWorldBRY);	// Bottom-Right in world coordinates
NCSEcwReadStatus NCScbmReadViewLineRGBA_ECW( NCSFileView *pNCSFileView, UINT *pRGBA);
extern NCSError NCSecwOpenFile_ECW(NCSFile **ppNCSFile,
									char *szInputFilename,		// input file name or network path
									BOOLEAN bReadOffsets,		// TRUE if the client wants the block Offset Table
									BOOLEAN bReadMemImage);		// TRUE if the client wants a Memory Image of the Header
extern int	NCSecwCloseFile_ECW( NCSFile *pNCSFile);
extern int	NCSecwReadLocalBlock_ECW( NCSFile *pNCSFile, UINT64 nBlockNumber, UINT8 **ppBlock, UINT *pBlockLength);
UINT8	*NCScbmReadFileBlockLocal_ECW(NCSFile *pNCSFile, NCSBlockId nBlock, UINT *pBlockLength );
extern BOOLEAN NCScbmGetFileBlockSizeLocal_ECW(NCSFile *pNCSFile, NCSBlockId nBlock, UINT *pBlockLength, UINT64 *pBlockOffset );

/** Internal initialisation functions.  These should not be called by user applications.*/
void NCSecwInitInternal();
void NCSecwShutdownInternal();

//
// CompressClient.
//
#define ERSWAVE_VERSION 2

/** 
 *  @enum
 *	An enumerated type specifying the format of the compressed data. Currently greyscale, RGB
 *	and luminance-chrominance formats are supported.  This will later need expanding to include 
 *	multiband formats such as CMYK.
 */
typedef enum {
	/** The compressed data is unformatted */
	COMPRESS_NONE		= NCSCS_NONE,
	/** Greyscale format, single band */
	COMPRESS_UINT8		= NCSCS_GREYSCALE,
	/** JPEG standard YUV digital format, three band */
	COMPRESS_YUV		= NCSCS_YUV,
	/** Multiband format */
	COMPRESS_MULTI		= NCSCS_MULTIBAND,
	/** RGB images (converted to COMPRESS_YUV internally) */
	COMPRESS_RGB		= NCSCS_sRGB			
} CompressFormat;


/** @def Maximum length of a datum definition string */
#define ECW_MAX_DATUM_LEN		16
#ifndef ECW_MAX_PROJECTION_LEN
/** @def Maximum length of a projection definition string */
#define ECW_MAX_PROJECTION_LEN	16
#endif

typedef struct {
	NCS_FILE_HANDLE	hFile;
	void	*pClientData;
} ECWFILE;

#define NULL_ECWFILE	{ NCS_NULL_FILE_HANDLE, NULL }

typedef UINT16 uint2;
typedef UINT uint4;

#define ECW_HEADER_ID_TAG   'e'				/* [02] first byte of a compressed file must be this */

/* Maximum number of pyramid levels.
** We want enough room for terrabyte sized files
** This doesn't need to be larger than log2(MAX_IMAGE_DIM/FILTER_SIZE).
*/
#define MAX_LEVELS 20

/*
**	QMF sideband images are sub-blocked, so that sub-sections of sideband images
**	can be retrieved without having to read the entire sideband image.
**	So a block will be BLOCK_SIZE_X * BLOCK_SIZE_Y symbols in size.
**	The total must be less than or equal to 64K result (to ensure that encoders
**	have a fixed maximum to encode).  Smaller blocks means faster load time over
**	internet (for NCS technology), but more inefficient compression and overhead.
**	The best size is 1024(x)x64(y), which is fast, good compression, and small size.
**	Note that the compressor can compress an entire file without blocking; this
**	is a convenience definition related to decompression of images, it is not
**	essential for operation of the compressor.
**
**	The X block size is typically greater than the Y block size. This is because
**	once a block is found, it is fast to read it, and because increasing Y block
**	size increases the amount of memory required to store blocks prior to encoding
**	Both numbers must be even numbers.
**
**  [07] Note: The ENCODE_RUN_ZERO decoder currently limits block size in X to 2^15
**	in size - which should never be a problem as this is a huge value for a single
**	block, but is something  to be aware of. The RUN_ZERO unpack code is easily enhanced for larger
**	block sizes, at a slight expense in performance.
**
**	[10] Block size can now be set during compression via command line. This is the default.
*/

// Made smaller, for fast NCS initial loads
#define	MIN_QMF_SIZE	64		// minimum size for the smallest LL


/* Use these to set up nr_[x|y]_blocks in the qmf, then use those values */

#define QMF_LEVEL_NR_X_BLOCKS(p_qmf)	\
			(((p_qmf)->x_size + ((p_qmf)->x_block_size - 1)) / (p_qmf)->x_block_size)

#define QMF_LEVEL_NR_Y_BLOCKS(p_qmf)	\
			(((p_qmf)->y_size + ((p_qmf)->y_block_size - 1)) / (p_qmf)->y_block_size)

// size of filter tap bank, must be an odd number 3 or greater
// Filters are hardcode as defines, making multiplications later much faster.
// Compiler will detect and optimize symetric numbers

#define FILTER_SIZE 11			// 11 tap filter banks

// Lowpass sums to 1.0
#define	LO_FILTER_0		(IEEE4)  0.007987761489921101
#define	LO_FILTER_1		(IEEE4)  0.02011649866148413
#define	LO_FILTER_2		(IEEE4) -0.05015758257647976
#define	LO_FILTER_3		(IEEE4) -0.12422330961337678
#define	LO_FILTER_4		(IEEE4)  0.29216982108655865
#define	LO_FILTER_5		(IEEE4)  0.7082136219037853
#define	LO_FILTER_6		(IEEE4)  0.29216982108655865
#define	LO_FILTER_7		(IEEE4) -0.12422330961337678
#define	LO_FILTER_8		(IEEE4) -0.05015758257647976 
#define	LO_FILTER_9		(IEEE4)  0.02011649866148413
#define	LO_FILTER_10	(IEEE4)  0.007987761489921101
// Highpass sums to 0.0
#define	HI_FILTER_0		(IEEE4) -0.007987761489921101
#define	HI_FILTER_1		(IEEE4)  0.02011649866148413
#define	HI_FILTER_2		(IEEE4)  0.05015758257647976
#define	HI_FILTER_3		(IEEE4) -0.12422330961337678
#define	HI_FILTER_4		(IEEE4) -0.29216982108655865
#define	HI_FILTER_5		(IEEE4)  0.7082136219037853
#define	HI_FILTER_6		(IEEE4) -0.29216982108655865
#define	HI_FILTER_7		(IEEE4) -0.12422330961337678
#define	HI_FILTER_8		(IEEE4)  0.05015758257647976 
#define	HI_FILTER_9		(IEEE4)  0.02011649866148413
#define	HI_FILTER_10	(IEEE4) -0.007987761489921101

/* The type of the quantized images. Must be SIGNED, and capable of holding    values  in the range [-MAX_BINS, MAX_BINS] */
typedef INT16 BinIndexType;  

/* The type used to represent the binsizes. Should be UNSIGNED. If this is
   changed, be sure to change the places where 
   binsizes are written or read from files.  */
typedef UINT16 BinValueType;

typedef UINT8 Byte;

/*
** Number of possible values for a symbol.  This must be at least
**(MAX_BINS * 4)  (one sign bit, one tag bit)...
*/
#define NUM_SYMBOL_VALUES 65536
#define SIGN_MASK	0x4000
#define RUN_MASK	0x8000
#define MAX_BIN_VALUE	16383	// 0x3fff
#define MIN_BIN_VALUE	-16383	// Prior to masking
#define VALUE_MASK	0x3fff		/* without the sign or run bits set */
#define MAX_RUN_LENGTH 0x7fff	/* can't be longer than this or would run into mask bits */

/*
**	The blocking format used for a file. Currently only one supported -
**	block each level.
*/

typedef enum {
	BLOCKING_LEVEL	= 1
} BlockingFormat;

/*
**	The encoding format for a sideband within a block. More encoding
**	standards might be added later.
*/
typedef UINT16 EncodeFormat;
#define	ENCODE_INVALID	0
#define	ENCODE_RAW		1
#define	ENCODE_HUFFMAN	2
#define	ENCODE_RANGE	3
#define	ENCODE_RANGE8	4
#define	ENCODE_ZEROS	5
#define	ENCODE_RUN_ZERO	6

#define MAX_SIDEBAND	4

typedef enum {
	LL_SIDEBAND = 0,	// must be in order as used for array offsets
	LH_SIDEBAND = 1,
	HL_SIDEBAND = 2,
	HH_SIDEBAND = 3
} Sideband;

/*
**	Some basic information about a QMF file, pointed to by all levels,
**	designed to make life easier for higher level applications.
**	NOTE!! NCSECWClient.h has this structure, defined as "NCSFileViewFileInfo".
**	Any changes must be kept in synch between the two structures
**
*/

typedef NCSFileViewFileInfo ECWFileInfo;
typedef NCSFileViewFileInfoEx ECWFileInfoEx;

/*
**
**	The QmfLevelBandStruct is only allocated for COMPRESSION. Because the bin size is
**	needed for decompression, it is pulled out of the band structure (binsize can be
**	different for each band) and put into the main level structure. So the bin_size
**	is the only multi-band value in the QMF level structure; the rest of the band
**	specific information is held here at the band level. This also makes the QMF
**	structure much smaller for decompression.
**
**	There is a multi-band, compression only, buffer allocated at the QMF level,
**	which is a pointer to the p_input_ll_line for all bands. This is so recursion,
**	including the top level, is easier to structure.
*/

typedef struct qmf_level_band_struct {
	//
	// These buffers are not allocated for the largest (file level) QMF level
	//
	IEEE4	*p_p_lo_lines[FILTER_SIZE+1];	// pointer to enough larger input lines for lowpass input
	IEEE4	*p_p_hi_lines[FILTER_SIZE+1];	// pointet to enough larger input lines for highpass input
	IEEE4	*p_low_high_block;				// the block of memory indexed into by the above. Used for free()
	// this is allocated with enough room on the X sides to handle reflection
	IEEE4	*p_input_ll_line;				// pointer to a single input line needed for this level

	// This points to the Y_BLOCK_SIZE buffer of output lines, quantized, stored for
	// LL (smallest level 0 only), LH, HL and HH. The index into the block for a given line
	// is (for maximum memory access performance):
	// (y_line_offset * x_line_size)
		/* These are used when p_file_qmf->bLowMemCompress is TRUE [20]*/
	UINT **p_p_ll_lengths;
	UINT8 ***p_p_p_ll_segs;
	UINT **p_p_lh_lengths;
	UINT8 ***p_p_p_lh_segs;
	UINT **p_p_hl_lengths;
	UINT8 ***p_p_p_hl_segs;
	UINT **p_p_hh_lengths;
	UINT8 ***p_p_p_hh_segs;
		/* These are used when p_file_qmf->bLowMemCompress is TRUE (1 line) or FALSE (Y_BLOCK_SIZE lines) [20]*/
	INT16	*p_quantized_output_ll_block;	// Only allocated for level 0 LL, NULL for all other levels
	INT16	*p_quantized_output_lh_block;	// LH block
	INT16	*p_quantized_output_hl_block;	// HL block
	INT16	*p_quantized_output_hh_block;	// HH block

	UINT	packed_length[MAX_SIDEBAND];	// the packed length for a single set of sidebands for this band
} QmfLevelBandStruct;

/*
**	One level of a QMF tree
**
** The QMF levels are as follows:
**	QMF level (level 0..file_level-1)
**		Normal QMF levels. Contain temporary buffers for line processing, and a temporary file
**		pointer for writing the compressed sidebands to.
**	File level:
**		A place-holder level. Contains no buffers, but does contain information about the
**		file to be read from.
*/

struct qmf_level_struct {
	UINT16	level;
	UINT8	nr_levels;			// Number of levels, excluding file level
	UINT8	version;			// [13] Top-level only: compressed file version
	UINT8	nr_sidebands;		// [13] Top-level only: set to MAX_SIDEBANDS during write of a file
	UINT8	bPAD1;				// [13] pad to long-word
	UINT16	nr_bands;			// [13] number of bands (not subbands) in the file, e.g. 3 for a RGB file
	UINT	x_size, y_size;
	UINT	next_output_line;	// COMPRESSION ONLY: next output line to construct. Will go from 0 to x_size - 1
	UINT	next_input_line;	// COMPRESSION ONLY: next line of input to read. 2 lines read for every output line
	struct	qmf_level_struct	*p_larger_qmf; 
	struct	qmf_level_struct	*p_smaller_qmf;
	struct	qmf_level_struct	*p_top_qmf;		/* pointer to top (smallest) level QMF */
	struct	qmf_level_struct	*p_file_qmf;	/* pointer to the fake (largest) file level QMF */

	UINT	*p_band_bin_size;			/* bin size for each band during quantization. Each sideband for a band has same binsize */
	UINT16	x_block_size;				/* size of a block in X direction */
	UINT16	y_block_size;				/* size of a block in Y direction */
	UINT	nr_x_blocks;				/* number of blocks in the X direction */
	UINT	nr_y_blocks;				/* number of blocks in the Y direction */
	UINT	nFirstBlockNumber;			/* [02] First block number for this level, 0 for level 0 */

	// These values are valid for the top (smallest) level QMF only
	BlockingFormat blocking_format;		// typically BLOCKING_LEVEL
	CompressFormat compress_format;		// typically COMPRESS_UINT8 or COMPRESS_YIQ
	IEEE4	compression_factor;			// a number between 1 and 1000


	// These are SHARED entries by all QMFs. They all point to the same data,
	// and only the top level QMF allocates and frees these. The p_a_block points
	// to a single block (valid compression only);
	// all QMF's point to the same block. The p_block_offsets is
	// an single array for all levels. It is allocated at p_top_qmf, and freed
	// at p_top_qmf only.  The other levels point to offsets in the array
	// There is +1 block than actually present, to calculate the final block offset.
	// These offsets are relative to the start of the first block, not the start of the file
	UINT64	first_block_offset;			/* offset from the start of the file to the first block */	
	UINT64	*p_block_offsets;			/* offset for each block for this level of the file */
	NCS_FILE_HANDLE	outfile;					/* output file */
	UINT	next_block_offset;			/* incremented during writing */
	UINT8	*p_a_block;					/* pointer to a single block during packing/unpacking */
	BOOLEAN bRawBlockTable;				/* used to indicated weather the block table was store as
										   RAW or compressed.  Required when freeing the memory
										   allocated for the Block Table in delete_qmf_levels */ //[19]
	BOOLEAN	bLowMemCompress;			/* Use "Low Memory" compression techniques [20]*/

	// COMPRESSION ONLY: nr_bands of the following pointers, which point into the
	// p_input_ll_line offset at [FILTERSIZE/2] for this QMF. Used for recursive calls. 
	IEEE4	**p_p_input_ll_line;		// pointer to nr_bands, a single input line needed for this level
	//
	// These buffers are not allocated for the largest (file level) QMF level
	// These are also ONLY allocated for compression, not for decompression. There is one
	// per band being compressed
	QmfLevelBandStruct	*p_bands;
	INT16	next_output_block_y_line;	// a number going from 0 to Y_BLOCK_SIZE-1
	//	The place to write the compressed sidebands for this level to. Each level has MAX_SIDEBAND files
	//	We don't encode the LL sideband image, so the [LL_SIDEBAND] item is normally unused except for debug
	char	*tmp_fname;					// temporary file to write this level's blocks, so we can set up block order
	NCS_FILE_HANDLE	tmp_file;					// file pointer to the above block image disk file

	//	if we are doing transmission encoding instead of blocked encoding
	IEEE4		scale_factor;			// scale factor, for decompression only
	UINT64		file_offset;			// offset to start of this level in the file

	//	[02] Addition information for NCS usage. Valid for the TopQmf (smallest level) only
	UINT8	*pHeaderMemImage;		// Pointer to in memory copy of header if not NULL
	UINT	nHeaderMemImageLen;		// Length of the above in memory header structure

	ECWFILE	hEcwFile;				// input ECW, TopQmf level only, for decompression only
	BOOLEAN	bEcwFileOpen;			// TRUE if above file handle is valid (open), otherwise FALSE

	ECWFileInfoEx	*pFileInfo;			// Valid at top level only
};

//
//	Region to decode/decompress within a compressed file for a given line.
//
//
// For a region, we keep local line buffers for each level within the region's depth. There
// may be less of these than QMF's, if we are extracting a reduced resolution view of the data.
//
// The upsampling logic results in 4x4 output values from 2x2 input values, so we keep
// line 0 and line 1 of each level's LL.  During processing, line 1 is rolled
// to line 1, and line 1 is requested from the smaller level recursively.
//
// Note that LH, HL and HH are loaded from the compressed file, LL is regenerated recursively
// from smaller levels.
//
// the line0 and line1 are +1 larger in X than required by this level; this handles
// different level sizes correctly. For example, we might expand expand a 11x12
// image up to a 22 x 23 image.

// The following returns an index into the line buffer, for the indicated
// band number (0..p_qmf->nr_bands), sideband (LL_SIDEBAND..HH_SIDEBAND) and line (0 or 1)
//
// NOTE WELL!! The "used_bands" is the value used for all band loops during decompression.
// This is so that (later) we can set up the decompress logic to uncompress only some bands.
// Only the unpack() routines will need to be modified for this to work. For now, though,
// we have to unpack all bands.

// Used to compute initial indexes for pointers
#define DECOMP_LEVEL_LINE01(p_level, band, sideband, line)	\
		((p_level)->buffer_ptr + (((2 * (band * MAX_SIDEBAND + sideband)) + (line))	* ((p_level)->level_size_x+2)))
// Used to compute index for a given band and subband
#define	DECOMP_INDEX	\
		(band * MAX_SIDEBAND + sideband)

typedef struct {
	UINT	used_bands;
	IEEE4	**p_p_line0;			// [05] this level's LL, LH, HL and HH for line N+0. Must be signed
	IEEE4	**p_p_line1;			// [05] this level's LL, LH, HL and HH for line N+1. Must be signed
	IEEE4	**p_p_line1_ll_sideband;// [05] This level's LL sideband with x_reflection for line 1, all bands

	UINT  start_read_lines;		// [11] starting read_lines state for ResetView()
	UINT	read_lines;				// # of lines of above level to read. Starts as 2, then will be 0 or 1
	// The following is the next line of this level to read.
	// So a level with sidebands of 32x32 could have this number range from 0..31,
	// as this level would be called up to 64 times to generate the 0..64 lines for the larger level
	// In other words, this is from 0 .. (# lines-1) in the side bands for this level.
	UINT	current_line;			// line currently being read at this level
	UINT	start_line;				// [11] starting line for ResetView()
	// this is the next block line to read (can be from 0..y_block_size). Only valid if have_blocks = TRUE
	INT16	next_input_block_y_line;// a number going from 0 to Y_BLOCK_SIZE-1
	UINT8	have_blocks;			// true if the next Y block number is valid
	UINT	start_x_block;			// first X block, from 0..p_pqm->x_size/p_qmf->x_block_size
	UINT	x_block_count;			// count of X blocks across

	void	*p_x_blocks;			// pointer to a set of decompressions in progress for a set of X blocks
	UINT	first_block_skip;		// how many symbols to skip at start of line in the first block in the set of X blocks
	UINT	last_block_skip;		// how many symbols to skip at end of line in the last block in the X set

	QmfLevelStruct	*p_qmf;				// handy pointer to the QMF for this level
	struct qmf_region_struct *p_region;	// handy pointer to the region that contains these levels
	// the reflection amounts. Need to add these to start/end/size to get true size
	// Always 0 or 1, so can be shorter byte values
	UINT8	reflect_start_x, reflect_end_x;
	UINT8	reflect_start_y, reflect_end_y;
	// NOTE WELL: These start/end/size values EXCLUDE the reflection values
	UINT	level_start_y, level_end_y, level_size_y;
	UINT	level_start_x, level_end_x, level_size_x;
	UINT	output_level_start_y, output_level_end_y, output_level_size_y;
	UINT	output_level_start_x, output_level_end_x, output_level_size_x;

	IEEE4	*buffer_ptr;			// [05] Band/sideband/line[01] buffer.
									// for speed we allocate a single buffer, and point into it.
									// So a single malloc/free per level
} QmfRegionLevelStruct;

// One of these is created for each start_region()/end_region() call pair
typedef struct qmf_region_struct {
	QmfLevelStruct	*p_top_qmf;		// pointer to the smallest level in the QMF
	QmfLevelStruct	*p_largest_qmf;	// pointer to the largest level we have to read for this region resolution
	// region location and size to read
	UINT	start_x, start_y;
	UINT	end_x, end_y;
	UINT	number_x, number_y;

	UINT	random_value;			// [14] random value for texture generation
	// floating point versions of current line and incrememtn. This is because
	// we read power of 2 lines, but need to feed non-power of two lines to caller
	UINT	read_line;				// non-zero if have to read line(s), 0 if not (duplicating line)
	//[22]
	IEEE8	current_line;			// the current output line being worked on
	IEEE8	start_line;				// [11] starting line for ResetView()
	IEEE8	increment_y;			// the increment of input lines to add for each output line. Will be <= 1.0
	IEEE8	increment_x;			// the increment of input pixels to add for each output pixel. Will be <= 1.0
	// recursive information. This array is p_largest_level->level+1 in size
	// The in_line[0|1] points are set up for the size of each level, so increase in size for each level
	QmfRegionLevelStruct	*p_levels;
	IEEE4	**p_p_ll_line;			// [05] the power of two size, INT format, output line. Must be signed
	IEEE4	*p_ll_buffer;			// [05] the buffer that the above indexes into
	UINT	used_bands;				// bands actually read - may be different from p_qmf->nr_bands in future versions
	// The band_list is allocated and freed by the caller. We do NOT allocate or free it
	UINT	nr_bands_requested;		// the number of bands requested from caller; may be differed to used_bands
	UINT	*band_list;				// array of used_bands, indicating actual band numbers to use
	struct NCSFileViewStruct *pNCSFileView;	// [02] pointer to the NCS File View for this QMF Region
	BOOLEAN	bAddTextureNoise;		// [11] TRUE if add texture noise during decompression
	UINT nCounter;                // [21] to fix rounding error in erw_decompress_read_region_line_bil()
} QmfRegionStruct;


/*
**	QMF tree management functions
*/

// QMF tree management routines used by both build/extract

extern void delete_qmf_levels(QmfLevelStruct *p_qmf);
extern UINT get_qmf_tree_nr_blocks( QmfLevelStruct *p_top_level );

QmfLevelStruct *new_qmf_level(UINT nBlockSizeX, UINT nBlockSizeY,
						UINT16 level, UINT x_size, UINT y_size, UINT nr_bands,
						QmfLevelStruct *p_smaller_qmf, QmfLevelStruct *p_larger_qmf);

NCSError allocate_qmf_buffers(QmfLevelStruct *p_top_qmf);



// Data unpack routines

#if defined( MACINTOSH ) && TARGET_API_MAC_OS8
extern int unpack_ecw_block(QmfLevelStruct *pQmfLevel, UINT nBlockX, UINT nBlockY,
					 Handle *ppUnpackedECWBlock, UINT	*pUnpackedLength,
					 UINT8 *pPackedBlock);
int align_ecw_block(NCSFile *pFile, NCSBlockId nBlockID,
					 Handle *ppAlignedECWBlock, UINT	*pAlignedLength,
					 UINT8 *pPackedBlock, UINT nPackedLength);
#else
extern int unpack_ecw_block(QmfLevelStruct *pQmfLevel, UINT nBlockX, UINT nBlockY,
					 UINT8 **ppUnpackedECWBlock, UINT	*pUnpackedLength,
					 UINT8 *pPackedECWBlock);
int align_ecw_block(NCSFile *pFile, NCSBlockId nBlockID, 
					 UINT8 **ppAlignedECWBlock, UINT	*pAlignedLength,
					 UINT8 *pPackedBlock, UINT nPackedLength);
#endif //MACINTOSH

extern int	unpack_data(UINT8 **p_raw,
						UINT8  *p_packed, UINT raw_length,
						UINT8 nSizeOfEncodeFormat);


int unpack_init_lines( QmfRegionLevelStruct *p_level );

int unpack_start_line_block( QmfRegionLevelStruct *p_level, UINT x_block,
							UINT8 *p_packed_block, UINT	lines_to_skip);

int	unpack_line( QmfRegionLevelStruct *p_level );

void unpack_finish_lines( QmfRegionLevelStruct *p_level );

void unpack_free_lines( QmfRegionLevelStruct *p_level );

/*
**	Decoder functions
*/

// Visible functions that can be called

extern QmfLevelStruct *erw_decompress_open(
			char *p_input_filename,		// File to open
			UINT8	*pMemImage,			// if non-NULL, open the memory image not the file
			BOOLEAN bReadOffsets,		// TRUE if the client wants the block Offset Table
			BOOLEAN bReadMemImage );	// TRUE if the client wants a Memory Image of the Header
			

extern QmfRegionStruct *erw_decompress_start_region( QmfLevelStruct *p_top_qmf,
				 UINT	nr_bands_requested,			// number of bands to read
				 UINT	*p_band_list,				// list of bands to be read. Caller allocs/frees this
				 UINT start_x, UINT start_y,
				 UINT end_x, UINT end_y,
				 UINT number_x, UINT number_y);

int erw_decompress_read_region_line( QmfRegionStruct *p_region, UINT8 *pRGBAPixels);


void erw_decompress_end_region( QmfRegionStruct *p_region );
extern void erw_decompress_close( QmfLevelStruct *p_top_qmf );

// handy argument parsing for stand alone program
extern int setup_decompress(int argc, char *argv[],
			char **p_p_input_erw_filename,
			char **p_p_output_bil_filename);

// Internal functions. Do not call these

extern int qdecode_qmf_level_line( QmfRegionStruct *p_region, UINT16 level, UINT y_line,
								  IEEE4 **p_p_output_line);

/*
**	ECW File reading Abstraction Routines
**	These are non-ER Mapper specific (as ECW gets linked stand alone),
**	and CAN NOT use fopen() to due # of file limitations on platforms like Windows
*/

/*
**	The Unix vs WIN32 calls are so different, we wrap the reads with functions,
**	that return FALSE if all went well, and TRUE if there was an error.
**	Note that the ECWFILE *can* be NULL (windows madness), so you will have to
**	keep a separate variable to decide if the file is open or not.
*/
BOOLEAN EcwFileOpenForRead(char *szFilename, ECWFILE *pFile);		// Opens file for binary reading
BOOLEAN EcwFileClose(ECWFILE hFile);								// Closes file
BOOLEAN EcwFileRead(ECWFILE hFile, void *pBuffer, UINT nLength);	// reads nLength bytes into existing pBuffer
BOOLEAN EcwFileSetPos(ECWFILE hFile, UINT64 nOffset);				// Seeks to specified location in file
BOOLEAN EcwFileGetPos(ECWFILE hFile, UINT64 *pOffset);				// Returns current file position
BOOLEAN EcwFileReadUint8(ECWFILE hFile, UINT8 *sym);
BOOLEAN EcwFileReadUint16(ECWFILE hFile, UINT16 *sym16);
BOOLEAN EcwFileReadUint32(ECWFILE hFile, UINT *sym32);
BOOLEAN EcwFileReadUint64(ECWFILE hFile, UINT64 *sym);
BOOLEAN EcwFileReadIeee8(ECWFILE hFile, IEEE8 *fvalue);
BOOLEAN EcwFileReadIeee4(ECWFILE hFile, IEEE4 *fsym32);

void sread_ieee8(IEEE8 *sym, UINT8 *p_s);
UINT sread_int32(UINT8 *p_s);
UINT16 sread_int16(UINT8 *p_s);

/*********************************************************
**	NCSECW Cache management values
**********************************************************/

// If a File View currently has a view size (window size) that
// is smaller than this number, blocks will be cached,
// otherwise the blocks will be freed as soon as they are used

#define NCSECW_MAX_VIEW_SIZE_TO_CACHE	4000


#define NCSECW_MAX_UNUSED_CACHED_FILES	1	    // Maximum number of files that can be open and unused
#define NCSECW_CACHED_BLOCK_POOL_SIZE	1000	// initial size of cached block pointers pool
#define NCSECW_MAX_SEND_PACKET_SIZE		(8*1024)	// maximum size of a packet to send
#define NCSECW_IDWT_QUEUE_GRANULARITY	32		// realloc the setview queue every this often

// Time related
#define NCSECW_PURGE_DELAY_MS			1000	// only purge cache after at least this time has passed since last purge
// These are stored in the global shared application structure, and can be changed by users
#define NCSECW_BLOCKING_TIME_MS			10000	// wait up to 10 seconds before giving up, for blocking clients
#define NCSECW_QUIET_WAIT_TIME_MS		10000	// wait up to 10 seconds before giving up, for blocking clients
#define	NCSECW_REFRESH_TIME_MS			500		// allow 0.5 seconds to pass before issuing a new refresh if new blocks have arrived
#define NCSECW_FILE_PURGE_TIME_MS		(30*60*1000)	// Ideally keep files for up to 30 minutes when idle
#define NCSECW_FILE_MIN_PURGE_TIME_MS	(30*1000)		// never purge files in less than 30 seconds

#define NCSECW_MAX_SETVIEW_PENDING	1	// maximum number of SetViews that can be pending
#define NCSECW_MAX_SETVIEW_CANCELS	3	// maximum number of SetViews that will be cancelled,
										// before forcing completion regardless of number pending
#define NCSECW_HIGHMAX_SETVIEW_PENDING 10	// unless this many setviews are pending, in which case
											// we flush pending's regardless (if we can)
#define NCSECW_MAX_OFFSET_CACHE		1024	// Offset cache size for ECW files with RAW block tables

// The type of caching used for a particular file view
typedef enum {
	NCS_CACHE_INVALID = 0,	// invalid caching method
	NCS_CACHE_DONT	  = 1,	// Don't cache for this file view (typically because a big file view)
	NCS_CACHE_VIEW	  = 2	// A "normal" file view, so cache blocks where possible
} NCSCacheMethod;

// The type of request for a block in a file cache list
typedef enum {
	NCSECW_BLOCK_INVALID	= 0,	// invalid block request
	NCSECW_BLOCK_REQUEST	= 1,	// Post a Request for the block
	NCSECW_BLOCK_CANCEL		= 2,	// Post a Cancel for the block
	NCSECW_BLOCK_RETURN		= 3		// Return the block in the cache list
} NCSEcwBlockRequestMethod;

// [03] current view callback state. ONLY used for callback style views
typedef enum {
	NCSECW_VIEW_INVALID		= 0,	// invalid state
	NCSECW_VIEW_QUIET		= 1,	// open, but no active setview, and not processing the IDWT
	NCSECW_VIEW_SET			= 2,	// Setview done, but not yet reading data
	NCSECW_VIEW_QUEUED		= 3,	// queued to process a iDWT for the view (in Thread queue)
	NCSECW_VIEW_IDWT		= 4		// processing a IDWT for the view
} NCSEcwViewCallbackState;

// [03] insertion order into a queue. Generally, LIFO is faster than FIFO if you don't care
typedef enum {
	NCSECW_QUEUE_INVALID	= 0,	// invalid state
	NCSECW_QUEUE_LIFO		= 1,	// Insert into queue in LIFO order
	NCSECW_QUEUE_FIFO		= 2		// Insert into queue in FIFO order
} NCSEcwQueueInsertOrder;

/*********************************************************
**	NCSECW Structure definitions
**********************************************************/

typedef struct NCSFileCachedBlockStruct {
	NCSBlockId	nBlockNumber;	// block number being cached
	struct NCSFileCachedBlockStruct	*pNextCachedBlock;	// next block in the cache list for the file
	UINT8	*pPackedECWBlock;	// the packed ECW block that is currently cached (or NULL)
	UINT8	*pUnpackedECWBlock;	// the packed ECW block that is currently cached (or NULL)
	UINT	nPackedECWBlockLength;	// Length of the packed block
	UINT	nUnpackedECWBlockLength;// Length of the unpacked block
	UINT	nUsageCount;		// number of times in use by FileViews
	UINT	nHitCount;			// number of times block has been read during inverse DWT operations
	BOOLEAN	bRequested;			// only TRUE if block request HAS BEEN SENT to the server
								// (so FALSE in every other situation, even if block not loaded yet)
	UINT64	nDecodeMissID;		/**[08]**/
} NCSFileCachedBlock;

// This is used during cache purging. There is one entry per level in the file
typedef struct NCSFileCachePurgeStruct {
	NCSFileCachedBlock	*pPreviousBlock;	// the block BEFORE the first block at this level
	NCSFileCachedBlock	*pLevelBlocks;		// the first block at this level
} NCSFileCachePurge;

typedef struct {
	NCSBlockId	nID;
	UINT		nLength;
	UINT64		nOffset;
	NCSTimeStampMs tsLastUsed;
} NCSFileBlockOffsetEntry;

struct NCSFileStruct {
	QmfLevelStruct	*pTopQmf;				// Pointer to the top level of the ECW QMF structure
	struct	NCSFileStruct		*pNextNCSFile, *pPrevNCSFile;	// NCSECW linked list of NCS files cached
	UINT	nUsageCount;			// number of times this file is currently open
	UINT	SemiUniqueId;			// Somewhat unique ID, based on File Name
	BOOLEAN bReadOffsets;			// TRUE if the block offsets table has been read and is valid for the QMF
	BOOLEAN bReadMemImage;			// TRUE if the NCSFile has a memory image of the header present
	char	*szUrlPath;				// URL (filename) for this file
	BOOLEAN	bValid;					// File is currently valid (it has not changed on disk since open).
	// Client side information (not valid when file opened at the server end)
	NCSTimeStampMs	tLastSetViewTime;// Last time a SetView was done, used to decide when to purge files from cache
	NCSPool			*pBlockCachePool;		// Pointer to pool of cached blocks
	NCSFileCachedBlock	*pFirstCachedBlock;	// Pointer to first block in the cached block list
	NCSFileCachedBlock	*pWorkingCachedBlock;	// Pointer to block last accessed (reduces search times)
	NCSFileCachedBlock	*pLastReceivedCachedBlock;	// Pointer to last block received to speed list access
	NCSClientUID	nClientUID;		// Unique UID for this client file opened
	NCSSequenceNr	nServerSequence; // current maximum sequence number read from back from the server
	NCSSequenceNr	nClientSequence; // current sequence number client has issued
	UINT8	*pLevel0ZeroBlock;		// a level 0 zero block containing all zeros for all bands and sidebands
	UINT8	*pLevelnZeroBlock;		// a > level 0 zero block (has one less sideband than level 0) 
	struct	NCSFileViewStruct	*pNCSFileViewList;			// list of OPEN file views for this file
	struct	NCSFileViewStruct	*pNCSCachedFileViewList;	// CLOSED but Cached File Views for this file
	BOOLEAN	bSendInProgress;			// if TRUE, a send pack (request and/or cancel) has been made, and callback will be made once complete
	UINT	nRequestsXmitPending;		// if non-zero, number of block read requests waiting to send
										// (NOT requests already sent that responses are waiting for)
	UINT	nCancelsXmitPending;		// if non-zero, number of block cancel requests waiting to sent
	UINT	nUnpackedBlockBandLength;	// length of one band in an unpacked block (always unpacked out to max X&Y block size)
	NCSFileCachePurge	*pNCSCachePurge;// an array, one entry per QMF level
	UINT16	nCachePurgeLevelCount;		// maximum levels CURRENTLY in the NCSCachePurge array, might be less than max_levels+1
	ECWFileInfoEx *pFileInfo;				// Handy collection of information about the file.
	BOOLEAN bIsConnected;				// Are we still connected to the server serving this file /**[07]**/
	BOOLEAN	bIsCorrupt;						// File is corrupt - displayed message for user

	NCSFileBlockOffsetEntry *pOffsetCache;
	UINT					nOffsetCache;
	BOOLEAN		bFileIOError; //[17]
};

struct NCSFileViewStruct {
	NCSFile			*pNCSFile;
	QmfRegionStruct *pQmfRegion;	// pointer to current QMF region
	NCSCacheMethod	nCacheMethod;	// caching method used by this file view
	struct NCSFileViewStruct	*pNextNCSFileView, *pPrevNCSFileView;	// list of views for this file
	NCSTimeStampMs	tLastBlockTime;	// Time that the last block was received.
									// Currently only valid for served (not local) files
	NCSEcwViewCallbackState	eCallbackState;
	NCSFileViewSetInfo	info;		// public information, and current view values
	NCSFileViewSetInfo	pending;	// pending SetView, which is waiting for current view to finish processing
	UINT16		nPending;			// Number of pending SetView's outstanding
	UINT16		nCancelled;			// Number of SetViews cancelled since a successful one
	BOOLEAN		bIsRefreshView;		// TRUE if the current view is just updating the previous view.
									// If true, the current view will be cancelled when a SetView is done
	UINT64		nNextDecodeMissID;	/**[08]**/
};

typedef LONG NCSEcwStatsType;

typedef struct tagNCSEcwStatistics {
	// NOTE:  DO NOT CHANGE ANY OF THESE FIELDS.  ADD NEW FIELDS AT END OF STRUCTURE.
	// FIELDS DOWN TO nMaximumCacheSize MUST BE IDENTICAL TO NCSecwStatisticsV1

	NCSEcwStatsType	nApplicationsOpen;		// number of applications currently open

	// Time wait for network server to respond with blocks
	NCSEcwStatsType	nBlockingTime;			// Time in ms to block clients that don't support callbacks
	NCSEcwStatsType	nRefreshTime;			// Time in ms to wait between refresh callbacks to smart clients

	// Statistics

	NCSEcwStatsType	nFilesOpen;				// number of files currently open
	NCSEcwStatsType	nFilesCached;			// number of files currently cached but not open
	NCSEcwStatsType	nFilesCacheHits;		// number of times an open found an existing cached file
	NCSEcwStatsType	nFilesCacheMisses;		// number of times an open could not find the file in cache
	NCSEcwStatsType	nFilesModified;			// number of times files were invalidated because they changed on disk while open

	NCSEcwStatsType	nFileViewsOpen;			// number of file views currently open
	NCSEcwStatsType	nFileViewsCached;		// number of file views cached but not open

	NCSEcwStatsType	nSetViewBlocksCacheHits;		// number of times a SetView hit a cached block structure
	NCSEcwStatsType	nSetViewBlocksCacheMisses;		// number of times a SetView missed a cached block

	NCSEcwStatsType	nReadBlocksCacheHits;			// number of times a read hit a cached block
	NCSEcwStatsType	nReadUnpackedBlocksCacheHits;	// number of times a read hit a unpacked cached block
	NCSEcwStatsType	nReadBlocksCacheMisses;			// number of times a read missed a cached block
	NCSEcwStatsType	nReadBlocksCacheBypass;			// number of times a read bypassed cache (for large view IO)

	NCSEcwStatsType	nRequestsSent;					// number of block read requests sent to the server
	NCSEcwStatsType	nCancelsSent;					// number of block read request cancels sent to the server
	NCSEcwStatsType	nBlocksReceived;				// number of blocks received
	NCSEcwStatsType  nCancelledBlocksReceived;		// number of blocks cancelled that were still received

	NCSEcwStatsType	nRequestsXmitPending;			// number of block read requests waiting to be sent to server
	NCSEcwStatsType	nCancelsXmitPending;			// number of block cancel requests waiting to be sent to server

	NCSEcwStatsType	nPackedBlocksCacheSize;			// Size in bytes of packed blocks in cache
	NCSEcwStatsType	nUnpackedBlocksCacheSize;		// Size in bytes of unpacked blocks in cache
	NCSEcwStatsType	nMaximumCacheSize;				// Maximum allowed size of cache

	// Add NEW things below ALWAYS AT END!
	//
	// NOTE: If you add extra stats here, increment ECW_STATS_STRUCT_VERSION and always check the nStatsStructVersion global
	// to ensure the new version of the structure is available.  If NCSecw.dll exists in the C:\winnt\system32 (NT/2000) or 
	// C:\Windows\system (Win9x) directories, that Dll will be loaded and the shared data segment in it used instead of a
	// shared memory segment.  This means you may be using an older version of the stats structure than you compiled in, hence
	// the need to check the version (and hence size).  This is all done for backwards compatibility due to the move from a
	// shared Dll in the system directory to a local dll but the need to shared global stats across multiple applications.
} NCSecwStatistics;


// [03] One of these per iDWT thread in the iDWT pool of threads (currently only 1 thread)
// These threads are ONLY used to process iDWT's for callback based SetViews.
typedef struct {
	NCSFileView	**ppNCSFileView;			// array of pointers to FileViews to process callbacks for
	INT		nQueueAllocLength;			// total queue length currently allocated; it might grow
	INT		nQueueNumber;				// number of active items in the queue, could be zero
} NCSidwt;

typedef struct {
	//NCSFile	*pNCSFileList;					// List of NCS files currently open
	UINT8		nStatsStructVersion;
	NCSecwStatistics	*pStatistics;
	BOOLEAN	bShutdown;						// [02] true if in process shutdown mode
	BOOLEAN	nForceFileReopen;				// normally FALSE - only set to TRUE for server load testing
	BOOLEAN bNoTextureDither;				// normally FALSE - TRUE for compression, so input compressed files have no texture dither added
	BOOLEAN	bForceLowMemCompress;			// Force a low memory compressioin
	NCSTimeStampMs	tLastCachePurge;		// How long ago the last Cache Purge was carried out
	UINT	nAggressivePurge;				// Higher number means previous purge did not reduce down to
											// desired purge budget, so be more aggressive on next purge
	UINT	nMaximumOpen;					// [15] Max # files open in cache
	NCSTimeStampMs	nPurgeDelay;			// [16] Time delay after last purge before new purge allowed
	NCSTimeStampMs  nFilePurgeDelay;		// [16] Time delay between last view closing and file being purged from cache
	NCSTimeStampMs  nMinFilePurgeDelay;		// [16] Min Time delay between last view closing and file being purged from cache

	UINT			nMaxOffsetCache;		// [16] Max size of offset cache for ECW files with RAW block tables.

	BOOLEAN bEcwpReConnect;					// [19] normally FALSE - if TRUE the ecw library will try and reconnect if connection has been lost to IWS
	BOOLEAN bJP2ICCManage;					// [20] normally TRUE - if FALSE the ecw library does not do ICC management on READ
	UINT	nMaxJP2FileIOCache;				// [20] JP2 file IO cache size
	UINT	nMaxProgressiveViewSize;		// [21] Maximum height and width of progressive mode views
} NCSEcwInfo;



/**********************************************************
**	NCS to ECW Glue and wrapper functions
**********************************************************/


extern NCSError NCSecwOpenFile(
					NCSFile **ppNCSFile,
					char *szInputFilename,		// input file name or network path
					BOOLEAN bReadOffsets,		// TRUE if the client wants the block Offset Table
					BOOLEAN bReadMemImage);		// TRUE if the client wants a Memory Image of the Header

extern int	NCSecwCloseFile( NCSFile *pNCSFile);

extern int	NCSecwReadLocalBlock( NCSFile *pNCSFile, UINT64 nBlockNumber,
					UINT8 **ppBlock, UINT *pBlockLength);
extern BOOLEAN NCScbmGetFileBlockSizeLocal(NCSFile *pNCSFile, NCSBlockId nBlock, UINT *pBlockLength, UINT64 *pBlockOffset );

// Internal functions to the library - not visible to the DLL
int	 NCSecwCloseFileCompletely( NCSFile *pNCSFile);

void NCSEcwStatsIncrement(NCSEcwStatsType *pVal, INT n);
void NCSEcwStatsDecrement(NCSEcwStatsType *pVal, INT n);

/**********************************************************
**	Routines internal to the library - not visible to the DLL
**********************************************************/

int	NCScbmCloseFileViewCompletely(NCSFileView **ppNCSFileViewList, NCSFileView *pNCSFileView);
UINT8 *NCSecwConstructZeroBlock(QmfLevelStruct *p_qmf, QmfRegionStruct *p_region,
						UINT x_block, UINT y_block);
UINT8	*NCScbmReadViewBlock(QmfRegionLevelStruct	*pQmfRegionLevel,
					  UINT nBlockX, UINT nBlockY);
void NCScbmFreeViewBlock(QmfRegionLevelStruct	*pQmfRegionLevel, UINT8 *pECWBlock);
BOOLEAN	NCScbmNetFileBlockRequest(NCSFile *pNCSFile, NCSBlockId nBlock );
NCSError NCScbmNetFileOpenInternal(UINT8 **ppHeaderMemImage, UINT *pnHeaderMemImageLen, void *pCBData, char *szUrlPath);
NCSError NCScbmNetFileOpen(UINT8 **ppHeaderMemImage, UINT *pnHeaderMemImageLen, NCSFile *pNCSFile, char *szUrlPath); //[19]
void	NCScbmNetFileXmitRequests(NCSError nError, UINT8 *pLastPacketSent, NCSFile *pNCSFile );
NCSFileCachedBlock *NCScbmGetCacheBlock(NCSFile *pNCSFile, NCSFileCachedBlock *pWorkingCachedBlock,
											   NCSBlockId nBlock, NCSEcwBlockRequestMethod eRequest);

/**********************************************************
**	GLOBAL NCSECW variables
**********************************************************/

extern NCSEcwInfo *pNCSEcwInfo;
