// ========================================================
// ===== TYPES ============================================

typedef unsigned char				u8;
typedef unsigned short			u16;
typedef unsigned int				u32;
typedef unsigned long long	u64;

typedef signed char			s8;
typedef signed short		s16;
typedef signed int			s32;

typedef volatile u8			vu8;
typedef volatile u16		vu16;
typedef volatile u32		vu32;

typedef u16							COLOR;
typedef u8							bool;

#define TRUE 			1
#define FALSE 		0

#define ALIGN4		__attribute__((aligned(4)))
#define INLINE		static inline

typedef struct { u32 data[8];  } TILE, TILE4;
typedef struct { u32 data[16]; } TILE8;
typedef u16				SCR_ENTRY;
typedef SCR_ENTRY	SCREENBLOCK[1024];
typedef TILE			CHARBLOCK[512];
typedef TILE8			CHARBLOCK8[256];

typedef struct BG_POINT { s16 x, y; } ALIGN4 BG_POINT, PT;

typedef struct VIEWPORT
{
	int x, xmin, xmax, xpage;
	int y, ymin, ymax, ypage;
} VIEWPORT;

typedef struct Sprite {
	/* Attribute 0: Color mode, sprite shape, y location */
	u16 attribute0;
	/* Attribute 1: Base size, horizontal & vertical flip, x location */
	u16 attribute1;
	/* Attribute 2: Image location, priority */
	u16 attribute2;
	u16 attribute3;
} ALIGN4 Sprite;

/* Bounding boxes for sprites and portals */
typedef struct BoundingBox {
	int y, ySize, x, xSize;
} BoundingBox;

/* Portal for region movement */
typedef struct Portal {
	BoundingBox* boundingBox;
	int destination;
	int destX, destY;
} Portal, *pPortal;

/* Holds all necessary information for the different maps (Ground, Water, Hitmap, Unused) */
typedef struct MapHandler {
	u16* bg;
	int x, y;
	int diffX, diffY;
	int bgNextCol, bgNextRow;
	int bgPrevCol, bgPrevRow;
	int mapNextCol, mapNextRow;
	int mapPrevCol, mapPrevRow;
	int width, height;
	const u8* tiles;
	const u16* map;
} MapHandler, *pMapHandler;

typedef struct SpriteCharacter
{
	s32			x, y;			// Screen Position
	s32         worldX, worldY; // World position
	u16			state;		// Sprite state
	u8			dir;			// Look direction
	u8			objId;		// Object index
	s32			aniFrame;	// Animation frame counter
	s32 		cx, cy;		// Center offset

	BoundingBox* boundingBox;

	Sprite *oamBuff;
} SpriteCharacter;

/* Holds all the information for the current screen tiles */
typedef struct SpriteHandler {
	Sprite* oamBuff;		// current OAM buffer
	SpriteCharacter dude;		// Main Character
	SCR_ENTRY *srcTiles;		// Tiles Data (2D)
} SpriteHandler;

/* Holds all the information for the game */
typedef struct ResourcePack {
	int numPortals;
	/* Ground */
	MapHandler* mh1;
	/* Water */
	MapHandler* mh2;
	/* Hitmap */
	MapHandler* mh3;

	Portal* portals;

	/* Sprites */
	SpriteHandler sh;
} ResourcePack, *pResourcePack;

/* 16x16 tile of 8x8 tiles */
typedef struct TileQuad {
	/* 8x8 tiles Top Left, Top Right, Bottom Left, Bottom Right */
	unsigned short tl, tr, bl, br;
	int tlX, tlY;
} TileQuad;




//------------------------------add1
typedef enum ToolType
{
	TT_EMPTY = 0, TT_SEEDS, TT_WATER, TT_AXE, TT_HAMMER, TT_HOE,
} ToolType;

typedef enum FarmState
{
	FS_EMPTY = 0, FS_SEEDS, FS_SMALL, FS_MED, FS_BIG,
} FarmState;

typedef enum PlantType
{
	PT_CARROT = 0, PT_MUSHROOM, PT_POTATO, PT_TOMATOS, //PT_ONIONS, PT_MELONS,
} PlantType;

typedef struct Farmable{
	FarmState state;
	PlantType type;
	u16 age;
	bool water;
	u16 noWaterDays;
} Farmable;

//------------------------------end_add1