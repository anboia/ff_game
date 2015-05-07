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
#define HITMAP_FARMABLE_COLOR (BGPaletteMem & 22)
#define HITMAP_PORTAL_COLOR (BGPaletteMem & 24)
#define HITMAP_WATER_COLOR (BGPaletteMem & 28)

// globals

plants allPlants[NUM_PLANTS]; // array of all the plants


/* Region loading */
void loadRegion(ResourcePack*, int);
void loadRegionMap(MapHandler*, const unsigned char*, int, int);
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
bool determineMove(unsigned short, int, int);

// farming stuff?
bool tqCheck(TileQuad tq);
void updatePlants(void);

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
	gameState = HOME + HOME_OUTSIDE;

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
    
	int i;
	for(i=0;i<NUM_PLANTS;i++)
		allPlants[i].type = NO_PLANT; // initialazing the plants array (you farm nothing farmer snow)
	return pResources;
}

void LoadContent(ResourcePack* pResources) {
	/* Load initial region (ground + hitmap) */
    loadRegion(pResources, (HOME + HOME_OUTSIDE));

	/* Load water map */
    loadRegionMap(pResources->mh2, Water_Tiles, WATER_TILES_NUM, 2);

	/* Load sprites into memory */
    loadSpriteHandler(&pResources->sh);
    DMAFastCopy((void*) sprite_data_2d_8bppPal, (void*) SPaletteMem,  128, DMA_32NOW);
}

void Update(ResourcePack* pResources) {
	int i, mask;
	double fat;
	/*char buff[30];*/
	TileQuad* tq = (TileQuad*) malloc(sizeof(TileQuad));

	keyPoll();
 	if(dude_update(pResources)) {
		if(keyIsDown(BUTTON_UP)) {
			/* Move up */
			/* Move ground */
			pResources->mh1->y--;
			pResources->mh1->diffY--;

			/* Move water */
			pResources->mh2->y--;

			/* Move hitmap */
            pResources->mh3->y--;
			pResources->mh3->diffY--;
		} else if(keyIsDown(BUTTON_DOWN)) {
			/* Move down */
			/* Move ground */
			pResources->mh1->y++;
			pResources->mh1->diffY++;

			/* Move water */
            pResources->mh2->y++;

			/* Move hitmap */
            pResources->mh3->y++;
			pResources->mh3->diffY++;
		} else if(keyIsDown(BUTTON_RIGHT)) {
			/* Move right */
			/* Move ground */
			pResources->mh1->x++;
			pResources->mh1->diffX++;

            /* Move water */
            pResources->mh2->x++;

			/* Move hitmap */
            pResources->mh3->x++;
			pResources->mh3->diffX++;
		} else if(keyIsDown(BUTTON_LEFT)) {
	        /* Move left */
			/* Move ground */
			pResources->mh1->x--;
			pResources->mh1->diffX--;

            /* Move water */
            pResources->mh2->x--;

			/* Move hitmap */
            pResources->mh3->x--;
			pResources->mh3->diffX--;
		} else if(keyHit(BUTTON_A)) {
			if(pResources->sh.dude.dir == LOOK_RIGHT) {
                tq = getTileQuad(pResources->mh3, pResources->sh.dude.worldX + pResources->sh.dude.boundingBox->xSize + 1, pResources->sh.dude.worldY);
				if(tqCheck(tq)){ //it's farmable

				}
			} else if(pResources->sh.dude.dir == LOOK_LEFT) {
                tq = getTileQuad(pResources->mh3, pResources->sh.dude.worldX - 1, pResources->sh.dude.worldY);
				if(tqCheck(tq)){//it's farmable

				}
			} else if(pResources->sh.dude.dir == LOOK_UP) {
                tq = getTileQuad(pResources->mh3, pResources->sh.dude.worldX, pResources->sh.dude.worldY - 1);
                if(tqCheck(tq)){//it's farmable

				}
			} else if(pResources->sh.dude.dir == LOOK_DOWN) {
                tq = getTileQuad(pResources->mh3, pResources->sh.dude.worldX, pResources->sh.dude.worldY + pResources->sh.dude.boundingBox->y + pResources->sh.dude.boundingBox->ySize + 1);
                if(tqCheck(tq)){//it's farmable

				}
			}
            /*sprintf(buff, "%d %d %d %d %d %d", pResources->sh.dude.worldX, pResources->sh.dude.worldY, tq->tl, tq->tr, tq->bl, tq->br);
			reset_text();
			print(3, 3, buff, TILE_ASCI_TRAN);*/
		} else if(keyHit(BUTTON_B)) {
			if(gameState == (HOME + HOME_OUTSIDE)) {
				gameState = ROAD1;
				loadRegion(pResources, ROAD1);
			} else if(gameState == ROAD1) {
				gameState = (TOWN + TOWN_OUTSIDE);
				loadRegion(pResources, (TOWN + TOWN_OUTSIDE));
			} else if(gameState == (TOWN + TOWN_OUTSIDE)) {
	            gameState = (OTHER_1 + OTHER_1_OUTSIDE);
				loadRegion(pResources, (OTHER_1 + OTHER_1_OUTSIDE));
			} else if(gameState == (OTHER_1 + OTHER_1_OUTSIDE)) {
	            gameState = (OTHER_2 + OTHER_2_OUTSIDE);
				loadRegion(pResources, (OTHER_2 + OTHER_2_OUTSIDE));
			} else if(gameState == (OTHER_2 + OTHER_2_OUTSIDE)) {
	            gameState = (OTHER_3 + OTHER_3_OUTSIDE);
				loadRegion(pResources, (OTHER_3 + OTHER_3_OUTSIDE));
			} else {
				gameState = (HOME + HOME_OUTSIDE);
				loadRegion(pResources, (HOME + HOME_OUTSIDE));
			}
		}
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
			updatePlants();
            DMAFastCopy((void*) Map_Palette, (void*) BGPaletteMem, 256, DMA_16NOW);
        } else if(counter_seconds > 24){
            counter_seconds = 0;
        }
	}

    updateTimers();
    updateSpriteMemory(pResources->sh.oamBuff);
}

