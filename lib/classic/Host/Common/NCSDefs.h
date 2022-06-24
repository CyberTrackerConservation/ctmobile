#pragma once

#include "pch.h"
#include <fxPlatform.h>

#pragma warning(disable: 4244)

/*
** NCS Specific typedefs
*/
typedef UINT			NCSBlockId;		/* Unique (per file) Block ID		*/
typedef UINT64			NCSClientUID;	/* Unique client ID (per server)	*/
typedef UINT64			NCSSequenceNr;	/* Packet sequence number			*/
typedef INT64			NCSTimeStampMs;	/* msec timestamp - wraps every 2^64 ms (10^6 years) */

/*
** NCS Color typedefs : Note that these are compatible with win32 api calls for COLORREF
*/
typedef UINT			NCSColor;

/*
** Enum type for different kinds of cell sample [03]
*/
typedef enum {						
	NCSCT_UINT8				=	0,	
	NCSCT_UINT16			=	1,	
	NCSCT_UINT32			=	2,	
	NCSCT_UINT64			=	3,	
	NCSCT_INT8				=	4,	
	NCSCT_INT16				=	5,	
	NCSCT_INT32				=	6,	
	NCSCT_INT64				=	7,	
	NCSCT_IEEE4				=	8,	
	NCSCT_IEEE8				=	9	
} NCSEcwCellType;					

// WIN32, LINUX (i386)
#define NCSBO_LSBFIRST

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

#define NCS_NO_UNALIGNED_ACCESS

#ifndef NCS_INLINE
#ifdef WIN32
#define NCS_INLINE __forceinline
#elif defined __GNUC__
#define NCS_INLINE __inline__
#else
#define NCS_INLINE __inline
#endif // WIN32
#endif // NCS_INLINE

#ifndef _ASSERT
#define _ASSERT(a)
#endif

/*
** Error Enum.
*/

/*
** IMPORTANT: Add new errors to the end of this list so we can retain some 
**			  backwards binary compatibilty. Also, don't forget to add the 
**			  error text!
*/

/** 
 *	@enum
 *	An enumerated type specifying all the errors associated with the SDK.  Each error
 *	code is associated with a string of explanatory text.
 */
