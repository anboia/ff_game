#include "gba_helper.h"
#include "sprite_data_2d_8bpp.h"
#include "sprite_dude.h"
#include "font.h"
#include "text.h"
#include "States.h"
#include "Map_Palette.h"
#include "Maps.h"
#include "string.h"

/* Important hitmap colors */
#define HITMAP_FARMABLE_COLOR 11 /*((unsigned short*) (0x05000016))*/
#define HITMAP_PORTAL_COLOR 12 /*((unsigned short*) (0x05000018))*/
#define HITMAP_WATER_COLOR 14 /*((unsigned short*) (0x0500001C))*/

/* Region loading */
void loadRegion(ResourcePack*, int, int, int);
void loadRegionMap(MapHandler*, const unsigned char*, int, int);
void loadPortals(Portal*, int*, int);
inline void loadRegionPal(const unsigned short*);

/* Vertical scrolling */
void copyRow(unsigned short*, const unsigned short*, int, int, int, int, int);
void scrollVertically(MapHandler*, bool);

/* Horizontal scrolling */
void copyCol(unsigned short*, const unsigned short*, int, int, int, int, int, int);
void scrollHorizontally(MapHandler*, bool);

/* Location determination */
unsigned short getTile(MapHandler*, int, int);
TileQuad* getTileQuad(MapHandler*, int, int);
bool determineMove(MapHandler*, u16);
unsigned char getPixelColor(MapHandler*, u16, int, int);

/* Main functions */
ResourcePack* Initialize();
void LoadContent(ResourcePack*);
void Update(ResourcePack*);
void Draw(ResourcePack*);

int main() {
	ResourcePack* pResources;
	pResources = Initialize();
	LoadContent(pResources);
	while(1) {
		Update(pResources);
	 	Draw(pResources);
	}
	return 1;
}

ResourcePack* Initialize() {
	/* Set Mode */
    SetMode(0 | BG0_ENABLE | BG1_ENABLE | BG2_ENABLE | BG3_ENABLE);

	/* Initialize resource pack */
	ResourcePack* pResources;
	pResources = (ResourcePack*) malloc(sizeof(ResourcePack));

	/* Initialize state */
	gameState = 0;

	/* Ground */
	pResources->mh1 = (MapHandler*) malloc(sizeof(MapHandler));
    REG_BG1CNT = BG_COLOR256 | TEXTBG_SIZE_256x256 | (15 << SCREEN_SHIFT) | (1 << CHAR_SHIFT);
    pResources->mh1->bg = (unsigned short*) screenBaseBlock(15);

	/* Water */
    pResources->mh2 = (MapHandler*) malloc(sizeof(MapHandler));
    REG_BG2CNT = BG_COLOR256 | TEXTBG_SIZE_256x256 | (23 << SCREEN_SHIFT) | (2 << CHAR_SHIFT);
	pResources->mh2->bg = (unsigned short*) screenBaseBlock(23);
    pResources->mh2->x = 0;
    pResources->mh2->y = 0;
    pResources->mh2->diffX = 0;
    pResources->mh2->diffY = 0;
    pResources->mh2->bgNextCol = 30;
    pResources->mh2->bgNextRow = 21;
    pResources->mh2->bgPrevCol = 31;
    pResources->mh2->bgPrevRow = 31;
    pResources->mh2->mapNextCol = 30;
    pResources->mh2->mapNextRow = 21;
    pResources->mh2->mapPrevCol = (WATER_WIDTH)-1;
	pResources->mh2->mapPrevRow = (WATER_HEIGHT)-1;
	pResources->mh2->width = WATER_WIDTH;
	pResources->mh2->height = WATER_HEIGHT;
	pResources->mh2->tiles = Water_Tiles;
	pResources->mh2->map = Water_Map;

	/* Hitmap */
	pResources->mh3 = (MapHandler*) malloc(sizeof(MapHandler));
    REG_BG3CNT = BG_COLOR256 | TEXTBG_SIZE_256x256 | (31 << SCREEN_SHIFT) | (3 << CHAR_SHIFT);
	pResources->mh3->bg = (unsigned short*) screenBaseBlock(31);

    /* Turn on timer0, set to 256 clocks */
    REG_TM0CNT = TIMER_FREQUENCY_256 | TIMER_ENABLE;
    /* Turn on timer1, grab overflow from timer0 */
    REG_TM1CNT = TIMER_OVERFLOW | TIMER_ENABLE;
    /* Turn on timer2, set to system clock */
    REG_TM2CNT = TIMER_FREQUENCY_SYSTEM | TIMER_ENABLE;
    /* Turn on timer3, grab overflow from timer2 */
    REG_TM3CNT = TIMER_OVERFLOW | TIMER_ENABLE;

    init_text();

    REG_DISPCNT |= OBJ_ENABLE | OBJ_MAP_2D;
    initSpriteHandler(pResources, (u16*)sprite_data_2d_8bppTiles, 112<<8, 64<<8);
    
	return pResources;
}

void LoadContent(ResourcePack* pResources) {
	/* Load initial region (ground + hitmap) */
    loadRegion(pResources, (HOME + HOME_OUTSIDE), 0, 0);

	/* Load water map */
    loadRegionMap(pResources->mh2, Water_Tiles, WATER_TILES_NUM, 2);

	/* Load sprites into memory */
    loadSpriteHandler(&pResources->sh);
    DMAFastCopy((void*) sprite_data_2d_8bppPal, (void*) SPaletteMem,  128, DMA_32NOW);
	dude_update(pResources);
}

