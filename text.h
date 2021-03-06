#include "stdarg.h"
#include "string.h"
#include "gba_memory.h"

// ========================================================
// ===== DEFINES ==========================================

#define DIALOG_WIDTH 28

// Tiles Offset
#define TILE_ASCI_TRAN 0
#define TILE_ASCI_OPAC 95
#define TILE_BORDER_1 190
#define TILE_BORDER_2 199

// Borders offset
#define BORDER_TOP_LEFT			0
#define BORDER_TOP					1
#define BORDER_TOP_RIGHT		2
#define BORDER_LEFT 				3
#define BORDER_MIDDLE				4
#define BORDER_RIGHT 				5
#define BORDER_BOTTOM_LEFT 	6
#define BORDER_BOTTOM 			7
#define BORDER_BOTTOM_RIGHT	8

#define PALETTE_START	253 // PALLETE COLLOR START
#define PALETTE_SIZE	3
#define COLLOR_BORDER	253
#define COLLOR_TEXT		254
#define COLLOR_BACK		255



#define TEXT_SBB 7
#define TEXT_CBB 0

/*
	EXAMPLE USAGE
	print dialog with two story. press KEY_A to continue
	print_story(2, "- Hi! how are you?\n\n- I am fine and you?\n\n- Well, today I'm having a lot of fun doing the final project! game programing rocks!! :D", "- Well, today I'm having a lot of fun doing the final project! game programing rocks!! :D");

	// draw a box
	draw_box(18, TILE_ASCI_OPAC);

	print(2,1,"  asdfasdf l", TILE_ASCI_OPAC);
*/



// ========================================================
// ===== GLOBALS ==========================================



// ========================================================
// ===== PROTOTYPES =======================================
// ========================================================
// ===== FUNCTIONS ========================================
void print(int x, int y, const char *str, int type)
{
	int c;
	SCR_ENTRY *dst = &se_mem[TEXT_SBB][y*32+x];

	x=0;
	while((c=*str++) != 0)
	{
		if(c == '\n')
		{
			while(x<26)dst[x++] = type;
			dst += (2&~31) + 32;    x=0;    }
		else
			dst[x++] = c-32+type;
	}
};

void set_tile(int x, int y, int id){
	se_mem[TEXT_SBB][y*32+x] = id;
};

void draw_box(int h, int type){
	int top = 20-h-2, left = 1, bottom = 19, right = 28, i;
	for(i = top; i<bottom; i++)
	{
		print(2, i, "                          ", type);
	}

	set_tile(left, top, 		TILE_BORDER_1+BORDER_TOP_LEFT);
	set_tile(right, top, 		TILE_BORDER_1+BORDER_TOP_RIGHT);
	set_tile(right, bottom, TILE_BORDER_1+BORDER_BOTTOM_RIGHT);
	set_tile(left, bottom, 	TILE_BORDER_1+BORDER_BOTTOM_LEFT);
	for (i = left+1; i < right; ++i)
	{
		set_tile(i,top,			TILE_BORDER_1+BORDER_TOP);
		set_tile(i,bottom,	TILE_BORDER_1+BORDER_BOTTOM);
	}
	for (i = top+1; i < bottom; ++i)
	{
		set_tile(left,i,	TILE_BORDER_1+BORDER_LEFT);
		set_tile(right,i,	TILE_BORDER_1+BORDER_RIGHT);
	}
};

void print_box(const char *str){
	int n = strlen(str);
	int i = 0, j = 0, p = 0, line = 0;
	int w = DIALOG_WIDTH - 2;
	char buff[1000];
	memset(buff,0,sizeof buff);

	// insert newlines if the the line is larger than the width
	while(i < n){
		int k = min(i+w,n);
		for ( j = i; j < k; ++j)
			if(str[j]=='\n')
				k = j + 1;
		while(k>i && str[k]!=' ' && str[k]!='\0' && str[k]!='\n')
			k--;
		if(k==i)
			k = j-i;
		else
			k -=i;
		for (j = 0; j < k; ++j, p++)
			buff[p] = str[i+j];
		if(buff[p]!='\n')
			buff[p++] = '\n';
		i += k + 1;
	}
	for ( i = 0; buff[i]; ++i)
		if (buff[i] == '\n')
			line++;

	// print string
	draw_box(line, TILE_ASCI_OPAC);
	print( 2, 19 - line, buff, TILE_ASCI_OPAC);
};

void reset_text(){
	memset(&se_mem[TEXT_SBB][0], TILE_ASCI_TRAN+' '-32, 2048);
};

void print_story(int num, ... )
{
	va_list arguments;
	va_start ( arguments, num );

	reset_text();
	while(num--) {
		print_box(va_arg ( arguments, const char * ) );
		wait_key(BUTTON_A);
		reset_text();
	}
	va_end ( arguments );
};

void init_palette(){
	// memcpy(paletteMem, fontPal, fontPalLen);
	int i = 0;
	for (i = PALETTE_START; i < PALETTE_START+PALETTE_SIZE; ++i)
	{
		paletteMem[i] = fontPal[i];
	}
};

void init_text(){
	REG_BG0CNT = BG_CBB(TEXT_CBB) | BG_SBB(TEXT_SBB) | BG_COLOR256 | TEXTBG_SIZE_256x256 ;

	// Load palette
	init_palette();

	// Load tiles into CBB 0
	memcpy(&tile_mem[TEXT_CBB][0], fontTiles, fontTilesLen);

};

/*void debug(int x, int y, int k){
	char buff[10];
	sprintf(buff, "result: %d", k);
	print(x,y, buff, TILE_ASCI_TRAN);
};*/
