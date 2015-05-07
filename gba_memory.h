// layers
#define OBJ_ENABLE		0x1000

// main sections
#define MEM_IO					0x04000000
#define MEM_VRAM				0x06000000 // video Buffer
#define MEM_PAL					0x05000000 // Palette

// main sections sizes
#define PAL_SIZE				0x00400
#define VRAM_SIZE				0x18000

#define VRAM_PAGE_SIZE	0x0A000

#define MEM_VRAM_BACK	(MEM_VRAM + VRAM_PAGE_SIZE)

#define DCNT_PAGE				0x0010

// VRAM access
// tile_mem[y] = TILE[]   (char block y)
// tile_mem[y][x] = TILE (char block y, tile x)
#define tile_mem		( (CHARBLOCK*)MEM_VRAM)
#define tile8_mem		((CHARBLOCK8*)MEM_VRAM)

// se_mem[y] = SB_ENTRY[] (screen block y)
// se_mem[y][x] = screen entry (screen block y, screen entry x)
#define se_mem			((SCREENBLOCK*)MEM_VRAM)


// BACKGROUND ==================================================================
/* Scrolling BG defines */
#define REG_BG_OFS			((BG_POINT*)(MEM_IO+0x0010)) // Bg scroll array

/* BG Setup defines */
#define REG_BGCNT				((vu16*)(MEM_IO+0x0008)) // Bg control array

#define BG_CBB_MASK		0x000C
#define BG_CBB_SHIFT		 2
#define BG_CBB(n)		((n)<<CHAR_SHIFT)
#define BG_SBB_MASK		0x1F00
#define BG_SBB_SHIFT		 8
#define BG_SBB(n)		((n)<<SCREEN_SHIFT)

#define BFN_PREP(x, name)	( ((x)<<name##_SHIFT) & name##_MASK )
#define BFN_GET(y, name)	( ((y) & name##_MASK)>>name##_SHIFT )
#define BFN_SET(y, x, name)	(y = ((y)&~name##_MASK) | BFN_PREP(x,name) )
#define BFN_CMP(y, x, name)	( ((y)&name##_MASK) == (x) )