void Update(ResourcePack* pResources) {
	int i, mask;
	double fat;
	/*char buff[30];*/
	TileQuad* tq = (TileQuad*) malloc(sizeof(TileQuad));
	Portal portal;
	SpriteCharacter* dude = &pResources->sh.dude;
	keyPoll();
	if(keyIsDown(BUTTON_UP)) {
		if(dude_update(pResources)) {
			/* Move up */
			/* Check if next tile is portal */
			tq = getTileQuad(pResources->mh3, dude->worldX, dude->worldY);
			if(tq->bl == 1 && tq->br == 1) {
				/* Find correct portal */
                for(i = 0; i < pResources->numPortals; i++) {
					if(dude->worldX >= pResources->portals[i].boundingBox->x &&
						dude->worldX <= pResources->portals[i].boundingBox->x + pResources->portals[i].boundingBox->xSize &&
						dude->worldY >= pResources->portals[i].boundingBox->y &&
						dude->worldX <= pResources->portals[i].boundingBox->y + pResources->portals[i].boundingBox->ySize) {
						portal = pResources->portals[i];
						break;
					}
				}
				loadRegion(pResources, portal.destination, 0, 0);
			} else { /* Otherwise, move bg */
				/* Move ground */
				pResources->mh1->y--;
				pResources->mh1->diffY--;

				/* Move water */
				pResources->mh2->y--;

				/* Move hitmap */
	            pResources->mh3->y--;
				pResources->mh3->diffY--;
			}
		}
	} else if(keyIsDown(BUTTON_DOWN)) {
		if(dude_update(pResources)) {
            /* Check if next tile is portal */
			tq = getTileQuad(pResources->mh3, dude->worldX, dude->worldY);
			if(tq->bl == 1 && tq->br == 1) {
				/* Find correct portal */
                for(i = 0; i < pResources->numPortals; i++) {
					if(dude->worldX >= pResources->portals[i].boundingBox->x &&
						dude->worldX <= pResources->portals[i].boundingBox->x + pResources->portals[i].boundingBox->xSize &&
						dude->worldY >= pResources->portals[i].boundingBox->y &&
						dude->worldX <= pResources->portals[i].boundingBox->y + pResources->portals[i].boundingBox->ySize) {
						portal = pResources->portals[i];
						break;
					}
				}
				loadRegion(pResources, portal.destination, 0, 0);
			} else { /* Otherwise, move bg */
				/* Move down */
				/* Move ground */
				pResources->mh1->y++;
				pResources->mh1->diffY++;

				/* Move water */
	            pResources->mh2->y++;

				/* Move hitmap */
	            pResources->mh3->y++;
				pResources->mh3->diffY++;
			}
		}
	} else if(keyIsDown(BUTTON_RIGHT)) {
		if(dude_update(pResources)) {
            /* Check if next tile is portal */
			tq = getTileQuad(pResources->mh3, dude->worldX, dude->worldY);
			if(tq->bl == 1 && tq->br == 1) {
				/* Find correct portal */
                for(i = 0; i < pResources->numPortals; i++) {
					if(dude->worldX >= pResources->portals[i].boundingBox->x &&
						dude->worldX <= pResources->portals[i].boundingBox->x + pResources->portals[i].boundingBox->xSize &&
						dude->worldY >= pResources->portals[i].boundingBox->y &&
						dude->worldX <= pResources->portals[i].boundingBox->y + pResources->portals[i].boundingBox->ySize) {
						portal = pResources->portals[i];
						break;
					}
				}
				loadRegion(pResources, portal.destination, 0, 0);
			} else { /* Otherwise, move bg */
				/* Move right */
				/* Move ground */
				pResources->mh1->x++;
				pResources->mh1->diffX++;

	            /* Move water */
	            pResources->mh2->x++;

				/* Move hitmap */
	            pResources->mh3->x++;
				pResources->mh3->diffX++;
			}
		}
	} else if(keyIsDown(BUTTON_LEFT)) {
		if(dude_update(pResources)) {
            /* Check if next tile is portal */
			tq = getTileQuad(pResources->mh3, dude->worldX, dude->worldY);
			if(tq->bl == 1 && tq->br == 1) {
				/* Find correct portal */
                for(i = 0; i < pResources->numPortals; i++) {
					if(dude->worldX >= pResources->portals[i].boundingBox->x &&
						dude->worldX <= pResources->portals[i].boundingBox->x + pResources->portals[i].boundingBox->xSize &&
						dude->worldY >= pResources->portals[i].boundingBox->y &&
						dude->worldX <= pResources->portals[i].boundingBox->y + pResources->portals[i].boundingBox->ySize) {
						portal = pResources->portals[i];
						break;
					}
				}
				loadRegion(pResources, portal.destination, 0, 0);
			} else { /* Otherwise, move bg */
		        /* Move left */
				/* Move ground */
				pResources->mh1->x--;
				pResources->mh1->diffX--;

	            /* Move water */
	            pResources->mh2->x--;

				/* Move hitmap */
	            pResources->mh3->x--;
				pResources->mh3->diffX--;
			}
		}
	} else if(keyHit(BUTTON_A)) {
		if(pResources->sh.dude.dir == LOOK_RIGHT) {
            tq = getTileQuad(pResources->mh3, pResources->sh.dude.worldX + pResources->sh.dude.boundingBox->xSize + 1, pResources->sh.dude.worldY);
		} else if(pResources->sh.dude.dir == LOOK_LEFT) {
            tq = getTileQuad(pResources->mh3, pResources->sh.dude.worldX - 1, pResources->sh.dude.worldY);
		} else if(pResources->sh.dude.dir == LOOK_UP) {
            tq = getTileQuad(pResources->mh3, pResources->sh.dude.worldX, pResources->sh.dude.worldY - 1);
		} else if(pResources->sh.dude.dir == LOOK_DOWN) {
            tq = getTileQuad(pResources->mh3, pResources->sh.dude.worldX, pResources->sh.dude.worldY + pResources->sh.dude.boundingBox->y + pResources->sh.dude.boundingBox->ySize + 1);
		}
        /*sprintf(buff, "%d %d %d %d %d %d", pResources->sh.dude.worldX, pResources->sh.dude.worldY, tq->tl, tq->tr, tq->bl, tq->br);
		reset_text();
		print(3, 3, buff, TILE_ASCI_TRAN);*/
	} else if(keyHit(BUTTON_B)) {
		if(gameState == (HOME + HOME_OUTSIDE)) {
			gameState = ROAD1;
			loadRegion(pResources, ROAD1, 0, 0);
		} else if(gameState == ROAD1) {
			gameState = (TOWN + TOWN_OUTSIDE);
			loadRegion(pResources, (TOWN + TOWN_OUTSIDE), 0, 0);
		} else if(gameState == (TOWN + TOWN_OUTSIDE)) {
            gameState = (OTHER_1 + OTHER_1_OUTSIDE);
			loadRegion(pResources, (OTHER_1 + OTHER_1_OUTSIDE), 0, 0);
		} else if(gameState == (OTHER_1 + OTHER_1_OUTSIDE)) {
            gameState = (OTHER_2 + OTHER_2_OUTSIDE);
			loadRegion(pResources, (OTHER_2 + OTHER_2_OUTSIDE), 0, 0);
		} else if(gameState == (OTHER_2 + OTHER_2_OUTSIDE)) {
            gameState = (OTHER_3 + OTHER_3_OUTSIDE);
			loadRegion(pResources, (OTHER_3 + OTHER_3_OUTSIDE), 0, 0);
		} else {
			gameState = (HOME + HOME_OUTSIDE);
			loadRegion(pResources, (HOME + HOME_OUTSIDE), 0, 0);
		}
	} else {
		/* Idle */
		dude_update(pResources);
	}

    free(tq);

    if(last_timers[1] != my_timers[1]) {
		/* Move water */
  		if(counter_seconds % 2 == 0) {
			pResources->mh2->x += 2;
			if(pResources->mh2->x >= 100) {
				pResources->mh2->x = 0;
			}
		} else {
			pResources->mh2->y--;
            if(pResources->mh2->y <= -100) {
				pResources->mh2->y = 0;
			}
		}
		/* Day/Night cycle */
    	if(counter_seconds > 16 && counter_seconds <= 20){
            for(i = 0; i < 256; i++){
                if(Map_Palette[i] > 0){
                    mask = (1<<6)-1;
                    fat = 0.95;
                    BGPaletteMem[i] = ((int)((BGPaletteMem[i]&mask)*fat)) |
			         (((int)(((BGPaletteMem[i]>>5)&mask)*fat))<<5) |
			         (((int)(((BGPaletteMem[i]>>10)&mask)*fat))<<10);
                }
			}
   	    } else if(counter_seconds > 1 && counter_seconds <= 5){
            for(i = 0; i < 256; i++){
                if(Map_Palette[i] > 0){
                    mask = (1<<6)-1;
                    fat = 1.05;
                    BGPaletteMem[i] = ((int)((BGPaletteMem[i]&mask)*fat)) |
			         (((int)(((BGPaletteMem[i]>>5)&mask)*fat))<<5) |
			         (((int)(((BGPaletteMem[i]>>10)&mask)*fat))<<10);
                }
            }
	    }

	    last_timers[1] = my_timers[1];

        counter_seconds++;
        if(counter_seconds == 7){
            DMAFastCopy((void*) Map_Palette, (void*) BGPaletteMem, 256, DMA_16NOW);
        } else if(counter_seconds > 24){
            counter_seconds = 0;
        }
	}

    updateTimers();
    updateSpriteMemory(pResources->sh.oamBuff);
}

