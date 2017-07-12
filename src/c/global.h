#pragma once
#include "pebble.h"

#define IF_COLOR(x)     COLOR_FALLBACK(x, (void)0)
#define IF_BW(x)        COLOR_FALLBACK((void)0, x)
#define IF_COLORBW(x,y) COLOR_FALLBACK(x, y)
#define IF_BWCOLOR(x,y) COLOR_FALLBACK(y, x)

#define null NULL  // CloudPebble highlights lowercase version which looks nicer, adding this macro to make it useable

//#define MAP_SIZE 21             // Map is MAP_SIZE * MAP_SIZE squares big
#define MAP_SIZE 10             // Map is MAP_SIZE * MAP_SIZE squares big
#define MAX_TEXTURES 15        // Most number of textures there's likely to be.  Feel free to increase liberally, but no more than 254.
#define MAX_SQUARETYPES 10     // Most number of different map square types (square type 0 is always out-of-bounds plus wherever else on the map uses square type 0)
#define MAX_OBJECTS 10          // Maximum number of objects which can have sprites
//options
#define IDCLIP false           // Walk thru walls
//#define view_border false       // Draw border around 3D viewing window
//#define app_logging true      // Whether program should log errors and info to the phone or not
  
#define Format1Bit 0  // Note: On Color, all 1bit images need to be converted to GBitmapFormat1BitPalette
#define Format2Bit 1
#define Format4Bit 2
#define Format8Bit 3  // Note: 64-color textures not currently supported (well, no >16 color textures)

typedef struct SquareTypeStruct {
  uint8_t face[4]; // texture[] number
  uint8_t ceiling; // texture[] number (255 = no ceiling / sky.  Well, 255 or any number >MAX_TEXTURES)
  uint8_t floor;   // texture[] number (255 = no floor texture)
  // other characteristics like walk thru and stuff
} SquareTypeStruct;

typedef struct PlayerStruct {
  int32_t x;                  // Player's X Position (64 pixels per square)
  int32_t y;                  // Player's Y Position (64 pixels per square)
  int16_t facing;             // Player Direction Facing (from 0 - TRIG_MAX_ANGLE)
} PlayerStruct;

typedef struct ObjectStruct {
  int32_t x;                  // Object's X Position (64 pixels per square)
  int32_t y;                  // Object's Y Position (64 pixels per square)
  int16_t facing;             // currently unsupported, but maybe shows different sprites depending on orientation to player

  uint8_t alignment;          // On the ceiling, middle, or floor
  int16_t offset;             // Offset from alignment
    //int32_t objectverticaloffset = 64 - objectheight; // on the ground
    //int32_t objectverticaloffset = 0;               // floating in the middle
    //int32_t objectverticaloffset = -64 + objectheight;   // on the ceiling
  
  int32_t dist;  // Distance in units between object and player
  int16_t angle; // angle between "center column of player's view" and the object
     //angle<0=left of center, 0=center column, >0=right of center
     //if cos_lookup(angle)>0, object is in front of player
     //if cos_lookup(angle)=0 then object is straight right or left of the player
     //if cos_lookup(angle)<0, object is in behind player

  //int16_t health;             //
  uint8_t type;               // Type 0 = Doesn't Exist.  1=Player/Enemy (8 sided).  2=Bullet.  Others: Lamp, Coin, HealthPack, etc
  uint8_t sprite;             // sprite[] for object
  //uint8_t mask;               // if B&W, this points to sprite[] to be used as a mask
  //int32_t data1;              // 
  //int32_t data2;              // 
} ObjectStruct;

typedef struct RayStruct {
   int32_t x;                 // x coordinate the ray hit
   int32_t y;                 // y coordinate the ray hit
  uint32_t dist;              // length of the ray, i.e. distance ray traveled
   uint8_t hit;               // square_type the ray hit [0-127]
   int32_t offset;            // horizontal spot on texture the ray hit [0-63] (used in memory pointers so int32_t)
   uint8_t face;              // face of the block it hit (00=East Wall, 01=North, 10=West, 11=South Wall)
} RayStruct;


typedef union TextureStruct {
  struct {
    uint8_t bits_per_pixel;  // number to shift (e.g. 0=1<<0=1, 1=1<<1=2, 2=1<<2=4) for how many bits represent a pixel (Occupies the same byte as format, hence the union)
  };
  struct {
    uint8_t format;          // (color only)            texture type    (0=Format1Bit/px, 1=Format2Bit/px, 2=Format4Bit/px, 3=Format8Bit/pixel, 4=invalid or blank)
    uint8_t *palette;        // (color only) Pointer to texture palette
    GBitmap *bmp;            //              Pointer to texture
    uint8_t *data;           //              Pointer to texture data
    //GBitmap *bmp2;       // (b&w only)   Pointer to 2nd texture (for transparent PNGs that make _BLACK and _WHITE)
    uint8_t width;           // texture width  in pixels (usually 64px for wall textures)
    uint8_t height;          // texture height in pixels (usually 64px for wall textures)
    uint8_t bytes_per_row;   // Bit shifted.  Currently assumes 64px wide image (TODO: Allow other widths)
    uint8_t pixels_per_byte; // Bit shifted. So instead of [8px/B, 4px/B, 2px/B] it stores [3,2,1] (8=1<<3, 4=1<<2, 2=1<<1)
    uint8_t colormax;        // largest color in palette.  it is (1<<XbitPalette)-1 (e.g. =15 in 4bit|16color palette, =3 in 2bit|4color palette.)
  };
// Format     (Bytes/Row)  (Bytes/Row)/2   px/byte   px/byte-1(mask)   bit/px  number_of_colors-1    Bytes/Row  px/byte px/byte-1   bit/px  number_of_colors-1
//0 = 1bit      <<3= *8        4-1= 3      >>3 = /8       7            <<0=*1       2-1= 1               3         3       7        0            1
//1 = 2bit      <<4=*16        8-1= 7      >>2 = /4       3            <<1=*2       4-1= 3               4         2       3        1            3
//2 = 4bit      <<5=*32       16-1=15      >>1 = /2       1            <<2=*4      16-1=15               5         1       1        2            15
//3 = 8bit                                                             <<3=*8      64-1=63                                          3
//4 = invalid                                                          <<4=*16      1-1=&0                                          4
} TextureStruct;