typedef enum {
	/* NCS Raster Errors */
	NCS_SUCCESS					= 0,		/**< No error */
	NCS_QUEUE_NODE_CREATE_FAILED,			/**< Queue node creation failed */
	NCS_FILE_OPEN_FAILED,					/**< File open failed */
	NCS_FILE_LIMIT_REACHED,					/**< The Image Web Server's licensed file limit has been reached */
	NCS_FILE_SIZE_LIMIT_REACHED,			/**< The requested file is larger than is permitted by the license on this Image Web Server */
	NCS_FILE_NO_MEMORY,						/**< Not enough memory for new file */
	NCS_CLIENT_LIMIT_REACHED,				/**< The Image Web Server's licensed client limit has been reached */
	NCS_DUPLICATE_OPEN,						/**< Detected duplicate open from net layer */
	NCS_PACKET_REQUEST_NYI,					/**< Packet request type not yet implemented */
	NCS_PACKET_TYPE_ILLEGAL,				/**< Packet type is illegal */
	NCS_DESTROY_CLIENT_DANGLING_REQUESTS,	/**< Client closed while requests outstanding */

	/* NCS Network Errors */
	NCS_UNKNOWN_CLIENT_UID,					/**< Client UID unknown */
	NCS_COULDNT_CREATE_CLIENT,				/**< Could not create new client */
	NCS_NET_COULDNT_RESOLVE_HOST,			/**< Could not resolve address of Image Web Server */
	NCS_NET_COULDNT_CONNECT,				/**< Could not connect to host */
	NCS_NET_RECV_TIMEOUT,					/**< Receive timeout */
	NCS_NET_HEADER_SEND_FAILURE,			/**< Error sending header */
	NCS_NET_HEADER_RECV_FAILURE,			/**< Error receiving header */
	NCS_NET_PACKET_SEND_FAILURE,			/**< Error sending packet */
	NCS_NET_PACKET_RECV_FAILURE,			/**< Error receiving packet */
	NCS_NET_401_UNAUTHORISED,				/**< 401 Unauthorised: SDK doesn't do authentication so this suggests a misconfigured server */			
	NCS_NET_403_FORBIDDEN,					/**< 403 Forbidden: could be a 403.9 from IIS or PWS meaning that the maximum simultaneous request limit has been reached */
	NCS_NET_404_NOT_FOUND,					/**< 404 Not Found: this error suggests that the server hasn't got Image Web Server installed */
	NCS_NET_407_PROXYAUTH,					/**< 407 Proxy Authentication: the SDK doesn't do proxy authentication yet either, so this also suggests misconfiguration */
	NCS_NET_UNEXPECTED_RESPONSE,			/**< Unexpected HTTP response could not be handled */
	NCS_NET_BAD_RESPONSE,					/**< HTTP response received outside specification */
	NCS_NET_ALREADY_CONNECTED,				/**< Already connected */
	NCS_INVALID_CONNECTION,					/**< Connection is invalid */
	NCS_WINSOCK_FAILURE,					/**< A Windows sockets failure occurred */

	/* NCS Symbol Errors */
	NCS_SYMBOL_ERROR,			/**< Symbology error */
	NCS_OPEN_DB_ERROR,			/**< Could not open database */
	NCS_DB_QUERY_FAILED,		/**< Could not execute the requested query on database */
	NCS_DB_SQL_ERROR,			/**< SQL statement could not be executed */
	NCS_GET_LAYER_FAILED,		/**< Open symbol layer failed */
	NCS_DB_NOT_OPEN,			/**< The database is not open */
	NCS_QT_TYPE_UNSUPPORTED,	/**< This type of quadtree is not supported */

	/* Preference errors */
	NCS_PREF_INVALID_USER_KEY,		/**< Invalid local user key name specified */
	NCS_PREF_INVALID_MACHINE_KEY,	/**< Invalid local machine key name specified */
	NCS_REGKEY_OPENEX_FAILED,		/**< Failed to open registry key */
	NCS_REGQUERY_VALUE_FAILED,		/**< Registry query failed */
	NCS_INVALID_REG_TYPE,			/**< Type mismatch in registry variable */

	/* Misc Errors */
	NCS_INVALID_ARGUMENTS,		/**< Invalid arguments passed to function */
	NCS_ECW_ERROR,				/**< ECW error */
	/* unspecified, but coming out of ecw */
	NCS_SERVER_ERROR,			/**< Server error */
	/* unspecified server error */
	NCS_UNKNOWN_ERROR,			/**< Unknown error */
	NCS_EXTENT_ERROR,			/**< Extent conversion failed */
	NCS_COULDNT_ALLOC_MEMORY,	/**< Could not allocate enough memory */
	NCS_INVALID_PARAMETER,		/**< An invalid parameter was used */

	/* Compression Errors */
	NCS_FILEIO_ERROR,						/**< Error reading or writing file */
	NCS_COULDNT_OPEN_COMPRESSION,			/**< Compression task could not be initialised */
	NCS_COULDNT_PERFORM_COMPRESSION,		/**< Compression task could not be processed */
	NCS_GENERATED_TOO_MANY_OUTPUT_LINES,	/**< Trying to generate too many output lines */
	NCS_USER_CANCELLED_COMPRESSION,			/**< Compression task was cancelled by client application */
	NCS_COULDNT_READ_INPUT_LINE,			/**< Could not read line from input data */
	NCS_INPUT_SIZE_EXCEEDED,				/**< Input image size was exceeded for this version of the SDK */

	/* Decompression Errors */
	NCS_REGION_OUTSIDE_FILE,	/**< Specified image region is outside image extents */
	NCS_NO_SUPERSAMPLE,			/**< Supersampling is not supported by the SDK functions */
	NCS_ZERO_SIZE,				/**< Specified image region has a zero width or height */
	NCS_TOO_MANY_BANDS,			/**< More bands specified than exist in the input file */
	NCS_INVALID_BAND_NR,		/**< An invalid band number has been specified */

	/* NEW Compression Error */
	NCS_INPUT_SIZE_TOO_SMALL,	/**< Input image size is too small to compress - for ECW compression there is a minimum output file size */

	/* NEW Network error */
	NCS_INCOMPATIBLE_PROTOCOL_VERSION,	/**< The ECWP client version is incompatible with this server */
	NCS_WININET_FAILURE,				/**< Windows Internet Client error */
	NCS_COULDNT_LOAD_WININET,			/**< wininet.dll could not be loaded - usually indicates Internet Explorer should be upgraded */

	/* NCSFile && NCSRenderer class errors */
	NCS_FILE_INVALID_SETVIEW,			/**< The parameters specified for setting a file view were invalid, or the view was not set */
	NCS_FILE_NOT_OPEN,					/**< No file is open */
	
	/* NEW JNI Java Errors */
	NCS_JNI_REFRESH_NOT_IMPLEMENTED,	/**< Class does not implement ECWProgressiveDisplay interface */
	/* A class is trying to use RefreshUpdate() method, but has not implemented ECWProgressiveDisplay*/

	/* NEW Coordinate Errors*/
	NCS_INCOMPATIBLE_COORDINATE_SYSTEMS,	/**< Incompatible coordinate systems */
	NCS_INCOMPATIBLE_COORDINATE_DATUM,		/**< Incompatible coordinate datum types */
	NCS_INCOMPATIBLE_COORDINATE_PROJECTION,	/**< Incompatible coordinate projection types */
	NCS_INCOMPATIBLE_COORDINATE_UNITS,		/**< Incompatible coordinate units types */
	NCS_COORDINATE_CANNOT_BE_TRANSFORMED,	/**< Non-linear coordinate systems not supported */
	NCS_GDT_ERROR,							/**< Error involving the GDT database */
	
	/* NEW NCScnet error */
	NCS_NET_PACKET_RECV_ZERO_LENGTH,	/**< Zero length packet received */
	/**[01]**/
	
	/* Macintosh SDK specific errors */
	NCS_UNSUPPORTEDLANGUAGE,			/**< Must use Japanese version of the ECW SDK */
	/**[02]**/

	/* Loss of connection */
	NCS_CONNECTION_LOST,				/**< Connection to server was lost */
	/**[03]**/

	NCS_COORD_CONVERT_ERROR,			/**< NCSGDT coordinate conversion failed */

	/* Metabase Stuff */
	NCS_METABASE_OPEN_FAILED,			/**< Failed to open metabase */
	/**[04]**/
	NCS_METABASE_GET_FAILED,			/**< Failed to get value from metabase */
	/**[04]**/
	NCS_NET_HEADER_SEND_TIMEOUT,		/**< Timeout sending header */
	/**[05]**/
	
	NCS_JNI_ERROR,						/**< Java JNI error */
	/**[06]**/

	NCS_DB_INVALID_NAME,				/**< No data source passed */
	/**[07]**/
	NCS_SYMBOL_COULDNT_RESOLVE_HOST,	/**< Could not resolve address of Image Web Server Symbol Server Extension */
	/**[07]**/
	
	NCS_INVALID_ERROR_ENUM,				/**< The value of an NCSError error number was invalid! */
	/**[08]**/
	
	/* NCSFileIO errors [10] */
	NCS_FILE_EOF,						/**< End of file reached */
	NCS_FILE_NOT_FOUND,					/**< File not found */
	NCS_FILE_INVALID,					/**< File was invalid or corrupt */
	NCS_FILE_SEEK_ERROR,				/**< Attempted to read, write or seek past file limits */
	NCS_FILE_NO_PERMISSIONS,			/**< Permissions not available to access file */
	NCS_FILE_OPEN_ERROR,				/**< Error opengin file */
	NCS_FILE_CLOSE_ERROR,				/**< Error closing file */
	NCS_FILE_IO_ERROR,					/**< Miscellaneous error involving file input or output */

	NCS_SET_EXTENTS_ERROR,				/**< Illegal or invalid world coordinates supplied */
	/**[09]**/ 

	NCS_FILE_PROJECTION_MISMATCH,		/**< Image projection does not match that of the controlling layer */
	/** 1.65 gdt errors [15]**/
	NCS_GDT_UNKNOWN_PROJECTION,		/**< Unknown map projection */
	NCS_GDT_UNKNOWN_DATUM,			/**< Unknown geodetic datum */
	NCS_GDT_USER_SERVER_FAILED,		/**< User specified Geographic Projection Database data server failed */
	NCS_GDT_REMOTE_PATH_DISABLED,	/**< Remote Geographic Projection Database file downloading has been disabled and no local GDT data is available */
	NCS_GDT_BAD_TRANSFORM_MODE,		/**< Invalid mode of transform */
	NCS_GDT_TRANSFORM_OUT_OF_BOUNDS,/**< Coordinate to be transformed is out of bounds */
	NCS_LAYER_DUPLICATE_LAYER_NAME,	/**< A layer already exists with the specified name */
	/**[17]**/
	NCS_LAYER_INVALID_PARAMETER,	/**< The specified layer does not contain the specified parameter */
	/**[18]**/
	NCS_PIPE_CREATE_FAILED,			/**< Failed to create pipe */
	/**[19]**/

	/* Directory creation errors */
	NCS_FILE_MKDIR_EXISTS,			/**< Directory to be created already exists */ /*[20]*/
	NCS_FILE_MKDIR_PATH_NOT_FOUND,	/**< The path specified for directory creation does not exist */ /*[20]*/
	NCS_ECW_READ_CANCELLED,			/**< File read was cancelled */

	/* JP2 georeferencing errors */
	NCS_JP2_GEODATA_READ_ERROR,		/**< Error reading geodata from a JPEG 2000 file */ /*[21]*/
	NCS_JP2_GEODATA_WRITE_ERROR,    /**< Error writing geodata to a JPEG 2000 file */	/*[21]*/
	NCS_JP2_GEODATA_NOT_GEOREFERENCED, /**< JPEG 2000 file not georeferenced */			/*[21]*/

	/* Progressive view too large error */
	NCS_PROGRESSIVE_VIEW_TOO_LARGE, /**< Progressive views are limited to 4000x4000 in size */ /*[23]*/
	
	// Insert new errors before here!
	
	NCS_MAX_ERROR_NUMBER			/**< The maximum error value in this enumerated type - should not itself be reported, must always be defined last */ /*[08]*/
} NCSError;

