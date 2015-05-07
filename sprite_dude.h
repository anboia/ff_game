// ========================================================
// ===== DEFINE ===========================================

// ===== KEY INPUT
#define KEY_A        0x0001
#define KEY_B        0x0002
#define KEY_SELECT   0x0004
#define KEY_START    0x0008
#define KEY_RIGHT    0x0010
#define KEY_LEFT     0x0020
#define KEY_UP       0x0040
#define KEY_DOWN     0x0080
#define KEY_R        0x0100
#define KEY_L        0x0200

#define KEY_MASK		0x03FF


#define SPaletteMem ((u16*) 0x5000200)

// typedef struct BG_POINT { s16 x, y; }  BG_POINT, PT;

#define TID_2D(i,j) ((i<<5)+j)

#define ATTR0_Y_MASK		0x00FF
#define ATTR0_Y_SHIFT			 0
#define ATTR0_Y(n)			((n)<<ATTR0_Y_SHIFT)

#define ATTR1_X_MASK		0x01FF
#define ATTR1_X_SHIFT			 0
#define ATTR1_X(n)			((n)<<ATTR1_X_SHIFT)

#define ATTR1_FLIP_MASK		0x3000
#define ATTR1_FLIP_SHIFT		12
#define ATTR1_FLIP(n)		((n)<<ATTR1_FLIP_SHIFT)

#define BFN_PREP(x, name)	( ((x)<<name##_SHIFT) & name##_MASK )
#define BFN_GET(y, name)	( ((y) & name##_MASK)>>name##_SHIFT )
#define BFN_SET(y, x, name)	(y = ((y)&~name##_MASK) | BFN_PREP(x,name) )
#define BFN_CMP(y, x, name)	( ((y)&name##_MASK) == (x) )


#define KEY_DIR				0x00F0	// Directions: left, right, up down

enum LookDir
{
	LOOK_RIGHT= 0, LOOK_DOWN, LOOK_LEFT, LOOK_UP,
};

#define SPR_STATE_STAND		0x0100
#define SPR_STATE_WALK		0x0200


// character speed
#define DUDE_SPEED				0x00D0
// #define DUDE_SPEED				0x0400




// ========================================================
// ===== PROTOTYPES =======================================

bool dude_update(ResourcePack* pResources);

void dude_init(SpriteHandler *sh, int x, int y, int obj_id);
void dude_set_state(SpriteCharacter *dude, u32 state);
int dude_input(SpriteCharacter *dude);

bool dude_move(ResourcePack* pResources, SpriteCharacter *dude, int key);
void dude_animate(SpriteCharacter *dude);

void dude_ani_stand(SpriteCharacter *dude);
void dude_ani_walk(SpriteCharacter *dude);

void initSpriteHandler(ResourcePack* current_screen, u16 *srcTiles, u32 dudeX, u32 dudeY);
void loadSpriteHandler(SpriteHandler* sh);
void oam_init(Sprite* oamBuff);
