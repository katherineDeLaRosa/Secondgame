#include <stdlib.h>
#include <string.h>

#include <stdlib.h>
#include <string.h>

// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>

// link the pattern table into CHR ROM
//#link "chr_generic.s"

// BCD arithmetic support
#include "bcd.h"
//#link "bcd.c"

// VRAM update buffer
#include "vrambuf.h"
//#link "vrambuf.c"

//the linking changes the colors. but you have to have them link to work
//#link "famitone2.s"
void __fastcall__ famitone_update(void);
//link "music_after_the rain.s"
extern char after_the_rain_music_data[];
//#link "music_dangerstreets.s"
extern char danger_streets_music_data[];
//#link "demosounds.s"
extern char demo_sounds[];
typedef enum { SND_START, SND_HIT, SND_COIN, SND_JUMP } SFXIndex;


#define COLS 32
#define ROWS 28

// define controller sprite
#define DEF_METASPRITE_2x2(name,code,pal)\
const unsigned char name[]={\
        0,      0,      (code)+0,   pal, \
        0,      8,      (code)+1,   pal, \
        8,      0,      (code)+2,   pal, \
        8,      8,      (code)+3,   pal, \
        128};

// define the controller sprite flipped horizontally
#define DEF_METASPRITE_2x2_FLIP(name,code,pal)\
const unsigned char name[]={\
        8,      0,      (code)+0,   (pal)|OAM_FLIP_H, \
        8,      8,      (code)+1,   (pal)|OAM_FLIP_H, \
        0,      0,      (code)+2,   (pal)|OAM_FLIP_H, \
        0,      8,      (code)+3,   (pal)|OAM_FLIP_H, \
        128};
  
// create the cpu controlled sprite
const unsigned char metasprite[]={
        0,      0,      (0xba)+0,   2, 
        0,      8,      (0xba)+2,   2, 
        8,      0,      (0xba)+1,   2, 
        8,      8,      (0xba)+3,   2, 
        128};



        
DEF_METASPRITE_2x2(playerRStand, 0xd8, 0);
DEF_METASPRITE_2x2(playerRRun1, 0xdc, 0);
DEF_METASPRITE_2x2(playerRRun2, 0xe0, 0);
DEF_METASPRITE_2x2(playerRRun3, 0xe4, 0);
DEF_METASPRITE_2x2(playerRJump, 0xe8, 0);
DEF_METASPRITE_2x2(playerRClimb, 0xec, 0);
DEF_METASPRITE_2x2(playerRSad, 0xf0, 0);

DEF_METASPRITE_2x2_FLIP(playerLStand, 0xd8, 0);
DEF_METASPRITE_2x2_FLIP(playerLRun1, 0xdc, 0);
DEF_METASPRITE_2x2_FLIP(playerLRun2, 0xe0, 0);
DEF_METASPRITE_2x2_FLIP(playerLRun3, 0xe4, 0);
DEF_METASPRITE_2x2_FLIP(playerLJump, 0xe8, 0);
DEF_METASPRITE_2x2_FLIP(playerLClimb, 0xec, 0);
DEF_METASPRITE_2x2_FLIP(playerLSad, 0xf0, 0);

DEF_METASPRITE_2x2(personToSave, 0xba, 1);

const unsigned char* const playerRunSeq[16] = {
  playerLRun1, playerLRun2, playerLRun3, 
  playerLRun1, playerLRun2, playerLRun3, 
  playerLRun1, playerLRun2,
  playerRRun1, playerRRun2, playerRRun3, 
  playerRRun1, playerRRun2, playerRRun3, 
  playerRRun1, playerRRun2,
};
	// actor index





/*{pal:"nes",layout:"nes"}*/
char PALETTE[32] = { 
  0x03,	// screen color // 0x15 for ground color

  0x11,0x30,0x27,0x00,	// background palette 0
  0x1C,0x20,0x2C,0x00,	// background palette 1
  0x00,0x10,0x20,0x00,	// background palette 2
  0x16,0x35,0x26,0x00,   // background palette 3

  0x04,0x10,0x1A,0x00,	// sprite palette 0 haircolor,skin tone, shirt and shoes
  0x00,0x17,0x25,0x00,	// sprite palette 1
  0x0D,0x2D,0x3A,0x00,	// sprite palette 2
  0x0D,0x28,0x2A	// sprite palette 3
};

void cputcxy(byte x, byte y, char ch) {
  vrambuf_put(NTADR_A(x,y), &ch, 1);
}

void cputsxy(byte x, byte y, const char* str) {
  vrambuf_put(NTADR_A(x,y), str, strlen(str));
}

const char BOX_CHARS[8] = { '+','+','+','1',0xc9,0xc9,0xc9,0xc9 };

