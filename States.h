unsigned short gameState;

/* Conditions */
#define RAINING 0x0100
#define NIGHT 0x1000

/* Regions */
#define HOME 10
	#define HOME_OUTSIDE 0
	#define HOME_HOUSE 1
	#define HOME_SHED 2
#define OTHER_1 20
	#define OTHER_1_OUTSIDE 0
	#define OTHER_1_HOUSE 1
#define OTHER_2 30
	#define OTHER_2_OUTSIDE 0
	#define OTHER_2_HOUSE 1
#define OTHER_3 40
	#define OTHER_3_OUTSIDE 0
	#define OTHER_3_HOUSE 1
#define TOWN 50
	#define TOWN_OUTSIDE 0
	#define TOWN_STORE 1
	#define TOWN_FT 2
#define ROAD1 60