// Memory management
void *NCSMalloc(UINT iSize, BOOLEAN bClear);
void *NCSRealloc(void *pPtr, UINT iSize, BOOLEAN bClear);
void NCSFree(void *pPtr);

UINT16 NCSByteSwap16(UINT16 n);
UINT NCSByteSwap32(UINT n);
UINT64 NCSByteSwap64(UINT64 n);
void NCSByteSwapRange16(UINT16 *pDst, UINT16 *pSrc, INT nValues);
void NCSByteSwapRange32(UINT *pDst, UINT *pSrc, INT nValues);
void NCSByteSwapRange64(UINT64 *pDst, UINT64 *pSrc, INT nValues);

NCSTimeStampMs NCSGetTimeStampMs(void);

/*
** Granular array factor (nelements to grow/shrink)
*/
#define NCS_ARRAY_GRANULARITY 64

#define NCSArrayInsertElement(T, pArray, nElements, nIndex, pElement)	\
	if(nElements % NCS_ARRAY_GRANULARITY == 0) {			        \
		void *pData = (void *)NCSRealloc((void *)(pArray),	        \
			((nElements) + NCS_ARRAY_GRANULARITY) *		            \
			sizeof(*(pArray)),				                        \
			FALSE);						                            \
		pArray = (T*)pData;						                    \
	}								                                \
	if((nIndex) < (nElements)) {					                \
		memmove(&((pArray)[(nIndex) + 1]),			                \
			&((pArray)[(nIndex)]),				                    \
			((nElements) - (nIndex)) * sizeof(*(pArray)));	        \
	}								                                \
	if(pElement) {							                        \
		(pArray)[(nIndex)] = *(pElement);			                \
	} else {							                            \
		memset(&((pArray)[(nIndex)]), 0, sizeof(*pArray));	        \
	}								                                \
	(nElements) += 1