void Draw(ResourcePack* pResources) {
	int dX, dY;
	dY = pResources->mh1->diffY;
	dX = pResources->mh1->diffX;
    if(keyIsDown(BUTTON_UP)) {
		/* Scroll up */
		if(dY == 8 || dY == -8) {
			pResources->mh1->diffY = 0;
            pResources->mh3->diffY = 0;
            scrollVertically(pResources->mh1, TRUE);
            scrollVertically(pResources->mh3, TRUE);
		}
	} else if(keyIsDown(BUTTON_DOWN)) {
		/* Scroll down */
        if(dY == 8 || dY == -8) {
			pResources->mh1->diffY = 0;
            pResources->mh3->diffY = 0;
			scrollVertically(pResources->mh1, FALSE);
			scrollVertically(pResources->mh3, FALSE);
		}
	} else if(keyIsDown(BUTTON_RIGHT)) {
		/* Scroll right */
        if(dX == 8 || dX == -8) {
			pResources->mh1->diffX = 0;
            pResources->mh3->diffX = 0;
            scrollHorizontally(pResources->mh1, TRUE);
            scrollHorizontally(pResources->mh3, TRUE);
		}
	} else if(keyIsDown(BUTTON_LEFT)) {
        /* Scroll left */
        if(dX == 8 || dX == -8) {
			pResources->mh1->diffX = 0;
            pResources->mh3->diffX = 0;
            scrollHorizontally(pResources->mh1, FALSE);
            scrollHorizontally(pResources->mh3, FALSE);
		}
	}
    /* Update BG registers */
	REG_BG1VOFS = pResources->mh1->y;
	REG_BG1HOFS = pResources->mh1->x;
    REG_BG2VOFS = pResources->mh2->y;
	REG_BG2HOFS = pResources->mh2->x;
	REG_BG3VOFS = pResources->mh3->y;
	REG_BG3HOFS = pResources->mh3->x;

	waitVBlank();
}

void updateTimers() {
    my_timers[0] = REG_TM0D / (65536 / 1000);
    my_timers[1] = REG_TM1D;
    my_timers[2] = REG_TM2D / (65536 / 1000);
    my_timers[3] = REG_TM3D;
}

unsigned short getTile(MapHandler* mh, int x, int y) {
	x = (x - (x % 8)) / 8;
	y = (y - (y % 8)) / 8;
	return mh->map[x + y*mh->width];
}