void draw_box(byte x, byte y, byte x2, byte y2, const char* chars) {
  byte x1 = x;
  cputcxy(x, y, chars[2]); // this sets up the top left corner
  cputcxy(x2, y, chars[3]); // this sets up the top right corner
  cputcxy(x, y2, chars[0]); // // this sets up the bottom left corner
  cputcxy(x2, y2, chars[1]); // this sets up the bottom right corner
  while (++x < x2) {
    cputcxy(x, y, chars[5]); // this the top row 
    cputcxy(x, y2, chars[4]); // this is the bottom row
  }
  while (++y < 36) {
    cputcxy(x1, y, chars[6]); // thsi is the left column
    cputcxy(x2, y, chars[7]); // this is the right colum
  }
}


//x boundary = 3,28, y boundary = 5,26
void draw_patches(const char* chars){
  int counter = 0;
  int randomx;
  int randomy;
  randomx = rand()%(28-3)+3;
  randomy = rand()%(26-5)+3;
  cputcxy(randomx+12, randomy+1, chars[5]);
  cputcxy(randomx+1,randomy, chars[5]);
  cputcxy(randomx-25,randomy - 12, chars[5]);

}

void draw_playfield() {
  int counter = 0;
  
  draw_box(1,1,COLS-2,ROWS-1,BOX_CHARS);
  while(counter < 5){
   draw_patches(BOX_CHARS);
    counter+=1;
  }

}

#define AE(tl,tr,bl,br) (((tl)<<0)|((tr)<<2)|((bl)<<4)|((br)<<6))

const unsigned char Attrib_Table[0x40]={
AE(3,3,1,0),AE(3,3,0,0),AE(3,3,0,0),AE(3,3,0,0), AE(2,2,0,0),AE(2,2,0,0),AE(2,2,0,0),AE(2,2,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,0,1,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0), AE(0,0,0,0),AE(0,0,0,0),AE(0,0,0,0),AE(0,1,0,1),
AE(1,1,1,1),AE(1,1,1,1),AE(1,1,1,1),AE(1,1,1,1), AE(1,1,1,1),AE(1,1,1,1),AE(1,1,1,1),AE(1,1,1,1),
};

const unsigned char Palette_Table[16]={ 
  0x07,
  0x09,0x09,0xa6,0x08,
  0x09,0x09,0xa6,0x08,
  0x09,0x09,0xa6,0xa6,
  0x09,0x09,0xa6
};

// put 8x8 grid of palette entries into the PPU
void setup_attrib_table() {
  vram_adr(0x2c0);
  vram_write(Attrib_Table, 0x40);
}

void setup_palette() {
  int i;
  // only set palette entries 0-15 (background only)
  for (i=0; i<16; i++)
    pal_col(i, Palette_Table[i]);
}
void end_screen(){
  ppu_off();
  oam_clear();
  vrambuf_clear();
  vram_adr(NTADR_A(5,14));
  vram_write("You tagged your friend!", 24);
  ppu_on_all();
  while(1){
  }
  ppu_off();
}
void win_screen (){
  vrambuf_clear();
    ppu_off();
  
  vram_adr(0x2000);
  vram_fill(0, 32*28);
  vram_adr(0x0);
  // write text to name table
  vram_adr(NTADR_A(5,14));		// set address
  vram_write("You tagged your friend", 23);
    vram_adr(NTADR_A(13,16));		// set address
  vram_write("You Win!", 8);// write bytes to video RAM

  // enable PPU rendering (turn on screen)
  ppu_on_all();

  // infinite loop
  while (1) ;
}
void doesplayerhitCPU(userx, usery, cpux, cpuy){
  if(abs(userx - cpux) < 8){
    if(abs(usery - cpuy) < 8){
      sfx_play(SND_HIT,0);
       win_screen ();
      
    }
  }

}

void clrscr() {
  vrambuf_clear();
  ppu_off();
  vram_adr(0x2000);
  vram_fill(0, 32*28);
  vram_adr(0x0);
  ppu_on_bg();
}



void play_round() {
  ppu_off();
  setup_attrib_table();
  setup_palette();
  clrscr();
  draw_playfield();
  while (1) {
  }
}




void play_game() {
  play_round();
}






// setup PPU and tables
void setup_graphics() {
  // clear sprites
  
  // clear sprites
  oam_hide_rest(0);
  // set palette colors
  pal_all(PALETTE);
} // setup_graphics
void setup(){
  PALETTE[0] = 0x7;
  pal_all(PALETTE);
}






void title_screen(){
  vram_adr(NTADR_A(8,5));
  vram_write("Lets play tag!!!", 16);
  
  vram_adr(NTADR_A(6,22));	
  vram_write("Press start to begin", 20);  
  ppu_on_all();
  while(1){
    if(pad_trigger(0)&PAD_START) break; 
  }
  ppu_off();
} // title screen