#define NCSArrayAppendElement(T, pArray, nElements, pElement)	    \
	NCSArrayInsertElement(T, pArray, nElements, nElements, pElement)

#define NCSArrayRemoveElement(T, pArray, nElements, nIndex)		    \
	if((nIndex) < (nElements) - 1) {				                \
		memmove(&((pArray)[(nIndex)]),				                \
			&((pArray)[(nIndex) + 1]),			                    \
			((nElements) - (nIndex) - 1) * sizeof(*(pArray)));	    \
	}								                                \
	(nElements) -= 1;						                        \
	if(nElements % NCS_ARRAY_GRANULARITY == 0) {			        \
		if((nElements) > 0) {					                    \
			void *pData = (void *)NCSRealloc((pArray),	            \
				(nElements) * sizeof(*(pArray)),	                \
				FALSE);					                            \
			pArray = (T*)pData;				                        \
		} else {						                            \
			NCSFree((pArray));				                        \
			pArray = (T*)NULL;				                        \
		}							                                \
	}

/*
** Pool.
*/

typedef struct {
	INT nElementsInUse;
	INT iLastFreeElement;
	void *pElements;
	BOOLEAN *pbElementInUse;
} NCSPoolNode;

typedef struct {
	UINT iElementSize;
	UINT nElementsPerNode;
	UINT nNodes;
	UINT nElementsInUse;

	UINT nAddNodes;
	NCSTimeStampMs tsAddNodeTime;
	
	UINT nRemoveNodes;
	NCSTimeStampMs tsRemoveNodeTime;
	
	UINT nAllocElements;
	NCSTimeStampMs tsAllocElementTime;
	
	UINT nFreeElements;
	NCSTimeStampMs tsFreeElementTime;

} NCSPoolStats;