TileQuad* getTileQuad(MapHandler* mh, int x, int y) {
	TileQuad* tq = (TileQuad*) malloc(sizeof(TileQuad));
	bool isTop, isLeft;
	int tmpX, tmpY;
	isTop = FALSE;
	isLeft = FALSE;
    tmpX = (x - (x % 8)) / 8;
	tmpY = (y - (y % 8)) / 8;
	if(x % 2 == 0) {
		isLeft = TRUE;
	}
	if(y % 2 == 0) {
		isTop = TRUE;
	}
	if(isTop && isLeft) {
		tq->tl = mh->map[tmpX + tmpY*mh->width];

		tmpX++;
		tq->tr = mh->map[tmpX + tmpY*mh->width];

        tmpX--;
		tmpY++;
		tq->bl = mh->map[tmpX + tmpY*mh->width];

        tmpX++;
		tq->br = mh->map[tmpX + tmpY*mh->width];
	} else if(isTop && !isLeft) {
        tq->tr = mh->map[tmpX + tmpY*mh->width];

        tmpX--;
        tq->tl = mh->map[tmpX + tmpY*mh->width];

		tmpY++;
		tq->bl = mh->map[tmpX + tmpY*mh->width];

        tmpX++;
		tq->br = mh->map[tmpX + tmpY*mh->width];
	} else if(!isTop && isLeft) {
        tq->bl = mh->map[tmpX + tmpY*mh->width];

		tmpY--;
        tq->tl = mh->map[tmpX + tmpY*mh->width];

        tmpX++;
		tq->tr = mh->map[tmpX + tmpY*mh->width];

		tmpY++;
		tq->br = mh->map[tmpX + tmpY*mh->width];
	} else if(!isTop && !isLeft) {
        tq->br = mh->map[tmpX + tmpY*mh->width];

		tmpY++;
		tq->tr = mh->map[tmpX + tmpY*mh->width];

        tmpX--;
		tq->tl = mh->map[tmpX + tmpY*mh->width];

		tmpY++;
        tq->bl = mh->map[tmpX + tmpY*mh->width];
	}
	return tq;
}

void oam_init(Sprite *oamBuff)
{
	Sprite *obj1= oamBuff;
	Sprite *obj2= (Sprite*)oamMem;
	int i;
	for (i = 0; i < 128; i++)
	{
		obj1[i].attribute0 = (1<<9);
		obj2[i].attribute0 = (1<<9);
	}
	updateSpriteMemory(oamBuff);
}

/* initialize sprites variables*/
void initSpriteHandler(ResourcePack *current_screen, u16 *srcTiles, u32 dudeX, u32 dudeY){
	current_screen->sh.oamBuff = (Sprite*) malloc(sizeof(Sprite)*128);
	oam_init(current_screen->sh.oamBuff);
	current_screen->sh.srcTiles = srcTiles;
	dude_init(&current_screen->sh, dudeX, dudeY, 0);
}

void loadSpriteHandler(SpriteHandler *sh){
	DMAFastCopy((void*) sh->srcTiles, (void*) spriteData, 4096, DMA_32NOW);
	updateSpriteMemory(sh->oamBuff);
}

// Default link attributes
Sprite dudeObjs[1]=
{
	{	0 | MODE_NORMAL 		 | COLOR_256 | TALL, SIZE_32, TID_2D(0,0)| 0, 0	},
};

const u32 id_stand[4]= { TID_2D(4,0), TID_2D(0,0), TID_2D(4,0), TID_2D(8,0) };
const u32 id_walk[4][8]= {
 {TID_2D(4,4), TID_2D(4,4), TID_2D(4,0), TID_2D(4,0), TID_2D(4,8), TID_2D(4,8), TID_2D(4,8), TID_2D(4,0)},
 {TID_2D(0,4), TID_2D(0,4), TID_2D(0,0), TID_2D(0,0), TID_2D(0,8), TID_2D(0,8), TID_2D(0,8), TID_2D(0,0)},
 {TID_2D(4,4), TID_2D(4,4), TID_2D(4,0), TID_2D(4,0), TID_2D(4,8), TID_2D(4,8), TID_2D(4,8), TID_2D(4,0)},
 {TID_2D(8,4), TID_2D(8,4), TID_2D(8,0), TID_2D(8,0), TID_2D(8,8), TID_2D(8,8), TID_2D(8,8), TID_2D(8,0)},
};

void dude_init(SpriteHandler *sh, int x, int y, int dude_id)
{
	sh->dude.x= x;
	sh->dude.y= y;
	sh->dude.worldX = x>>8;
	sh->dude.worldY = y>>8;
	sh->dude.cx = -8;
	sh->dude.cy = -16;

	sh->dude.boundingBox = (BoundingBox*) malloc(sizeof(BoundingBox));
	sh->dude.boundingBox->x = 2;
	sh->dude.boundingBox->y = 25;
	sh->dude.boundingBox->xSize = 14;
	sh->dude.boundingBox->ySize = 7;

	dude_set_state(&sh->dude, SPR_STATE_STAND);
	sh->dude.dir 	= LOOK_DOWN;
	sh->dude.objId = dude_id;
	sh->dude.oamBuff = sh->oamBuff;

	DMAFastCopy(dudeObjs, &(sh->oamBuff[dude_id]), 2, DMA_32NOW);
}

bool dude_update(ResourcePack* pResources){
	int key;
	SpriteCharacter* dude = &pResources->sh.dude;
	key = dude_input(dude);
	if(dude_move(pResources, dude, key)) {
		dude_animate(dude);
		return TRUE;
	}
	return FALSE;
}

void dude_set_state(SpriteCharacter *dude, u32 state)
{
	dude->state= state;
	dude->aniFrame= 0;
}

int dude_input(SpriteCharacter *dude)
{
	int key = 0;
	if( keyIsDown(KEY_RIGHT) )
	{
		dude->dir= LOOK_RIGHT;
		key = KEY_RIGHT;
	}
	else if( keyIsDown(KEY_LEFT) )
	{
		dude->dir= LOOK_LEFT;
		key = KEY_LEFT;
	}

	if( keyIsDown(KEY_DOWN) )
	{
		dude->dir= LOOK_DOWN;
		key = KEY_DOWN;
	}
	else if( keyIsDown(KEY_UP) )
	{
		dude->dir= LOOK_UP;
		key = KEY_UP;
	}

	if( !keyIsDown(KEY_DIR) ) {
		dude_set_state(dude, SPR_STATE_STAND);
	}

	if( keyHit(KEY_DIR) ) {
		dude_set_state(dude, SPR_STATE_WALK);
	}
	return key;
}

