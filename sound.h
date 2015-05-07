
#include "sound_digging.h"
#include "sound_rooster.h" 
#include "sound_openning.h"
#include "sound_spurs.h"
#include "sound_select.h"
#include "sound_story.h"




#include <stdlib.h> 
#include <string.h>


//define some interrupt registers 
#define REG_IME        *(u16*)0x4000208 
#define REG_IE         *(u16*)0x4000200 
#define REG_IF         *(u16*)0x4000202 
#define REG_INTERRUPT  *(u32*)0x3007FFC 
#define REG_DISPSTAT   *(u16*)0x4000004 
#define INT_VBLANK     0x0001 



//define some timer and DMA registers/values 
#define REG_TM0D       *(volatile u16*)0x4000100 
#define REG_TM0CNT     *(volatile u16*)0x4000102 
#define REG_DMA1SAD    *(volatile u32*)0x40000BC 
#define REG_DMA1DAD    *(volatile u32*)0x40000C0 
#define REG_DMA1CNT_H  *(volatile u16*)0x40000C6 
#define TIMER_ENABLE    0x80 
//#define DMA_DEST_FIXED  64 
//#define DMA_REPEAT      512 
//#define DMA_32          1024 
//#define DMA_ENABLE      32768 
//#define DMA_TIMING_SYNC_TO_DISPLAY  4096 | 8192

//define some sound hardware registers/values 
#define REG_SGCNT0_H *(volatile u16*)0x4000082 
#define REG_SGCNT1 *(volatile u16*)0x4000084 
#define DSOUND_A_RIGHT_CHANNEL  256 
#define DSOUND_A_LEFT_CHANNEL   512 
#define DSOUND_A_FIFO_RESET     2048 
#define SOUND_MASTER_ENABLE     128



#define MULTIBOOT int __gba_multiboot; 
MULTIBOOT


#define REG_VCOUNT *(volatile unsigned short*) 0x04000006

void WaitVBlack(){
	while(REG_VCOUNT >= 160);
	while(REG_VCOUNT < 160);
}


//create a struct to keep track of sound data 
typedef struct tagSound {
    void *pBuffer;
    u16 samplerate;
    u32 length; 
}sound;
//create variables that describe the sounds 



sound s_dig = {&digging, 20000, 9232}; 
sound s_rooster = {&rooster, 8000, 15984};
sound s_openning = {&openning, 8000, 19216}; 
sound s_spurs = {&spurs, 8000, 12216};


//global variables 
u16 SamplePosition = 0; 
u16 SampleLength = 0; 
char temp[20];

int track = 0;


//PlaySound(&s_sound)
void PlaySound(sound *theSound) {
    u16 samples;        
	//output to both channels and reset the FIFO    
	REG_SGCNT0_H = DSOUND_A_RIGHT_CHANNEL | DSOUND_A_LEFT_CHANNEL | DSOUND_A_FIFO_RESET;
    //enable all sound    
	REG_SGCNT1 = SOUND_MASTER_ENABLE;
    //DMA1 source address    
	REG_DMA1SAD = (u32)theSound->pBuffer;
    //DMA1 destination address    
	REG_DMA1DAD = 0x40000A0;
    //write 32 bits into destination every vblank    
	REG_DMA1CNT_H =  64 | 512 | 1024 | 4096 | 8192 | 32768;
    //set the sample rate    
	samples = 16777216 / theSound->samplerate; //2097    
	REG_TM0D = 65536 - samples;
	
    //keep track of the playback position and length    
	SampleLength = theSound->length / samples * 15.57;    
	SamplePosition = 0;
    //enable the timer    
	REG_TM0CNT = TIMER_ENABLE;
}

void MyHandler(void) 
{    
	u16 Int_Flag;
    //disable interrupts    
	REG_IME = 0x00;
    //backup the interrupt flags    
	Int_Flag = REG_IF;
    //look for vertical refresh    
	if((REG_IF & INT_VBLANK) == INT_VBLANK)
    {   
		//is a sample currently playing?        
		if (SampleLength)
        {
            //increment the position, check if complete            
			SamplePosition++;            
			if (SamplePosition > SampleLength)            
			{                
				//stop playback: disable the timer and DMA                
				REG_TM0CNT = 0;                
				REG_DMA1CNT_H = 0;                
				//reset length                
				SampleLength = 0;
				//track ++;
			}        
		}
	}
    //restore the interrupt flags    
	REG_IF = Int_Flag;
    //enable interrupts    
	REG_IME = 0x01; 
}

