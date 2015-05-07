/* Define the timer constants */
#define TIMER_FREQUENCY_SYSTEM 0x0
#define TIMER_FREQUENCY_64 0x1
#define TIMER_FREQUENCY_256 0x2
#define TIMER_FREQUENCY_1024 0x3
#define TIMER_OVERFLOW 0x4
#define TIMER_ENABLE 0x80
#define TIMER_IRQ_ENABLE 0x40

/* Define the timer status addresses */
#define REG_TM0CNT     *(volatile u16*)0x4000102
#define REG_TM1CNT     *(volatile u16*)0x4000106
#define REG_TM2CNT     *(volatile u16*)0x400010A
#define REG_TM3CNT     *(volatile u16*)0x400010E

/* Define the timer data addresses */
#define REG_TM0D       *(volatile u16*)0x4000100
#define REG_TM1D       *(volatile u16*)0x4000104
#define REG_TM2D       *(volatile u16*)0x4000108
#define REG_TM3D       *(volatile u16*)0x400010C

int my_timers[4];
int last_timers[4];

void updateTimers();