// define actors
#define NUM_ACTORS 2
#define NUM_ACTORS2 2
// actor x/y positions
byte actor_x[NUM_ACTORS];
byte actor_y[NUM_ACTORS];
// actor x/y deltas per frame (signed)
sbyte actor_dx[NUM_ACTORS];
sbyte actor_dy[NUM_ACTORS];


// define the cpu controlled sprite
byte actor2_x[NUM_ACTORS2];
byte actor2_y[NUM_ACTORS2];
// actor x/y deltas per frame (signed)
sbyte actor2_dx[NUM_ACTORS2];
sbyte actor2_dy[NUM_ACTORS2];









void game(){
  char i;
  char oam_id;	// sprite ID
  char pad;
  ppu_off();
  vram_adr(NTADR_A(8,5));
  vram_write("                ", 16);
  
  vram_adr(NTADR_A(6,22));	
  vram_write("                    ", 20);  	
  
  // print instructions
  vram_adr(NTADR_A(6,3));
  vram_write("\x1c\x1d\x1e\x1f tag your friend", 20);
  

  // setup graphics
  setup(); 
	
  // initialize actors with random values
  for(i=0;i<NUM_ACTORS;i++) {
    actor_x[i] = i*32+128; // current position in the screen on the x-axis 
    actor_y[i] = i*8+64;   // current position in the screen on the y-axis
    actor_dx[i] = 0;
    actor_dy[i] = 0;
  }  
  // initialize the cpu controlled sprite with random values 
  // so it can spaw at a random location
  for(i=0;i<NUM_ACTORS2;i++) {
    actor2_x[i] = rand();
    actor2_y[i] = rand();
    actor2_dx[i] = (rand() & 7) - 3;
    actor2_dy[i] = (rand() & 7) - 3;
  }
  ppu_on_all();
  setup_palette();
  draw_playfield();
  while(1){
    
    // this moves the player controlled sprite
    oam_id =0;
    for(i=0; i <2;i++){
    	pad = pad_poll(i);
        doesplayerhitCPU( actor_x[i],actor_y[i],actor2_x[i],actor2_y[i]);
      if (pad&PAD_LEFT && actor_x[i]>15) {
      actor_dx[i]=-2;     
      }
      else if (pad&PAD_RIGHT && actor_x[i]<228) actor_dx[i]=2;
      else actor_dx[i]=0;
      // move actor[i] up/down
      if (pad&PAD_UP && actor_y[i]>12) actor_dy[i]=-2;
      else if (pad&PAD_DOWN && actor_y[i]<198) actor_dy[i]=2;
      else actor_dy[i]=0;
    }
    // this is for the player controlled sprite
    for (i=0; i<NUM_ACTORS-1; i++) { // get the total number of actors and sub one less than
      				     // total. this will show one less characters
      byte runseq = actor_x[i] & 7;
      if (actor_dx[i] >= 0)
        runseq += 8;
      oam_id = oam_meta_spr(actor_x[i], actor_y[i], oam_id, playerRunSeq[runseq]);   
      actor_x[i] += actor_dx[i];
      actor_y[i] += actor_dy[i];

    }
    // create a for loop for the cpu controlled sprite
    for (i=0; i<NUM_ACTORS2-1; i++) { // get the total number of actors and sub one less than
      				     // total. this will show one less characters
      oam_id = oam_meta_spr(actor2_x[i], actor2_y[i], oam_id, metasprite);
      actor2_x[i] += actor2_dx[i];     
      actor2_y[i] += actor2_dy[i];
      if(actor2_x[i] >= 228){actor2_dx[i]=-2;} // if we go too far right go left
      if(actor2_x[i] == 10 ){actor2_dx[i]=6;} // if we go too far left go right

      if(actor2_y[i] >= 200){actor2_dy[i]=-5;} // if we go too far down go up
      if(actor2_y[i] == 10 ){actor2_dy[i]=2;}
    }    
    // hide rest of sprites
    // if we haven't wrapped oam_id around to 0
    if (oam_id!=0) oam_hide_rest(oam_id);
    // wait for next frame
    ppu_wait_frame();   
    
  }
}






void setup_sounds() {
  famitone_init(danger_streets_music_data);
  sfx_init(demo_sounds);
  nmi_set_callback(famitone_update);
   sfx_play(0,1);

}







void main(void)
{  
  setup_graphics();
  setup_sounds();
  music_play(20);
  
  // draw message  
  title_screen();
  vrambuf_clear();
  set_vram_update(updbuf);
  // infinite loop
  while(1) {    
    game();
  }
}