// function to check if its a famring tile
bool tqCheck(TileQuad tq){

    unsigned short tl, tr, bl, br;
	tl = tq->tl;
	tr = tq->tr;
	bl = tq->bl;
	br = tq->br;
	if(tl == 11 && tr == 11 && bl == 11 && br = 11) //check if its a farming tile
		return true;
	return false;

}

//----------------------------------------- olha a nova funcao ai galere-----------------------------------------//
void updatePlants(void){
	int i;
	int stage;
	for(i=0;i<NUM_PLANTS;i++){
		if(allPlants[i].type!=NO_PLANT){
			allPlants[i].age++;
			if(!allPlants[i].water)
				allPlants[i].noWaterDays++;
			if(allPlants[i].noWaterDays>=2)
				allPlants.type = NO_PLANT;
			else{
				allPlants[i].water = 0;
				if(allPlants[i].type == TOMATOS){
					stage = allPlants[i].stage;
					if(stage==SEED && age>3){
						//change the sprite
						stage = GROWING1;
					}else if (stage==GROWING1 && age>6){
						//change the sprite
						stage = GROWING2;
					}else if(stage == GROWING2 && age>9){
						//change the sprite
						stage = DONE;
					}
					allPlants[i].stage = stage;
				} else if(allPlants[i].type == ONIONS){
                    stage = allPlants[i].stage;
					if(stage==SEED && age>4){
						//change the sprite
						stage = GROWING1;
					}else if (stage==GROWING1 && age>8){
						//change the sprite
						stage = GROWING2;
					}else if(stage == GROWING2 && age>10){
						//change the sprite
						stage = DONE;
					}
					allPlants[i].stage = stage;
				} else if(allPlants[i].type == MELONS){
                    stage = allPlants[i].stage;
					if(stage==SEED && age>5){
						//change the sprite
						stage = GROWING1;
					}else if (stage==GROWING1 && age>10){
						//change the sprite
						stage = GROWING2;
					}else if(stage == GROWING2 && age>13){
						//change the sprite
						stage = DONE;
					}
					allPlants[i].stage = stage;
				} else if(allPlants[i].type == MUSHROOM){
                    stage = allPlants[i].stage;
					if(stage==SEED && age>2){
						//change the sprite
						stage = GROWING1;
					}else if (stage==GROWING1 && age>4){
						//change the sprite
						stage = GROWING2;
					}else if(stage == GROWING2 && age>6){
						//change the sprite
						stage = DONE;
					}
					allPlants[i].stage = stage;
				}
			}

		}
	}

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

void copyRow(unsigned short* bg, const unsigned short* map, int bgRow, int bgCol, int mRow, int mCol, int width) {
    int i;
	for(i = 0; i < 32; i++) {
		bg[bgRow*32 + (bgCol+i)%32] = map[mRow*width + (mCol+i)%width];
	}
}

void copyCol(unsigned short* bg, const unsigned short* map, int bgRow, int bgCol, int mRow, int mCol, int width, int height) {
	int i;
	for(i = 0; i < 32; i++) {
		bg[((bgRow+i)%32)*32 + bgCol] = map[((mRow+i)%height)*width + mCol];
	}
}

void scrollHorizontally(MapHandler* mh, bool isRight) {
	if(isRight) {
        if(mh->bgPrevCol < 31) {
			mh->bgPrevCol++;
		} else {
			mh->bgPrevCol = 0;
		}
        if(mh->bgNextCol < 31) {
			mh->bgNextCol++;
		} else {
			mh->bgNextCol = 0;
		}
		if(mh->mapPrevCol < mh->width-1) {
			mh->mapPrevCol++;
		} else {
			mh->mapPrevCol = 0;
		}
		if(mh->mapNextCol < mh->width-1) {
			mh->mapNextCol++;
		} else {
			mh->mapNextCol = 0;
		}
	} else {
        if(mh->bgPrevCol > 0) {
			mh->bgPrevCol--;
		} else {
			mh->bgPrevCol = 31;
		}
        if(mh->bgNextCol > 0) {
			mh->bgNextCol--;
		} else {
			mh->bgNextCol = 31;
		}
		if(mh->mapPrevCol > 0) {
			mh->mapPrevCol--;
		} else {
			mh->mapPrevCol = mh->width-1;
		}
		if(mh->mapNextCol > 0) {
			mh->mapNextCol--;
		} else {
			mh->mapNextCol = mh->width-1;
		}
	}
	copyCol(mh->bg, mh->map, mh->y/8, mh->bgNextCol, mh->y/8, mh->mapNextCol, mh->width, mh->height);
	copyCol(mh->bg, mh->map, mh->y/8, mh->bgPrevCol, mh->y/8, mh->mapPrevCol, mh->width, mh->height);
    copyRow(mh->bg, mh->map, mh->bgNextRow, mh->x/8, mh->mapNextRow, mh->x/8, mh->width);
	copyRow(mh->bg, mh->map, mh->bgPrevRow, mh->x/8, mh->mapPrevRow, mh->x/8, mh->width);
}

void scrollVertically(MapHandler* mh, bool isUp) {
	if(isUp) {
        if(mh->bgPrevRow > 0) {
			mh->bgPrevRow--;
		} else {
			mh->bgPrevRow = 31;
		}
        if(mh->bgNextRow > 0) {
			mh->bgNextRow--;
		} else {
			mh->bgNextRow = 31;
		}
		if(mh->mapPrevRow > 0) {
			mh->mapPrevRow--;
		} else {
			mh->mapPrevRow = mh->height-1;
		}
		if(mh->mapNextRow > 0) {
			mh->mapNextRow--;
		} else {
			mh->mapNextRow = mh->height-1;
		}
	} else {
        if(mh->bgPrevRow < 31) {
			mh->bgPrevRow++;
		} else {
			mh->bgPrevRow = 0;
		}
        if(mh->bgNextRow < 31) {
			mh->bgNextRow++;
		} else {
			mh->bgNextRow = 0;
		}
		if(mh->mapPrevRow < mh->height-1) {
			mh->mapPrevRow++;
		} else {
			mh->mapPrevRow = 0;
		}
		if(mh->mapNextRow < mh->height-1) {
			mh->mapNextRow++;
		} else {
			mh->mapNextRow = 0;
		}
	}
	copyRow(mh->bg, mh->map, mh->bgNextRow, mh->x/8, mh->mapNextRow, mh->x/8, mh->width);
	copyRow(mh->bg, mh->map, mh->bgPrevRow, mh->x/8, mh->mapPrevRow, mh->x/8, mh->width);
    copyCol(mh->bg, mh->map, mh->y/8, mh->bgNextCol, mh->y/8, mh->mapNextCol, mh->width, mh->height);
	copyCol(mh->bg, mh->map, mh->y/8, mh->bgPrevCol, mh->y/8, mh->mapPrevCol, mh->width, mh->height);
}

void loadRegion(ResourcePack* pResources, int destCode) {
	u16 tmpState = gameState;
	if((tmpState << 8) != destCode) {
        pResources->mh1->x = 0;
        pResources->mh1->y = 0;
	    pResources->mh1->diffX = 0;
	    pResources->mh1->diffY = 0;
	    pResources->mh1->bgNextCol = 30;
	    pResources->mh1->bgNextRow = 21;
	    pResources->mh1->bgPrevCol = 31;
	    pResources->mh1->bgPrevRow = 31;
	    pResources->mh1->mapNextCol = 30;
	    pResources->mh1->mapNextRow = 21;
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
				pResources->mh1->map = Home_Farm_Map;
                pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
				pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
				pResources->mh3->width = pResources->mh1->width;
				pResources->mh3->height = pResources->mh1->height;
				pResources->mh3->map = Home_Farm_Hitmap_Map;
                loadRegionPal(Map_Palette);
                loadRegionMap(pResources->mh1, Home_Farm_Tiles, HOME_FARM_TILES_NUM, 1);
                loadRegionMap(pResources->mh3, Home_Farm_Hitmap_Tiles, HOME_FARM_HITMAP_TILES_NUM, 3);
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
				pResources->mh1->map = Other_Farm1_Map;
                pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
				pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
				pResources->mh3->width = pResources->mh1->width;
				pResources->mh3->height = pResources->mh1->height;
				pResources->mh3->map = Other_Farm1_Hitmap_Map;
                loadRegionPal(Map_Palette);
                loadRegionMap(pResources->mh1, Other_Farm1_Tiles, OTHER_FARM1_TILES_NUM, 1);
                loadRegionMap(pResources->mh3, Other_Farm1_Hitmap_Tiles, OTHER_FARM1_HITMAP_TILES_NUM, 3);
				break;
			case (OTHER_1 + OTHER_1_HOUSE):
				break;
			case (OTHER_2 + OTHER_2_OUTSIDE):
                pResources->mh1->mapPrevCol = (OTHER_FARM2_WIDTH)-1;
				pResources->mh1->mapPrevRow = (OTHER_FARM2_HEIGHT)-1;
				pResources->mh1->width = OTHER_FARM2_WIDTH;
				pResources->mh1->height = OTHER_FARM2_HEIGHT;
				pResources->mh1->map = Other_Farm2_Map;
                pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
				pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
				pResources->mh3->width = pResources->mh1->width;
				pResources->mh3->height = pResources->mh1->height;
				pResources->mh3->map = Other_Farm2_Hitmap_Map;
                loadRegionPal(Map_Palette);
                loadRegionMap(pResources->mh1, Other_Farm2_Tiles, OTHER_FARM2_TILES_NUM, 1);
                loadRegionMap(pResources->mh3, Other_Farm2_Hitmap_Tiles, OTHER_FARM2_HITMAP_TILES_NUM, 3);
				break;
			case (OTHER_2 + OTHER_2_HOUSE):
				break;
            case (OTHER_3 + OTHER_3_OUTSIDE):
                pResources->mh1->mapPrevCol = (OTHER_FARM3_WIDTH)-1;
				pResources->mh1->mapPrevRow = (OTHER_FARM3_HEIGHT)-1;
				pResources->mh1->width = OTHER_FARM3_WIDTH;
				pResources->mh1->height = OTHER_FARM3_HEIGHT;
				pResources->mh1->map = Other_Farm3_Map;
                pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
				pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
				pResources->mh3->width = pResources->mh1->width;
				pResources->mh3->height = pResources->mh1->height;
				pResources->mh3->map = Other_Farm3_Hitmap_Map;
                loadRegionPal(Map_Palette);
                loadRegionMap(pResources->mh1, Other_Farm3_Tiles, OTHER_FARM3_TILES_NUM, 1);
                loadRegionMap(pResources->mh3, Other_Farm3_Hitmap_Tiles, OTHER_FARM3_HITMAP_TILES_NUM, 3);
				break;
			case (OTHER_3 + OTHER_3_HOUSE):
				break;
			case (TOWN + TOWN_OUTSIDE):
                pResources->mh1->mapPrevCol = (TOWN_WIDTH)-1;
				pResources->mh1->mapPrevRow = (TOWN_HEIGHT)-1;
				pResources->mh1->width = TOWN_WIDTH;
				pResources->mh1->height = TOWN_HEIGHT;
				pResources->mh1->map = Town_Map;
                pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
				pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
				pResources->mh3->width = pResources->mh1->width;
				pResources->mh3->height = pResources->mh1->height;
				pResources->mh3->map = Town_Hitmap_Map;
				loadRegionPal(Map_Palette);
                loadRegionMap(pResources->mh1, Town_Tiles, TOWN_TILES_NUM, 1);
                loadRegionMap(pResources->mh3, Town_Hitmap_Tiles, TOWN_HITMAP_TILES_NUM, 3);
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
				pResources->mh1->map = Road1_Map;
                pResources->mh3->mapPrevCol = pResources->mh1->mapPrevCol;
				pResources->mh3->mapPrevRow = pResources->mh1->mapPrevRow;
				pResources->mh3->width = pResources->mh1->width;
				pResources->mh3->height = pResources->mh1->height;
				pResources->mh3->map = Road1_Hitmap_Map;
                loadRegionPal(Map_Palette);
                loadRegionMap(pResources->mh1, Road1_Tiles, ROAD1_TILES_NUM, 1);
                loadRegionMap(pResources->mh3, Road1_Hitmap_Tiles, ROAD1_HITMAP_TILES_NUM, 3);
				break;
		}
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

void updateTimers() {
    my_timers[0] = REG_TM0D / (65536 / 1000);
    my_timers[1] = REG_TM1D;
    my_timers[2] = REG_TM2D / (65536 / 1000);
    my_timers[3] = REG_TM3D;
}

unsigned short getTile(MapHandler* mh, int x, int y) {
	x = (x + (x % 8)) / 8;
	y = (y + (y % 8)) / 8;
	return mh->map[x + y*mh->width];
}

TileQuad* getTileQuad(MapHandler* mh, int x, int y) {
	TileQuad* tq = (TileQuad*) malloc(sizeof(TileQuad));
	bool isTop, isLeft;
	int tmpX, tmpY;
	isTop = FALSE;
	isLeft = FALSE;
    tmpX = (x + (x % 8)) / 8;
	tmpY = (y + (y % 8)) / 8;
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
	sh->dude.boundingBox->x = 0;
	sh->dude.boundingBox->y = 16;
	sh->dude.boundingBox->xSize = 16;
	sh->dude.boundingBox->ySize = 16;

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
                if(determineMove(tile1, 0, 0) && determineMove(tile3, 0, 0)) {
		            dude->worldX++;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			} else { /* New X is not on a tile boundary (top, mid, bot) */
                tile1 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize + 1, dude->worldY + dude->boundingBox->y);
				tile2 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize + 1, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize/2);
				tile3 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize + 1, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize);
                if(determineMove(tile1, 0, 0) && determineMove(tile2, 0, 0) && determineMove(tile3, 0, 0)) {
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
                if(determineMove(tile1, 0, 0) && determineMove(tile3, 0, 0)) {
		            dude->worldX--;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			} else { /* X is not on a tile boundary (top, mid, bot) */
                tile1 = getTile(pResources->mh3, dude->worldX - 1, dude->worldY + dude->boundingBox->y);
				tile2 = getTile(pResources->mh3, dude->worldX - 1, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize/2);
				tile3 = getTile(pResources->mh3, dude->worldX - 1, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize);
                if(determineMove(tile1, 0, 0) && determineMove(tile2, 0, 0) && determineMove(tile3, 0, 0)) {
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
                if(determineMove(tile1, 0, 0) && determineMove(tile3, 0, 0)) {
		            dude->worldY--;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			} else { /* Y is not on a tile boundary (left, mid, right) */
                tile1 = getTile(pResources->mh3, dude->worldX, dude->worldY + dude->boundingBox->y - 1);
				tile2 = getTile(pResources->mh3, dude->worldX + (dude->boundingBox->xSize/2), dude->worldY + dude->boundingBox->y - 1);
				tile3 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize, dude->worldY + dude->boundingBox->y - 1);
                if(determineMove(tile1, 0, 0) && determineMove(tile2, 0, 0) && determineMove(tile3, 0, 0)) {
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
                if(determineMove(tile1, 0, 0) && determineMove(tile3, 0, 0)) {
		            dude->worldY++;
					return TRUE;
				} else { /* Otherwise, can't move */
					return FALSE;
				}
			} else { /* Y is not on a tile boundary (left, mid, right) */
                tile1 = getTile(pResources->mh3, dude->worldX, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize + 1);
				tile2 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize/2, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize + 1);
				tile3 = getTile(pResources->mh3, dude->worldX + dude->boundingBox->xSize, dude->worldY + dude->boundingBox->y + dude->boundingBox->ySize + 1);
                if(determineMove(tile1, 0, 0) && determineMove(tile2, 0, 0) && determineMove(tile3, 0, 0)) {
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

bool determineMove(unsigned short tile, int x, int y) {
	if(tile != 0 && tile != 1 && tile != 11) {
		return FALSE;
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