bool dude_move(ResourcePack* pResources, SpriteCharacter *dude, int key) {
	unsigned short tile1, tile2, tile3;
	if(key == BUTTON_RIGHT) {
		/* If in bounds, check if the player can move */
		if(pResources->mh1->x < (pResources->mh1->width-1)*8 - 240) {
			/* Check if movement is possible */
			/* New X is on a tile boundary (only top and bot) */
			if((dude->worldX + dude->boundingBox->xSize + 1) % 8 == 0) {
                tile1 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize + 1, dude->worldY + dude->boundingBox->y);
				tile3 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize + 1 , dude->worldY + dude->boundingBox->y + (dude->boundingBox->ySize/2));
                if(determineMove(pResources->mh3, tile1) && determineMove(pResources->mh3, tile3)) {
		            dude->worldX++;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			} else { /* New X is not on a tile boundary (top, mid, bot) */
                tile1 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize + 1, dude->worldY + dude->boundingBox->y);
				tile2 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize + 1, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize/2);
				tile3 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize + 1, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize);
                if(determineMove(pResources->mh3, tile1) && determineMove(pResources->mh3, tile2) && determineMove(pResources->mh3, tile3)) {
		            dude->worldX++;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			}
		} else { /* Otherwise, out of bounds */
			return FALSE;
		}
	} else if(key == BUTTON_LEFT) {
        /* If in bounds, check if the player can move */
		if(pResources->mh1->x > 0) {
			/* Check if movement is possible */
            if((dude->worldX - 1) % 8 == 0) {
                tile1 = getTile(pResources->mh3, dude->worldX - 1, dude->worldY + dude->boundingBox->y);
				tile3 = getTile(pResources->mh3, dude->worldX - 1, dude->worldY + dude->boundingBox->y + (dude->boundingBox->ySize/2));
                if(determineMove(pResources->mh3, tile1) && determineMove(pResources->mh3, tile3)) {
		            dude->worldX--;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			} else { /* X is not on a tile boundary (top, mid, bot) */
                tile1 = getTile(pResources->mh3, dude->worldX - 1, dude->worldY + dude->boundingBox->y);
				tile2 = getTile(pResources->mh3, dude->worldX - 1, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize/2);
				tile3 = getTile(pResources->mh3, dude->worldX - 1, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize);
                if(determineMove(pResources->mh3, tile1) && determineMove(pResources->mh3, tile2) && determineMove(pResources->mh3, tile3)) {
		            dude->worldX--;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			}
		} else { /* Otherwise, out of bounds */
			return FALSE;
		}
	} else if(key == BUTTON_UP) {
        /* If in bounds, check if the player can move */
		if(pResources->mh1->y > 0) {
			/* Check if movement is possible */
            /* Y is on a tile boundary (only left and right) */
            if((dude->worldY - 1) % 8 == 0) {
                tile1 = getTile(pResources->mh3, dude->worldX, dude->worldY + dude->boundingBox->y - 1);
				tile3 = getTile(pResources->mh3, dude->worldX + (dude->boundingBox->xSize/2), dude->worldY + dude->boundingBox->y - 1);
                if(determineMove(pResources->mh3, tile1) && determineMove(pResources->mh3, tile3)) {
		            dude->worldY--;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			} else { /* Y is not on a tile boundary (left, mid, right) */
                tile1 = getTile(pResources->mh3, dude->worldX, dude->worldY + dude->boundingBox->y - 1);
				tile2 = getTile(pResources->mh3, dude->worldX + (dude->boundingBox->xSize/2), dude->worldY + dude->boundingBox->y - 1);
				tile3 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize, dude->worldY + dude->boundingBox->y - 1);
                if(determineMove(pResources->mh3, tile1) && determineMove(pResources->mh3, tile2) && determineMove(pResources->mh3, tile3)) {
		            dude->worldY--;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			}
		} else { /* Otherwise, out of bounds */
			return FALSE;
		}
	} else if(key == BUTTON_DOWN) {
        /* If in bounds, check if the player can move */
		if(pResources->mh1->y < (pResources->mh1->height-1)*8 - 160) {
            /* Check if movement is possible */
            /* Y is on a tile boundary (only left and right) */
            if((dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize) % 8 == 0) {
                tile1 = getTile(pResources->mh3, dude->worldX, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize + 1);
				tile3 = getTile(pResources->mh3, dude->worldX + (dude->boundingBox->xSize/2), dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize + 1);
                if(determineMove(pResources->mh3, tile1) && determineMove(pResources->mh3, tile3)) {
		            dude->worldY++;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			} else { /* Y is not on a tile boundary (left, mid, right) */
                tile1 = getTile(pResources->mh3, dude->worldX, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize + 1);
				tile2 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize/2, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize + 1);
				tile3 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize + 1);
                if(determineMove(pResources->mh3, tile1) && determineMove(pResources->mh3, tile2) && determineMove(pResources->mh3, tile3)) {
		            dude->worldY++;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			}
		} else { /* Otherwise, out of bounds */
			return FALSE;
		}
	}
	/* No movement buttons pressed, update sprite anyway */
	return TRUE;
}

bool determineMove(MapHandler* mh, unsigned short tile) {
	int i, j;
	/* Loop over pixels in tile */
	if(tile != 0) {
		for(i = 0; i < 16; i++) {
			for(j = 0; j < 16; j++) {
				/* If hitmap color found, return FALSE */
				if(getPixelColor(mh, tile, j, i) == HITMAP_WATER_COLOR) {
					return FALSE;
				}
			}
		}
	}
	return TRUE;
}

void dude_animate(SpriteCharacter *dude){
	switch(dude->state)
	{
	case SPR_STATE_STAND:
		dude_ani_stand(dude);
		break;
	case SPR_STATE_WALK:
		dude->aniFrame += DUDE_SPEED/3;
		dude_ani_walk(dude);
	}
}

void dude_ani_stand(SpriteCharacter *dude){
	Sprite *obj= &dude->oamBuff[dude->objId];
	PT pt = { (dude->x>>8) , (dude->y>>8) };
	int dir = dude->dir;

	BFN_SET(obj[0].attribute0, pt.y, ATTR0_Y);
	BFN_SET(obj[0].attribute1, pt.x, ATTR1_X);
	BFN_SET(obj[0].attribute1,    0, ATTR1_FLIP);
	obj[0].attribute2 = id_stand[dir] | 0;
	if(dir == LOOK_LEFT) obj[0].attribute1 |= HORIZONTAL_FLIP;

}
void dude_ani_walk(SpriteCharacter *dude){
	Sprite *obj= &dude->oamBuff[dude->objId];
	PT pt = { (dude->x>>8) , (dude->y>>8) };
	int dir = dude->dir;
	int frame = (dude->aniFrame>>8) & 7;

	BFN_SET(obj[0].attribute0, pt.y, ATTR0_Y);
	BFN_SET(obj[0].attribute1, pt.x, ATTR1_X);
	BFN_SET(obj[0].attribute1,    0, ATTR1_FLIP);
	obj[0].attribute2 = id_walk[dir][(frame)] |  0;
	if(dir == LOOK_LEFT) obj[0].attribute1 |= HORIZONTAL_FLIP;

}

void loadRegion(ResourcePack* pResources, int destCode, int x, int y) {
    pResources->mh1->x = x;
    pResources->mh1->y = y;
    pResources->mh1->diffX = 0;
    pResources->mh1->diffY = 0;
    pResources->mh1->bgNextCol = 30;
    pResources->mh1->bgNextRow = 20;
    pResources->mh1->bgPrevCol = 31;
    pResources->mh1->bgPrevRow = 31;
    pResources->mh1->mapNextCol = 30;
    pResources->mh1->mapNextRow = 20;
    pResources->mh3->x = pResources->mh1->x;
    pResources->mh3->y = pResources->mh1->y;
    pResources->mh3->diffX = pResources->mh1->diffX;
    pResources->mh3->diffY = pResources->mh1->diffY;
    pResources->mh3->bgNextCol = pResources->mh1->bgNextCol;
    pResources->mh3->bgNextRow = pResources->mh1->bgNextRow;
    pResources->mh3->bgPrevCol = pResources->mh1->bgPrevCol;
    pResources->mh3->bgPrevRow = pResources->mh1->bgPrevRow;
    pResources->mh3->mapNextCol = pResources->mh1->mapNextCol;
    pResources->mh3->mapNextRow = pResources->mh1->mapNextRow;
	switch(destCode) {
		case (HOME + HOME_OUTSIDE):
			pResources->mh1->mapPrevCol = (HOME_FARM_WIDTH)-1;
			pResources->mh1->mapPrevRow = (HOME_FARM_HEIGHT)-1;
			pResources->mh1->width = HOME_FARM_WIDTH;
			pResources->mh1->height = HOME_FARM_HEIGHT;
			pResources->mh1->tiles = Home_Farm_Tiles;
			pResources->mh1->map = Home_Farm_Map;
            pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
			pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
			pResources->mh3->width = pResources->mh1->width;
			pResources->mh3->height = pResources->mh1->height;
			pResources->mh3->tiles = Home_Farm_Hitmap_Tiles;
			pResources->mh3->map = Home_Farm_Hitmap_Map;
            loadRegionPal(Map_Palette);
            loadRegionMap(pResources->mh1, Home_Farm_Tiles, HOME_FARM_TILES_NUM, 1);
            loadRegionMap(pResources->mh3, Home_Farm_Hitmap_Tiles, HOME_FARM_HITMAP_TILES_NUM, 3);
            loadPortals(pResources->portals, &pResources->numPortals, HOME + HOME_OUTSIDE);
			break;
		case (HOME + HOME_HOUSE):
			break;
		case (HOME + HOME_SHED):
			break;
		case (OTHER_1 + OTHER_1_OUTSIDE):
            pResources->mh1->mapPrevCol = (OTHER_FARM1_WIDTH)-1;
			pResources->mh1->mapPrevRow = (OTHER_FARM1_HEIGHT)-1;
			pResources->mh1->width = OTHER_FARM1_WIDTH;
			pResources->mh1->height = OTHER_FARM1_HEIGHT;
			pResources->mh1->tiles = Other_Farm1_Tiles;
			pResources->mh1->map = Other_Farm1_Map;
            pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
			pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
			pResources->mh3->width = pResources->mh1->width;
			pResources->mh3->height = pResources->mh1->height;
            pResources->mh3->tiles = Other_Farm1_Tiles;
			pResources->mh3->map = Other_Farm1_Hitmap_Map;
            loadRegionPal(Map_Palette);
            loadRegionMap(pResources->mh1, Other_Farm1_Tiles, OTHER_FARM1_TILES_NUM, 1);
            loadRegionMap(pResources->mh3, Other_Farm1_Hitmap_Tiles, OTHER_FARM1_HITMAP_TILES_NUM, 3);
            loadPortals(pResources->portals, &pResources->numPortals, OTHER_1 + OTHER_1_OUTSIDE);
			break;
		case (OTHER_1 + OTHER_1_HOUSE):
			break;
		case (OTHER_2 + OTHER_2_OUTSIDE):
            pResources->mh1->mapPrevCol = (OTHER_FARM2_WIDTH)-1;
			pResources->mh1->mapPrevRow = (OTHER_FARM2_HEIGHT)-1;
			pResources->mh1->width = OTHER_FARM2_WIDTH;
			pResources->mh1->height = OTHER_FARM2_HEIGHT;
            pResources->mh1->tiles = Other_Farm2_Tiles;
			pResources->mh1->map = Other_Farm2_Map;
            pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
			pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
			pResources->mh3->width = pResources->mh1->width;
			pResources->mh3->height = pResources->mh1->height;
            pResources->mh3->tiles = Other_Farm2_Hitmap_Tiles;
			pResources->mh3->map = Other_Farm2_Hitmap_Map;
            loadRegionPal(Map_Palette);
            loadRegionMap(pResources->mh1, Other_Farm2_Tiles, OTHER_FARM2_TILES_NUM, 1);
            loadRegionMap(pResources->mh3, Other_Farm2_Hitmap_Tiles, OTHER_FARM2_HITMAP_TILES_NUM, 3);
            loadPortals(pResources->portals, &pResources->numPortals, OTHER_2 + OTHER_2_OUTSIDE);
			break;
		case (OTHER_2 + OTHER_2_HOUSE):
			break;
        case (OTHER_3 + OTHER_3_OUTSIDE):
            pResources->mh1->mapPrevCol = (OTHER_FARM3_WIDTH)-1;
			pResources->mh1->mapPrevRow = (OTHER_FARM3_HEIGHT)-1;
			pResources->mh1->width = OTHER_FARM3_WIDTH;
			pResources->mh1->height = OTHER_FARM3_HEIGHT;
            pResources->mh1->tiles = Other_Farm3_Tiles;
			pResources->mh1->map = Other_Farm3_Map;
            pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
			pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
			pResources->mh3->width = pResources->mh1->width;
			pResources->mh3->height = pResources->mh1->height;
            pResources->mh3->tiles = Other_Farm3_Hitmap_Tiles;
			pResources->mh3->map = Other_Farm3_Hitmap_Map;
            loadRegionPal(Map_Palette);
            loadRegionMap(pResources->mh1, Other_Farm3_Tiles, OTHER_FARM3_TILES_NUM, 1);
            loadRegionMap(pResources->mh3, Other_Farm3_Hitmap_Tiles, OTHER_FARM3_HITMAP_TILES_NUM, 3);
            loadPortals(pResources->portals, &pResources->numPortals, OTHER_3 + OTHER_3_OUTSIDE);
			break;
		case (OTHER_3 + OTHER_3_HOUSE):
			break;
		case (TOWN + TOWN_OUTSIDE):
            pResources->mh1->mapPrevCol = (TOWN_WIDTH)-1;
			pResources->mh1->mapPrevRow = (TOWN_HEIGHT)-1;
			pResources->mh1->width = TOWN_WIDTH;
			pResources->mh1->height = TOWN_HEIGHT;
            pResources->mh1->tiles = Town_Tiles;
			pResources->mh1->map = Town_Map;
            pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
			pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
			pResources->mh3->width = pResources->mh1->width;
			pResources->mh3->height = pResources->mh1->height;
            pResources->mh3->tiles = Town_Hitmap_Tiles;
			pResources->mh3->map = Town_Hitmap_Map;
			loadRegionPal(Map_Palette);
            loadRegionMap(pResources->mh1, Town_Tiles, TOWN_TILES_NUM, 1);
            loadRegionMap(pResources->mh3, Town_Hitmap_Tiles, TOWN_HITMAP_TILES_NUM, 3);
			loadPortals(pResources->portals, &pResources->numPortals, TOWN + TOWN_OUTSIDE);
			break;
		case (TOWN + TOWN_STORE):
			break;
		case (TOWN + TOWN_FT):
			break;
		case (ROAD1):
            pResources->mh1->mapPrevCol = (ROAD1_WIDTH)-1;
			pResources->mh1->mapPrevRow = (ROAD1_HEIGHT)-1;
			pResources->mh1->width = ROAD1_WIDTH;
			pResources->mh1->height = ROAD1_HEIGHT;
            pResources->mh1->tiles = Road1_Tiles;
			pResources->mh1->map = Road1_Map;
            pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
			pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
			pResources->mh3->width = pResources->mh1->width;
			pResources->mh3->height = pResources->mh1->height;
            pResources->mh3->tiles = Road1_Hitmap_Tiles;
			pResources->mh3->map = Road1_Hitmap_Map;
            loadRegionPal(Map_Palette);
            loadRegionMap(pResources->mh1, Road1_Tiles, ROAD1_TILES_NUM, 1);
            loadRegionMap(pResources->mh3, Road1_Hitmap_Tiles, ROAD1_HITMAP_TILES_NUM, 3);
			loadPortals(pResources->portals, &pResources->numPortals, ROAD1);
			break;
	}
}

inline void loadRegionPal(const unsigned short* palette) {
    DMAFastCopy((void*) palette, (void*) BGPaletteMem, 256, DMA_16NOW);
}

void loadRegionMap(MapHandler* mh, const unsigned char* tiles, int numTiles, int cBB) {
	int i;
	DMAFastCopy((void*) tiles, (void*) charBaseBlock(cBB), numTiles/4, DMA_32NOW);
	for(i = 0; i < 32; i++) {
        copyCol(mh->bg, mh->map, 0, i, 0, i, mh->width, mh->height);
	}
}

void loadPortals(Portal* portals, int* numPortals, int code) {
    free(portals);
	switch(code) {
		case HOME + HOME_OUTSIDE:
			(*numPortals) = HOME_FARM_OUTSIDE_PORTALS_NUM;
			portals = (Portal*) malloc(sizeof(Portal)*(*numPortals));
			portals[0].destination = ROAD1;
			portals[0].destX = 0;
			portals[0].destY = 0;
			portals[0].boundingBox->x = 26*8;
			portals[0].boundingBox->xSize = 48;
			portals[0].boundingBox->y = 20*8;
			portals[0].boundingBox->ySize = 32;

            portals[1].destination = HOME + HOME_HOUSE;
			portals[1].destX = 0;
			portals[1].destY = 0;
			portals[1].boundingBox->x = 58*8;
			portals[1].boundingBox->xSize = 16;
			portals[1].boundingBox->y = 46*8;
			portals[1].boundingBox->ySize = 16;

            portals[2].destination = HOME + HOME_SHED;
			portals[2].destX = 0;
			portals[2].destY = 0;
			portals[2].boundingBox->x = 82*8;
			portals[2].boundingBox->xSize = 16;
			portals[2].boundingBox->y = 48*8;
			portals[2].boundingBox->ySize = 16;
			break;
		case HOME + HOME_HOUSE:
			break;
		case HOME + HOME_SHED:
			break;
		case TOWN + TOWN_OUTSIDE:
            (*numPortals) = TOWN_OUTSIDE_PORTALS_NUM;
			portals = (Portal*) malloc(sizeof(Portal)*(*numPortals));
            portals[0].destination = ROAD1;
			portals[0].destX = 0;
			portals[0].destY = 0;
			portals[0].boundingBox->x = 136*8;
			portals[0].boundingBox->xSize = 32;
			portals[0].boundingBox->y = 50*8;
			portals[0].boundingBox->ySize = 80;

            portals[1].destination = TOWN + TOWN_STORE;
			portals[1].destX = 0;
			portals[1].destY = 0;
			portals[1].boundingBox->x = 54*8;
			portals[1].boundingBox->xSize = 16;
			portals[1].boundingBox->y = 74*8;
			portals[1].boundingBox->ySize = 16;

            portals[2].destination = TOWN + TOWN_FT;
			portals[2].destX = 0;
			portals[2].destY = 0;
			portals[2].boundingBox->x = 102*8;
			portals[2].boundingBox->xSize = 16;
			portals[2].boundingBox->y = 44*8;
			portals[2].boundingBox->ySize = 16;
			break;
		case TOWN + TOWN_FT:
			break;
		case TOWN + TOWN_STORE:
			break;
		case ROAD1:
            (*numPortals) = ROAD1_PORTALS_NUM;
			portals = (Portal*) malloc(sizeof(Portal)*(*numPortals));
            portals[0].destination = TOWN + TOWN_OUTSIDE;
			portals[0].destX = 0;
			portals[0].destY = 0;
			portals[0].boundingBox->x = 20*8;
			portals[0].boundingBox->xSize = 32;
			portals[0].boundingBox->y = 98*8;
			portals[0].boundingBox->ySize = 80;

            portals[1].destination = OTHER_2 + OTHER_2_OUTSIDE;
			portals[1].destX = 0;
			portals[1].destY = 0;
			portals[1].boundingBox->x = 138*8;
			portals[1].boundingBox->xSize = 48;
			portals[1].boundingBox->y = 20*8;
			portals[1].boundingBox->ySize = 32;

            portals[2].destination = OTHER_1 + OTHER_1_OUTSIDE;
			portals[2].destX = 0;
			portals[2].destY = 0;
			portals[2].boundingBox->x = 166*8;
			portals[2].boundingBox->xSize = 32;
			portals[2].boundingBox->y = 68*8;
			portals[2].boundingBox->ySize = 48;

            portals[3].destination = OTHER_3 + OTHER_3_OUTSIDE;
			portals[3].destX = 0;
			portals[3].destY = 0;
			portals[3].boundingBox->x = 166*8;
			portals[3].boundingBox->xSize = 32;
			portals[3].boundingBox->y = 148*8;
			portals[3].boundingBox->ySize = 48;

            portals[4].destination = HOME + HOME_OUTSIDE;
			portals[4].destX = 0;
			portals[4].destY = 0;
			portals[4].boundingBox->x = 114*8;
			portals[4].boundingBox->xSize = 48;
			portals[4].boundingBox->y = 216*8;
			portals[4].boundingBox->ySize = 32;
			break;
		case OTHER_1 + OTHER_1_OUTSIDE:
            (*numPortals) = OTHER_FARM1_OUTSIDE_PORTALS_NUM;
			portals = (Portal*) malloc(sizeof(Portal)*(*numPortals));
            portals[0].destination = ROAD1;
			portals[0].destX = 0;
			portals[0].destY = 0;
			portals[0].boundingBox->x = 20*8;
			portals[0].boundingBox->xSize = 32;
			portals[0].boundingBox->y = 72*8;
			portals[0].boundingBox->ySize = 64;

            portals[1].destination = OTHER_1 + OTHER_1_HOUSE;
			portals[1].destX = 0;
			portals[1].destY = 0;
			portals[1].boundingBox->x = 72*8;
			portals[1].boundingBox->xSize = 16;
			portals[1].boundingBox->y = 28*8;
			portals[1].boundingBox->ySize = 16;
			break;
		case OTHER_1 + OTHER_1_HOUSE:
			break;
		case OTHER_2 + OTHER_2_OUTSIDE:
            (*numPortals) = OTHER_FARM2_OUTSIDE_PORTALS_NUM;
			portals = (Portal*) malloc(sizeof(Portal)*(*numPortals));
            portals[0].destination = ROAD1;
			portals[0].destX = 0;
			portals[0].destY = 0;
			portals[0].boundingBox->x = 54*8;
			portals[0].boundingBox->xSize = 48;
			portals[0].boundingBox->y = 106*8;
			portals[0].boundingBox->ySize = 32;

            portals[1].destination = OTHER_2 + OTHER_2_HOUSE;
			portals[1].destX = 0;
			portals[1].destY = 0;
			portals[1].boundingBox->x = 30*8;
			portals[1].boundingBox->xSize = 16;
			portals[1].boundingBox->y = 64*8;
			portals[1].boundingBox->ySize = 16;
			break;
		case OTHER_2 + OTHER_2_HOUSE:
			break;
		case OTHER_3 + OTHER_3_OUTSIDE:
            (*numPortals) = OTHER_FARM3_OUTSIDE_PORTALS_NUM;
			portals = (Portal*) malloc(sizeof(Portal)*(*numPortals));
            portals[0].destination = ROAD1;
			portals[0].destX = 0;
			portals[0].destY = 0;
			portals[0].boundingBox->x = 60*8;
			portals[0].boundingBox->xSize = 16;
			portals[0].boundingBox->y = 28*8;
			portals[0].boundingBox->ySize = 16;

            portals[1].destination = OTHER_3 + OTHER_3_HOUSE;
			portals[1].destX = 0;
			portals[1].destY = 0;
			portals[1].boundingBox->x = 20*8;
			portals[1].boundingBox->xSize = 32;
			portals[1].boundingBox->y = 36*8;
			portals[1].boundingBox->ySize = 64;
			break;
		case OTHER_3 + OTHER_3_HOUSE:
			break;
		default:
			break;
	}
}

unsigned char getPixelColor(MapHandler* mh, u16 tile, int x, int y) {
	int tileOffset;
	if(x >= 8 || y >= 8 || x < 0 || y < 0) {
		return 0;
	} else {
		tileOffset = (64)*tile;
		return mh->tiles[tileOffset + y*8 + x];
	}
}