typedef struct {
	NCSPoolStats	psStats;
	UINT			nMaxElements;
	BOOLEAN			bCollectStats;
	NCSPoolNode		*pNodes;
} NCSPool;

/*
** Pool.
*/
NCSPool *NCSPoolCreate(UINT iElementSize, UINT nElements);
void *NCSPoolAlloc(NCSPool *pPool, BOOLEAN bClear);
void NCSPoolFree(NCSPool *pPool, void *pPtr);
void NCSPoolDestroy(NCSPool *pPool);
void NCSPoolSetMaxSize(NCSPool *pPool, UINT nMaxElements);
NCSPoolStats NCSPoolGetStats(NCSPool *pPool);
void NCSPoolEnableStats(NCSPool *pPool);
void NCSPoolDisableStats(NCSPool *pPool);

/*
** File.
*/
#define NCS_FILE_READ			(1 << 0)
#define NCS_FILE_READ_WRITE		(1 << 1)
#define NCS_FILE_CREATE			(1 << 2)
#define NCS_FILE_CREATE_UNIQUE	(1 << 3)
#define NCS_FILE_APPEND			(1 << 4)
#define NCS_FILE_TEMPORARY		(1 << 5)
#define NCS_FILE_ASYNCIO		(1 << 6)

/*#ifdef WIN32

typedef	HANDLE					NCS_FILE_HANDLE;
#define NCS_NULL_FILE_HANDLE	NULL
typedef DWORD					NCS_FILE_ORIGIN; 
#define NCS_FILE_SEEK_START		FILE_BEGIN
#define NCS_FILE_SEEK_CURRENT	FILE_CURRENT
#define NCS_FILE_SEEK_END		FILE_END

#else
*/
typedef	FILE*					NCS_FILE_HANDLE;
#define NCS_NULL_FILE_HANDLE	0
typedef int						NCS_FILE_ORIGIN;
#define NCS_FILE_SEEK_START		SEEK_SET
#define NCS_FILE_SEEK_CURRENT	SEEK_CUR
#define NCS_FILE_SEEK_END		SEEK_END

//#endif	/* WIN32 */

INT64 NCSFileSeek(NCS_FILE_HANDLE handle, INT64 offset, NCS_FILE_ORIGIN origin);
INT64 NCSFileTell(NCS_FILE_HANDLE handle);

NCSError NCSFileOpen(const char *szFilename, int iFlags, NCS_FILE_HANDLE *phFile);
NCSError NCSFileClose(NCS_FILE_HANDLE handle);
NCSError NCSFileRead(NCS_FILE_HANDLE handle, void* buffer, UINT count, UINT* pRead);

