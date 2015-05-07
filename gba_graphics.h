#define REG_DISPCNT *(unsigned long*) 0x4000000
#define REG_DISPSTAT *(volatile unsigned short*) 0x400004
#define REG_VCOUNT *(volatile unsigned short*) 0x04000006
#define MODE_3 0x3
#define MODE_4 0x4
#define BG0_ENABLE 0x100
#define BG1_ENABLE 0x200
#define BG2_ENABLE 0x400
#define BG3_ENABLE 0x800
#define BACKBUFFER 0x10
#define SetMode(mode) REG_DISPCNT = (mode)
#define RGB(r, g, b) (unsigned short) ((r)+((g) << 5)+((b) << 10))

unsigned short* videoBuffer = (unsigned short*) 0x6000000;
unsigned short* frontBuffer = (unsigned short*) 0x6000000;
unsigned short* backBuffer = (unsigned short*) 0x600A000;
unsigned short* paletteMem = (unsigned short*) 0x5000000;
volatile unsigned short* scanlineCounter = (volatile unsigned short*) 0x4000006;

void waitVBlank() {
	while(REG_VCOUNT >= 160);
	while(REG_VCOUNT < 160);
};

void waitVBlank2() {
	while(*scanlineCounter < 160);
};

void flipPage() {
	if(REG_DISPCNT & BACKBUFFER) {
		REG_DISPCNT &= ~BACKBUFFER;
		videoBuffer = backBuffer;
	} else {
		REG_DISPCNT |= BACKBUFFER;
		videoBuffer = frontBuffer;
	}
};

void drawPixel3(int x, int y, unsigned short c) {
	videoBuffer[y*240 + x] = c;
};

void drawPixel4(int x, int y, unsigned char c) {
	unsigned short pixel;
	unsigned short offset = (y*240 + x) >> 1;
	pixel = videoBuffer[offset];
	if(x & 1) {
		videoBuffer[offset] = (c << 8) + (pixel & 0x00FF);
	} else {
		videoBuffer[offset] = (pixel & 0xFF00) + c;
	}
};

unsigned short getPixelColor3(int x, int y) {
	return videoBuffer[y*240 + x];
};

void drawLine3(int x1, int y1, int x2, int y2, unsigned short color) {
	int i, deltax, deltay, numPixels;
	int d, dinc1, dinc2;
	int x, xinc1, xinc2;
	int y, yinc1, yinc2;
	//calculate deltaX and deltaY
	deltax = abs(x2 - x1);
	deltay = abs(y2 - y1);
	//initialize
	if(deltax >= deltay) {
		//If x is independent variable
		numPixels = deltax + 1;
		d = (2 * deltay) - deltax;
		dinc1 = deltay << 1;
		dinc2 = (deltay - deltax) << 1;
		xinc1 = 1;
		xinc2 = 1;
		yinc1 = 0;
		yinc2 = 1;
	} else {
		//if y is independant variable
		numPixels = deltay + 1;
		d = (2 * deltax) - deltay;
		dinc1 = deltax << 1;
		dinc2 = (deltax - deltay) << 1;
		xinc1 = 0;
		xinc2 = 1;
		yinc1 = 1;
		yinc2 = 1;
	}
	//move the right direction
	if(x1 > x2) {
		xinc1 = -xinc1;
		xinc2 = -xinc2;
	}
	if(y1 > y2) {
		yinc1 = -yinc1;
		yinc2 = -yinc2;
	}
	x = x1;
	y = y1;
	//draw the pixels
	for(i = 1; i < numPixels; i++) {
		drawPixel3(x, y, color);
		if(d < 0) {
		    d = d + dinc1;
		    x = x + xinc1;
		    y = y + yinc1;
		} else {
		    d = d + dinc2;
		    x = x + xinc2;
		    y = y + yinc2;
		}
	}
};

void drawBox3(int left, int top, int right, int bottom, unsigned short color) {
	int x, y;
	for(y = top; y <= bottom; y++) {
		for(x = left; x <= right; x++) {
			drawPixel3(x, y, color);
		}
	}
};

void drawSquare3(int left, int top, int right, int bottom, unsigned short color) {
	int x, y;
	/* Draw top line */
	for(x = left; x <= right; x++) {
		drawPixel3(x, top, color);
	}
	/* Draw right line */
	for(y = top; y <= bottom; y++) {
		drawPixel3(right, y, color);
	}
	/* Draw bottom line */
	for(x = left; x <= right; x++) {
		drawPixel3(x, bottom, color);
	}
	/* Draw left line */
	for(y = top; y <= bottom; y++) {
		drawPixel3(left, y, color);
	}
};

bool drawCenteredPic3(unsigned short image[], int w, int h) {
	int xDiff, yDiff, i, j;
	/* Find margins (xDiff = L and R margins, yDiff = Top and bottom margins */
	xDiff = (240 - w) / 2;
	yDiff = (160 - h) / 2;
	/* If picture is too big, return false */
	if(w > 240 || h > 160) {
		return FALSE;
	} else { /* Otherwise, display image */
		/* Set VRAM index to the top-right location w/ margins */
        i = 240*yDiff + xDiff;
		/* Loop over image array */
		for(j = 0; j < 10200; j++) {
			videoBuffer[i++] = image[j];
			/* If the end of the image line is reached,
			move to beginning of next image line */
			if(j % w == 0) {
				yDiff++;
				i = 240*yDiff + xDiff;
			}
		}
		return TRUE;
	}
};

bool drawPic3(unsigned short image[], int w, int h, int x, int y) {
    int i, xCtr, yCtr;
	if(w > 240 || h > 160 || y + h > 160 || x + w > 240) {
		return FALSE;
	} else {
		i = 0;
		yCtr = y;
		for(yCtr = y; yCtr < y + h; yCtr++) {
			for(xCtr = x; xCtr < w + x; xCtr++) {
				drawPixel3(xCtr, yCtr, image[i++]);
			}
		}
		return TRUE;
	}
};

bool drawPic4(const unsigned char image[], int w, int h, int x, int y) {
	int i, xCtr, yCtr;
	i = 0;
	for(yCtr = y; yCtr < y + h; yCtr++) {
		for(xCtr = x; xCtr < x + w; xCtr++) {
			drawPixel4(xCtr, yCtr, image[i++]);
		}
	}
	return TRUE;
};

void drawCircle(int xCenter, int yCenter, int radius, unsigned short color) {
	int x = 0;
	int y = radius;
	int p = 3 - 2 * radius;
	while (x <= y) {
		drawPixel3(xCenter + x, yCenter + y, color);
		drawPixel3(xCenter - x, yCenter + y, color);
		drawPixel3(xCenter + x, yCenter - y, color);
		drawPixel3(xCenter - x, yCenter - y, color);
		drawPixel3(xCenter + y, yCenter + x, color);
		drawPixel3(xCenter - y, yCenter + x, color);
		drawPixel3(xCenter + y, yCenter - x, color);
		drawPixel3(xCenter - y, yCenter - x, color);
		if (p < 0) {
			p += 4 * x++ + 6;
		} else {
			p += 4 * (x++ - y--) + 10;
		}
	}
};
