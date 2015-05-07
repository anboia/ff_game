/* Scrolling BG defines */
#define REG_BG0HOFS *(volatile unsigned short*) 0x4000010
#define REG_BG0VOFS *(volatile unsigned short*) 0x4000012
#define REG_BG1HOFS *(volatile unsigned short*) 0x4000014
#define REG_BG1VOFS *(volatile unsigned short*) 0x4000016
#define REG_BG2HOFS *(volatile unsigned short*) 0x4000018
#define REG_BG2VOFS *(volatile unsigned short*) 0x400001A
#define REG_BG3HOFS *(volatile unsigned short*) 0x400001C
#define REG_BG3VOFS *(volatile unsigned short*) 0x400001E

/* BG Setup defines */
#define REG_BG0CNT *(volatile unsigned short*) 0x4000008
#define REG_BG1CNT *(volatile unsigned short*) 0x400000A
#define REG_BG2CNT *(volatile unsigned short*) 0x400000C
#define REG_BG3CNT *(volatile unsigned short*) 0x400000E
#define BG_COLOR256 0x80
#define CHAR_SHIFT 2
#define SCREEN_SHIFT 8
#define WRAPAROUND 0x1

/* Misc BG defines */
#define BGPaletteMem ((unsigned short*) 0x5000000)
#define TEXTBG_SIZE_256x256 0x0
#define TEXTBG_SIZE_256x512 0x8000
#define TEXTBG_SIZE_512x256 0x4000
#define TEXTBG_SIZE_512x512 0xC000
#define charBaseBlock(n) (((n)*0x4000)+0x6000000)
#define screenBaseBlock(n) (((n)*0x800)+0x6000000)

/* DMA Fast Copy defines */
#define REG_DMA3SAD *(volatile unsigned int*) 0x40000D4
#define REG_DMA3DAD *(volatile unsigned int*) 0x40000D8
#define REG_DMA3CNT *(volatile unsigned int*) 0x40000DC
#define DMA_ENABLE 0x80000000
#define DMA_TIMING_IMMEDIATE 0x00000000
#define DMA_16 0x00000000
#define DMA_32 0x04000000
#define DMA_32NOW (DMA_ENABLE | DMA_TIMING_IMMEDIATE | DMA_32)
#define DMA_16NOW (DMA_ENABLE | DMA_TIMING_IMMEDIATE | DMA_16)

void DMAFastCopy(void* src, void* dest, unsigned int count, unsigned int mode) {
	if(mode == DMA_16NOW || mode == DMA_32NOW) {
		REG_DMA3SAD = (unsigned int) src;
		REG_DMA3DAD = (unsigned int) dest;
		REG_DMA3CNT = count | mode;
	}
}


void copyRow(unsigned short* bg, const unsigned short* map, int bgRow, int bgCol, int mRow, int mCol, int width) {
    int i;
	for(i = 0; i < 32; i++) {
		bg[bgRow*32 + (bgCol+i)%32] = map[mRow*width + (mCol+i)%width];
	}
};

void copyCol(unsigned short* bg, const unsigned short* map, int bgRow, int bgCol, int mRow, int mCol, int width, int height) {
	int i;
	for(i = 0; i < 32; i++) {
		bg[((bgRow+i)%32)*32 + bgCol] = map[((mRow+i)%height)*width + mCol];
	}
};

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
};

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
};