//
// These routines read/write the specified type from/to a file.  The type in the file is the byte order
// specifiec by the routine name.  The value will be swapped to/from the native CPU byte order as
// appropriate.
//
// eg, NCSFileReadUINT16_LSB() - will read a UINT16 (LSB format on disk) from the file, and swap it to 
// the native CPU's byte order.
//
NCSError NCSFileReadUINT8_MSB(NCS_FILE_HANDLE hFile, UINT8 *pBuffer);
NCSError NCSFileReadUINT8_LSB(NCS_FILE_HANDLE hFile, UINT8 *pBuffer);
NCSError NCSFileReadUINT16_MSB(NCS_FILE_HANDLE hFile, UINT16 *pBuffer);
NCSError NCSFileReadUINT16_LSB(NCS_FILE_HANDLE hFile, UINT16 *pBuffer);
NCSError NCSFileReadUINT32_MSB(NCS_FILE_HANDLE hFile, UINT *pBuffer);
NCSError NCSFileReadUINT32_LSB(NCS_FILE_HANDLE hFile, UINT *pBuffer);
NCSError NCSFileReadUINT64_MSB(NCS_FILE_HANDLE hFile, UINT64 *pBuffer);
NCSError NCSFileReadUINT64_LSB(NCS_FILE_HANDLE hFile, UINT64 *pBuffer);
NCSError NCSFileReadIEEE4_MSB(NCS_FILE_HANDLE hFile, IEEE4 *pBuffer);
NCSError NCSFileReadIEEE4_LSB(NCS_FILE_HANDLE hFile, IEEE4 *pBuffer);
NCSError NCSFileReadIEEE8_MSB(NCS_FILE_HANDLE hFile, IEEE8 *pBuffer);
NCSError NCSFileReadIEEE8_LSB(NCS_FILE_HANDLE hFile, IEEE8 *pBuffer);

/*
** Huffman.
*/
typedef struct _NCSHuffmanSymbol {
	UINT16 nValue;
	BOOLEAN bZeroRun;
} NCSHuffmanSymbol;

#define NCS_HUFFMAN_MAX_RUN_LENGTH 0x7fff
#define NCS_HUFFMAN_SIGN_MASK	0x4000
#define NCS_HUFFMAN_VALUE_MASK	0x3fff
#define NCS_HUFFMAN_RUN_MASK	0x8000

typedef struct {
	void	*pTree;
	UINT	nBitsUsed;		
} NCSHuffmanState;

NCSError unpack_huffman(UINT8 *pPacked, INT16 *pUnPacked, UINT nRawLength);
void unpack_huffman_init_state(NCSHuffmanState *pState, UINT8 **ppPacked);
void unpack_huffman_fini_state(NCSHuffmanState *pState);
NCSHuffmanSymbol *unpack_huffman_symbol(UINT8 **ppPacked, NCSHuffmanState *pState);
static NCS_INLINE BOOLEAN unpack_huffman_symbol_zero_run(NCSHuffmanSymbol *pSymbol) {
				return(pSymbol->bZeroRun);
			}
static NCS_INLINE UINT16 unpack_huffman_symbol_zero_length(NCSHuffmanSymbol *pSymbol) {
				return(pSymbol->nValue);
			};
static NCS_INLINE INT16 unpack_huffman_symbol_value(NCSHuffmanSymbol *pSymbol) {
				return(pSymbol->nValue);
			};

/*
** Misc.
*/
#if (defined(WIN32) && defined(_X86_))

#define FLT_TO_INT_INIT() { unsigned int old_controlfp_val = _controlfp(_RC_NEAR, _MCW_RC)
#define FLT_TO_INT_INIT_FLR() { unsigned int old_controlfp_val = _controlfp(_RC_DOWN, _MCW_RC)
#define FLT_TO_INT_FINI() _controlfp(old_controlfp_val, _MCW_RC); }

#define FLT_TO_UINT8(a, b)								\
	{                                                   \
		INT FLT_TO_INT_rval;							\
		__asm                                           \
		{                                               \
			__asm fld dword ptr [b]                     \
			__asm fistp dword ptr [FLT_TO_INT_rval]     \
		}                                               \
		a = FLT_TO_INT_rval;							\
	}
#define FLT_TO_INT32(a, b)						\
		__asm                                   \
		{                                       \
			__asm fld dword ptr [b]             \
			__asm fistp dword ptr [a]           \
		}

#else	/* WIN32 && _X86_ */

#define FLT_TO_INT_INIT() { 
#define FLT_TO_INT_FINI() }

#define FLT_TO_UINT8(a, b)								\
	   { a = (UINT8) b; }
#define FLT_TO_INT32(a,b)								\
	   { a = (INT) b; }

#endif	/* WIN32 && _X86_ */