#define logging true                   // Enable/Disable logging for debugging
#define logwithtime true               // Log with timestamp or just regular logging
//Note: printf uses APP_LOG_LEVEL_DEBUG
#if logging
  #if logwithtime
    #define    LOG_TIME(log_level, format, ...) app_log( \
      log_level, __FILE__, __LINE__, "[%d:%02d:%02d.%03d] " format, \
      (int)(time(NULL)%SECONDS_PER_DAY)/SECONDS_PER_HOUR, \
      (int)(time(NULL)%SECONDS_PER_HOUR)/SECONDS_PER_MINUTE, \
      (int)(time(NULL)%SECONDS_PER_MINUTE), time_ms(NULL, NULL), ## __VA_ARGS__ \
    )
  #else
    #define    LOG_TIME(log_level, format, ...)
  #endif
  #define      LOG_MS(...) app_log(APP_LOG_LEVEL_INFO, "ms", (int)((time(NULL)&255)*1000 + time_ms(NULL, NULL))&65535, __VA_ARGS__);
  #define         LOG(...) LOG_TIME(APP_LOG_LEVEL_DEBUG,         __VA_ARGS__)
  #define   LOG_DEBUG(...) LOG_TIME(APP_LOG_LEVEL_DEBUG,         __VA_ARGS__)
  #define LOG_WARNING(...) LOG_TIME(APP_LOG_LEVEL_WARNING,       __VA_ARGS__)
  #define   LOG_ERROR(...) LOG_TIME(APP_LOG_LEVEL_ERROR,         __VA_ARGS__)
  #define    LOG_INFO(...) LOG_TIME(APP_LOG_LEVEL_INFO,          __VA_ARGS__)
  #define LOG_VERBOSE(...) LOG_TIME(APP_LOG_LEVEL_DEBUG_VERBOSE, __VA_ARGS__)
#else
  #define         LOG(...)
  #define   LOG_DEBUG(...)
  #define LOG_WARNING(...)
  #define   LOG_ERROR(...)
  #define    LOG_INFO(...)
  #define LOG_VERBOSE(...)
  #define      LOG_MS(...)
  #define LOG_TIMESTAMP(...) 
#endif





int32_t sqrt32(int32_t a);
int32_t sqrt_int(int32_t a, int8_t depth);

// absolute value
int32_t abs32(int32_t x);
int16_t abs16(int16_t x);
int8_t  abs8 (int8_t  x);
int32_t abs_int(int32_t a);

// sign function returns: -1 or 0 or 1 if input is <0 or =0 or >0
int8_t  sign8 (int8_t  x);
int16_t sign16(int16_t x);
int32_t sign32(int32_t x);

//y=Log(x) base 2 (opposite of x=1<<y: Converts 32 to 5, 16 to 4, 65536 to 16)
uint8_t log16 (uint16_t x);

void LoadMapTextures();
void LoadSprites();
void UnLoadMapTexturesAndSprites();
void GenerateSquareMap();
void GenerateRandomMap();
void GenerateMazeMap(int32_t startx, int32_t starty);

void walk(int32_t direction, int32_t distance);  
void shoot_ray(int32_t start_x, int32_t start_y, int32_t angle);

uint8_t getmap(int32_t x, int32_t y);
void setmap(int32_t x, int32_t y, uint8_t value);


void draw_textbox(GContext *ctx, GRect textframe, char *text);
void draw_map(GContext *ctx, GRect box, int32_t zoom);
//void draw_3D(GContext *ctx, GRect box, uint8_t border_color); //, int32_t zoom);
void draw_3D(GContext *ctx, GRect box); //, int32_t zoom);



//extern int32_t Q1, Q2, Q3, Q4, Q5; // Generic global variables for debugging purposes

extern uint8_t map[];
extern SquareTypeStruct squaretype[]; // note squaretype[0]=out of bounds ceiling/floor rendering
extern TextureStruct texture[];
extern TextureStruct sprite[];

extern GBitmap *sprite_image[1];
extern GBitmap *sprite_mask[1];

extern PlayerStruct player;
extern ObjectStruct object[];
extern RayStruct ray;

extern bool emulator;

extern uint8_t number_of_existing_objects;
extern uint8_t object_distance_list[];

void update_and_sort_sprites();


// Websocket Commands
void init_app_message();
void send_status(int val);
void send_message(char *msg);
void send_position();





