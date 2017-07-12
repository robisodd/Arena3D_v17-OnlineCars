// ------------------------------------------------------------------------ //
//  Color Drawing Functions
// ------------------------------------------------------------------------ //
#if defined(PBL_COLOR) && defined(PBL_ROUND)
#include "global.h"

int16_t fb_radius[] = {  // Half of width of row[y] or half of height of column[x].  Double for row width or column height.
  14, 19, 24, 27, 30, 33, 35, 38, 40, 42,
  44, 45, 47, 49, 50, 52, 53, 54, 56, 57,
  58, 59, 61, 62, 63, 64, 65, 66, 67, 68,
  68, 69, 70, 71, 72, 72, 73, 74, 75, 75,
  76, 77, 77, 78, 78, 79, 80, 80, 81, 81,
  82, 82, 83, 83, 83, 84, 84, 85, 85, 85,
  86, 86, 86, 87, 87, 87, 88, 88, 88, 88,
  88, 89, 89, 89, 89, 89, 90, 90, 90, 90,
  90, 90, 90, 90, 90, 90, 90, 90, 90, 90,
  
  90, 90, 90, 90, 90, 90, 90, 90, 90, 90,
  90, 90, 90, 90, 89, 89, 89, 89, 89, 88,
  88, 88, 88, 88, 87, 87, 87, 86, 86, 86,
  85, 85, 85, 84, 84, 83, 83, 83, 82, 82,
  81, 81, 80, 80, 79, 78, 78, 77, 77, 76,
  75, 75, 74, 73, 72, 72, 71, 70, 69, 68,
  68, 67, 66, 65, 64, 63, 62, 61, 59, 58,
  57, 56, 54, 53, 52, 50, 49, 47, 45, 44,
  42, 40, 38, 35, 33, 30, 27, 24, 19, 14
};

int16_t *fb_middle_radius = fb_radius + 90;  // Half of width or height of row or column Vertically in the Middle or Centered Horizontally

int16_t fb_row[] = { // Memory position for x=0 (may need to add x to be within screen bounds)
      0,    33,    76,   127,   184,   247,   315,   388,   466,   548,
    634,   723,   815,   911,  1010,  1112,  1217,  1324,  1434,  1547,
   1662,  1779,  1899,  2022,  2147,  2274,  2403,  2534,  2667,  2802,
   2938,  3075,  3214,  3355,  3498,  3642,  3787,  3934,  4083,  4233,
   4384,  4537,  4691,  4846,  5002,  5159,  5318,  5478,  5639,  5801,
   5964,  6128,  6293,  6459,  6625,  6792,  6960,  7129,  7299,  7469,
   7640,  7812,  7984,  8157,  8331,  8505,  8680,  8856,  9032,  9208,
   9384,  9561,  9739,  9917, 10095, 10273, 10452, 10632, 10812, 10992,
  11172, 11352, 11532, 11712, 11892, 12072, 12252, 12432, 12612, 12792,

  12972, 13152, 13332, 13512, 13692, 13872, 14052, 14232, 14412, 14592,
  14772, 14952, 15132, 15312, 15491, 15669, 15847, 16025, 16203, 16380,
  16556, 16732, 16908, 17084, 17259, 17433, 17607, 17780, 17952, 18124,
  18295, 18465, 18635, 18804, 18972, 19139, 19305, 19471, 19636, 19800,
  19963, 20125, 20286, 20446, 20605, 20762, 20918, 21073, 21227, 21380,
  21531, 21681, 21830, 21977, 22122, 22266, 22409, 22550, 22689, 22826,
  22962, 23097, 23230, 23361, 23490, 23617, 23742, 23865, 23985, 24102,
  24217, 24330, 24440, 24547, 24652, 24754, 24853, 24949, 25041, 25130,
  25216, 25298, 25376, 25449, 25517, 25580, 25637, 25688, 25731, 25764
};

// ================================================================ //
//   How to support transparencies and the alpha channel
// ================================================================ //
#define FULL_SHADOW 0b00111111 // 100% black
#define MORE_SHADOW 0b01111111 // 66%  dark
#define SOME_SHADOW 0b10111111 // 33%  shade
#define NONE_SHADOW 0b11111111 // full color

uint8_t shadowtable[] = {192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /* ------------------ */ \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /*      0% alpha      */ \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /*        Clear       */ \
                         192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,  /* ------------------ */ \

                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,  /* ------------------ */ \
                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,  /*     33% alpha      */ \
                         192,192,192,193,192,192,192,193,192,192,192,193,196,196,196,197,  /*    Transparent     */ \
                         208,208,208,209,208,208,208,209,208,208,208,209,212,212,212,213,  /* ------------------ */ \

                         192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202,  /* ------------------ */ \
                         192,192,193,194,192,192,193,194,196,196,197,198,200,200,201,202,  /*     66% alpha      */ \
                         208,208,209,210,208,208,209,210,212,212,213,214,216,216,217,218,  /*    Translucent     */ \
                         224,224,225,226,224,224,225,226,228,228,229,230,232,232,233,234,  /* ------------------ */ \

                         192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,  /* ------------------ */ \
                         208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,  /*    100% alpha      */ \
                         224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,  /*      Opaque        */ \
                         240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255}; /* ------------------ */

// uint8_t combine_colors(uint8_t bg_color, uint8_t fg_color) {
//   return (shadowtable[((~fg_color)&0b11000000) + (bg_color&63)]&63) + shadowtable[fg_color];
// }

// Shade a rectangular region
// void shadow_rect(GContext *ctx, GRect rect, uint8_t alpha) {
//   //alpha |= 0b00111111; // sanitize alpha input // commented out cause I'm careful and don't need no safety
  
//   GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
//   if(framebuffer) {   // if successfully captured the framebuffer
//     uint8_t* screen = gbitmap_get_data(framebuffer);
//     for(uint16_t y_addr=rect.origin.y*144, row=0; row<rect.size.h; y_addr+=144, ++row)
//       for(uint16_t x_addr=rect.origin.x, x=0; x<rect.size.w; ++x_addr, ++x)
//       screen[y_addr+x_addr] = shadowtable[alpha & screen[y_addr+x_addr]];
//     graphics_release_frame_buffer(ctx, framebuffer);
//   }
// }

void fill_translucent_rect(GContext *ctx, GRect rect, GColor color) {
  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);                // Capture the framebuffer
  if(framebuffer) {                                                         // if successfully captured
    // Section 1: Bounds Checking and Iteration
    int16_t h = gbitmap_get_bounds(framebuffer).size.h;                     // Get screen height
    if(rect.origin.y<0) {rect.size.h += rect.origin.y; rect.origin.y = 0;}  // if top    is beyond screen bounds, then adjust
    if(rect.origin.y + rect.size.h > h) rect.size.h = h - rect.origin.y;    // if bottom is beyond screen bounds, then adjust
    for(int y = rect.origin.y; y < rect.origin.y+rect.size.h; y++) {        // Iterate over all y of visible part of rect

      GBitmapDataRowInfo info = gbitmap_get_data_row_info(framebuffer, y);  // Get visible width
      if(info.min_x < rect.origin.x) info.min_x = rect.origin.x;            // If left  is within screen bounds, then adjust
      if(info.max_x > rect.origin.x + rect.size.w)                          // If right is within screen bounds
        info.max_x = rect.origin.x + rect.size.w;                           //   then adjust right side
      for(int x = info.min_x; x <= info.max_x; x++) {                       // Iterate over all x of visible part of rect
        //info.data[x] = shadowtable[color & info.data[x]];
        info.data[x] = (shadowtable[((~color.argb)&0b11000000) + (info.data[x]&0b00111111)]&0b00111111) + shadowtable[color.argb];
      }
    }
    graphics_release_frame_buffer(ctx, framebuffer);                        // Release the Kraken!  ... err, framebuffer.
  }
}

void shadow_rect(GContext *ctx, GRect rect, uint8_t alpha) {
  fill_translucent_rect(ctx, rect, (GColor){.argb = alpha | 0b00111111});
}









// goes through all sprites, sees if they still exist
void update_and_sort_sprites() {

  int32_t dx, dy;
  number_of_existing_objects=0;
  
  for(uint8_t i=0; i<MAX_OBJECTS; i++) {
    if(object[i].type) {  // if exists
      object_distance_list[number_of_existing_objects] = i;
      number_of_existing_objects++;

      dx = object[i].x - player.x;
      dy = object[i].y - player.y;
      object[i].angle = atan2_lookup(dy, dx); // angle = angle between player's x,y and sprite's x,y
         object[i].dist = (((dx^(dx>>31)) - (dx>>31)) > ((dy^(dy>>31)) - (dy>>31))) ? (dx<<16) / cos_lookup(object[i].angle) : (dy<<16) / sin_lookup(object[i].angle);
      // object[i].dist = (((dx<0)?0-dx:dx) > ((dy<0)?0-dy:dy))                     ? (dx<<16) / cos_lookup(object[i].angle) : (dy<<16) / sin_lookup(object[i].angle);
      // object[i].dist = (abs32(dx)>abs32(dy))                                     ? (dx<<16) / cos_lookup(object[i].angle) : (dy<<16) / sin_lookup(object[i].angle);
      object[i].angle -= player.facing;  // angle is now angle between center view column and object. <0=left of center, 0=center column, >0=right of center
    }
  }
  
  // Insertion Sort: Sort object_distance_list in order from farthest to closest
  // Chosen by comparing sorting algorithms here: http://www.sorting-algorithms.com
  // Adapted from pseudocode found here: https://en.wikipedia.org/wiki/Insertion_sort
  for(uint8_t i=1; i<number_of_existing_objects; i++) {
    uint8_t temp = object_distance_list[i];
    uint8_t j = i;
    while(j>0 && object[object_distance_list[j-1]].dist<object[temp].dist) {
      object_distance_list[j] = object_distance_list[j-1];
      --j;
    }
    object_distance_list[j] = temp;
  }
  // END Insertion Sort
}










// ------------------------------------------------------------------------ //
//  Drawing to screen functions
// ------------------------------------------------------------------------ //
// void fill_window(GContext *ctx, uint8_t *data) {
//   for(uint16_t y=0, yaddr=0; y<168; y++, yaddr+=20)
//     for(uint16_t x=0; x<19; x++)
//       ((uint8_t*)(((GBitmap*)ctx)->addr))[yaddr+x] = data[y%8];
// }
//for(uint16_t y=0,; y<168*144; y+=144) for(uint16_t x=0; x<144; x++) screen[y+x] = 0b11000110;  // Fill with Blue background
//for(uint16_t i=0; i<168*144; i++) screen[i] = 0b11000110;  // Fill entire screen with Blue background

void draw_textbox(GContext *ctx, GRect textframe, char *text) {
  //graphics_context_set_fill_color(ctx, GColorBlack);   graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Solid Rectangle
  shadow_rect(ctx, textframe, MORE_SHADOW);
  graphics_context_set_stroke_color(ctx, GColorWhite); graphics_draw_rect(ctx, textframe);                //White Rectangle Border  
  graphics_context_set_text_color(ctx, GColorWhite);  // White Text
  graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);  //Write Text
}


// 1-pixel-per-square map:
//   for (int16_t x = 0; x < MAP_SIZE; x++) for (int16_t y = 0; y < MAP_SIZE; y++) {graphics_context_set_stroke_color(ctx, map[y*MAP_SIZE+x]>0?1:0); graphics_draw_pixel(ctx, GPoint(x, y));}
void draw_map(GContext *ctx, GRect rect, int32_t zoom) {
  // note: Currently doesn't handle drawing beyond screen boundaries
  // zoom = pixel size of each square
  shadow_rect(ctx, rect, SOME_SHADOW);
  int32_t x, y, yaddr, xaddr, xonmap, yonmap, xonmapinit;
  xonmapinit = ((player.x*zoom)>>6) - (rect.size.w/2);  // Divide by ZOOM to get map X coord, but rounds [-ZOOM to 0] to 0 and plots it, so divide by ZOOM after checking if <0
  yonmap     = ((player.y*zoom)>>6) - (rect.size.h/2);

  
  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);                // Capture the framebuffer
  if(framebuffer) {                                                         // if successfully captured
    // Section 1: Bounds Checking and Iteration
    int16_t h = gbitmap_get_bounds(framebuffer).size.h;                     // Get screen height
    if(rect.origin.y<0) {
      yonmap -= rect.origin.y;
      rect.size.h += rect.origin.y;
      rect.origin.y = 0;
    }  // if top    is beyond screen bounds, then adjust
    if(rect.origin.y + rect.size.h > h) rect.size.h = h - rect.origin.y;    // if bottom is beyond screen bounds, then adjust
    for(int y = rect.origin.y; y < rect.origin.y+rect.size.h; y++, yonmap++) {        // Iterate over all y of visible part of rect
      if(yonmap>=0 && yonmap<(MAP_SIZE*zoom)) {             // If within Y bounds
        GBitmapDataRowInfo info = gbitmap_get_data_row_info(framebuffer, y);  // Get visible width
        xonmap = xonmapinit;
        if(info.min_x < rect.origin.x) {
          xonmap += rect.origin.x - info.min_x;
          info.min_x = rect.origin.x;            // If left  is within screen bounds, then adjust
        }
        if(info.max_x > rect.origin.x + rect.size.w)                          // If right is within screen bounds
          info.max_x = rect.origin.x + rect.size.w;                           //   then adjust right side
        for(int x = info.min_x; x <= info.max_x; x++)                       // Iterate over all x of visible part of rect
          if(xonmap>=0 && xonmap<(MAP_SIZE*zoom))
            if(map[(((yonmap/zoom)*MAP_SIZE))+(xonmap/zoom)]>127) //   Map shows a wall >127
              info.data[x] = 0b11111111;              //     White dot
      }
    }
    graphics_release_frame_buffer(ctx, framebuffer);
  }  // endif successfully captured framebuffer

  graphics_context_set_fill_color(ctx, (GColor){.argb=((time_ms(NULL, NULL) % 250)>125?0b11000000:0b11111111)});          // Flashing dot (250 is rate, 125 (1/2 of 250) means half the time black, half white)
  graphics_fill_rect(ctx, GRect((rect.size.w/2)+rect.origin.x - 1, (rect.size.h/2)+rect.origin.y - 1, 3, 3), 0, GCornerNone); // Square Cursor

  graphics_context_set_stroke_color(ctx, GColorWhite); graphics_draw_rect(ctx, GRect(rect.origin.x-1, rect.origin.y-1, rect.size.w+2, rect.size.h+2)); // White Border
}




void fill_box(GContext *ctx, GRect box, GColor color) {
  fill_translucent_rect(ctx, box, color);
}

void draw_border(uint8_t *screen, GRect box, uint8_t color) {
  uint16_t lft = (box.origin.x < 1) ? 0 : box.origin.x - 1;
  uint16_t rgt = (box.origin.x + box.size.w > 143) ? 143 : box.origin.x + box.size.w;
  uint16_t top = (box.origin.y < 1) ? 0 : (box.origin.y - 1)*144;
  uint16_t bot = (box.origin.y + box.size.h > 167) ? 167*144 : (box.origin.y + box.size.h)*144;

  for(uint16_t x=lft; x<=rgt; ++x) {
    screen[top+x]=0b11101010;
    screen[bot+x]=0b11010101;
  }
  for(uint16_t y=top; y<bot; y+=144) {
    screen[lft+y]=0b11101010;
    screen[rgt+y]=0b11010101;
  }
}

void draw_3Da(GContext *ctx, GRect box) { //, int32_t zoom) {
  fill_box(ctx, box, (GColor){.argb=0b11110011});    // Fill with Purple background for designing
}

//int32_t Q1=0, Q2=0, Q3=0, Q4=0, Q5=0;
// implement more options: draw_3D_wireframe?  draw_3D_shaded?
//void draw_3Db(GContext *ctx, GRect box, uint8_t border_color) { //, int32_t zoom) {
void draw_3D(GContext *ctx, GRect box) { //, int32_t zoom) {
  uint8_t border_color = 0;
  //mid_y = (box.size.h/2) or maybe box.origin.y + (box.size.h/2) (middle in view or pixel on screen)
  //mid_x = (box.size.w/2) or maybe box.origin.x + (box.size.w/2)
  //int32_t dx, dy;
  int16_t angle;
  int32_t farthest = 0; //colh, z;
  int32_t y, colheight, half_col_height, bottom_half, half_rect_height;
  uint32_t x, top_center_y, bottom_center_y, xoffset, yoffset;
  uint8_t *target;
  uint8_t *palette;
  uint8_t txt;

  int32_t dist[box.size.w];                 // Array of non-cos adjusted distance for each vertical wall segment -- for sprite rendering
  half_rect_height = (box.size.h-1)>>1;         // Subtract one in case height is an even number
  top_center_y = ((box.origin.y + half_rect_height)); // address of screen pixel vertically centered at column X
  bottom_half = (box.size.h&1) ? 0 : 1; // whether bottom-half column starts at the same center pixel or one below (due to odd/even box.size.h)
  bottom_center_y = top_center_y + bottom_half;                         // If box.size.h is even, there's no center pixel (else top and bottom half_column_heights are different), so start bottom column one pixel lower (or not, if h is odd)

  // Draw background
  //graphics_context_set_fill_color(ctx, GColorBlack);  graphics_fill_rect(ctx, box, 0, GCornerNone); // Black background
  //graphics_context_set_fill_color(ctx, GColorCobaltBlue);  graphics_fill_rect(ctx, box, 0, GCornerNone); // Blue background
  // Draw Sky from horizion on up, rotate based upon player angle
  //graphics_context_set_fill_color(ctx, 1); graphics_fill_rect(ctx, GRect(box.origin.x, box.origin.y, box.size.w, box.size.h/2), 0, GCornerNone);    // White Sky  (Lightning?  Daytime?)
  //graphics_context_set_stroke_color(ctx, GColorOrange); graphics_draw_rect(ctx, GRect(box.origin.x-1, box.origin.y-1, box.size.w+2, box.size.h+2)); // Draw Box around view (not needed if fullscreen)

  fill_box(ctx, box, (GColor){.argb=0b11110011});    // Fill with Purple background for designing
  
  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
  if(framebuffer) {   // if successfully captured the framebuffer
    uint8_t *screen = gbitmap_get_data(framebuffer);
    
    //if(border_color) draw_border(screen, box, border_color); // Draw Box around view (not needed if fullscreen)
    
    x = box.origin.x;  // X screen coordinate
    for(int16_t col = 0; col < box.size.w; ++col, ++x) {        // Begin RayTracing Loop
      //if(x/2 == (x+1)/2) continue;
      //if(x>179) printf("ERROR X = %d", (int)x);
      //if(x<30) continue;
       
      //angle = atan2_lookup((64*col/box.size.w) - 32, 64);    // dx = (64*(col-(box.size.w/2)))/box.size.w; dy = 64; angle = atan2_lookup(dx, dy);
      //angle = atan2_lookup( col - 90,    180  ); // Normal-ish zoom
      //angle = atan2_lookup((col - 90)*2, 180*2); // same as Normal-ish zoom
      //angle = atan2_lookup( col - 90,     90  ); // Everything is square but is SUPER zoomed out
      //angle = atan2_lookup(col - 90, 180);
      #define dazoom 1
      //angle = atan2_lookup(col - 90, 90*dazoom);
      int16_t val1 = col-90;
      int16_t val2 = 90*dazoom;
      angle = atan2_lookup(val1, val2);
      //printf("atan(%d, %d) = %d", (int)val1, (int)val2, angle);

      shoot_ray(player.x, player.y, player.facing + angle);  //Shoot rays out of player's eyes.  pew pew.
      ray.hit &= 127;                                        // Whether ray hit a block (>127) or not (<128), set ray.hit to valid block type [0-127]
      if(ray.dist > (uint32_t)farthest) farthest = ray.dist; // farthest (furthest?) wall (for sprite rendering. only render sprites closer than farthest wall)
      dist[col] = (uint32_t)ray.dist;                        // save distance of this column for sprite rendering later
      ray.dist *= cos_lookup(angle);                         // multiply by cos to stop fisheye lens (should be >>16 to get actual dist, but that's all done below)
      //ray.dist <<= 16;                         // multiply by cos to stop fisheye lens (should be >>16 to get actual dist, but that's all done below)
      //colheight = (box.size.h << 21) /  ray.dist;    // half wall segment height = box.size.h * wallheight * 64(the "zoom factor") / (distance >> 16) // now /2 (<<21 instead of 22)
      #define wallheight 64
      int32_t zoomheight = box.size.h * dazoom;
      colheight = ((wallheight*32768*zoomheight) << 0) /  ray.dist;    // half wall segment height = box.size.h * wallheight * 64(the "zoom factor") / (distance >> 16) // now /2 (<<21 instead of 22)
      
      half_col_height = (half_rect_height>=fb_radius[x]) ? fb_radius[x] - 1 : half_rect_height;  // make sure current column isn't drawn beyond the screen edge
      //if(half_col_height>57) half_col_height = 57;
      if(colheight>half_col_height) colheight = half_col_height; // Make sure line isn't drawn beyond bounding box
      
      
      // Draw Color Walls
      txt = squaretype[ray.hit].face[ray.face];
      palette = texture[txt].palette;
      target = texture[txt].data + (ray.offset<<texture[txt].bytes_per_row) + (1<<(texture[txt].bytes_per_row-1)); // put pointer in the middle of row texture.y=ray.offset (pointer = texture's upper left [0,0] + y*)
      
      //y=0;  // y is y +/- from vertical center
      for(y=0; y<=colheight; y++) {
        //xoffset =  ((y * ray.dist / box.size.h) >> 16); // xoffset = which pixel of the texture is hit (0-31).  See Footnote 2
        xoffset =  ((y * ray.dist / zoomheight) >> 16); // xoffset = which pixel of the texture is hit (0-31).  See Footnote 2
        //screen[addr  - yoffset] = palette[(*(target - 1 - (xoffset>>texture[txt].pixels_per_byte)) >> ((                                 (xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)];  // Draw Top Half
        //screen[addr2 + yoffset] = palette[(*(target     + (xoffset>>texture[txt].pixels_per_byte)) >> (((7>>texture[txt].bits_per_pixel)-(xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)];  // Draw Bottom Half (Texture is horizontally mirrored top half)
        screen[fb_row[top_center_y    - y] + x] = palette[(*(target - 1 - (xoffset>>texture[txt].pixels_per_byte)) >> ((                                 (xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)];  // Draw Top Half
        screen[fb_row[bottom_center_y + y] + x] = palette[(*(target     + (xoffset>>texture[txt].pixels_per_byte)) >> (((7>>texture[txt].bits_per_pixel)-(xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)];  // Draw Bottom Half (Texture is horizontally mirrored top half)
      }
      // End Draw Walls
      
      
      // Draw Floor/Ceiling
//       int32_t temp_x = (((box.size.h << 5) * cos_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
//       int32_t temp_y = (((box.size.h << 5) * sin_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
      int32_t temp_x = (((zoomheight << 5) * cos_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
      int32_t temp_y = (((zoomheight << 5) * sin_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
      for(; y<=half_col_height; y++) {       // y and yoffset continue from wall top&bottom to view edge (unless wall is taller than view edge)
        int32_t map_x = player.x + (temp_x / y);     // map_x & map_y = spot on map the screen pixel points to
        int32_t map_y = player.y + (temp_y / y);     // map_y = player.y + dist_y, dist = (height/2 * 64 * (sin if y, cos if x) / i) (/cos to un-fisheye)
        ray.hit = getmap(map_x, map_y) & 127;        // ceiling/ground of which cell is hit.  &127 shouldn't be needed since it *should* be hitting a spot without a wall

        map_x&=63; map_y&=63; // Get position on texture
        txt=squaretype[ray.hit].ceiling; screen[fb_row[top_center_y    - y] + x] = texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)];
        txt=squaretype[ray.hit].floor;   screen[fb_row[bottom_center_y + y] + x] = texture[txt].palette[(((*(texture[txt].data + ((63-map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)];
//         txt=squaretype[ray.hit].ceiling; screen[addr  - yoffset] = texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)];
//         txt=squaretype[ray.hit].floor;   screen[addr2 + yoffset] = texture[txt].palette[(((*(texture[txt].data + ((63-map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)];
      } // End Floor/Ceiling
      
      
      ray.hit = dist[col]; // just to stop compiler from complaining if not rendering sprites
    } // End RayTracing Loop
/*
// =======================================BEGIN SPRITE SECTION OF DRAW3D====================================================    
  update_and_sort_sprites();

  int32_t sprite_col, object_dist;
  for(uint8_t j=0; j<number_of_existing_objects; ++j) {
    object_dist = object[object_distance_list[j]].dist;  // note: make sure to NOT use cosine adjusted distance!  Else sprite 1px in front of you becomes super big and even if it's hundreds of px to the side it will still display on the screen
    
    if(cos_lookup(object[object_distance_list[j]].angle)>0) { // if object is in front of player.  note: if cos(angle)=0 then object is straight right or left of the player
      if(farthest>=object_dist) { // if ANY wall is further (or =) than object distance, then some part might be visible, so display it
        sprite_col = (box.size.w/2) + ((sin_lookup(object[object_distance_list[j]].angle)*box.size.w)>>16);  // column on screen of sprite center
        int32_t sprite_scale = (box.size.h);// * 20 / 10;
        int32_t sprite_width  =          (sprite_scale * sprite[object[object_distance_list[j]].sprite].width)    / object_dist; // sprite_scale=box.size.h.  this should use box.size.w, but wanna keep sprite h/w proportionate
        int32_t sprite_height =          (sprite_scale * sprite[object[object_distance_list[j]].sprite].height)   / object_dist; // 
        int32_t sprite_vertical_offset = (sprite_scale * object[object_distance_list[j]].offset) / object_dist; // 

        int16_t sprite_x_min = sprite_col - (sprite_width/2); // left edge of sprite on screen
        int16_t sprite_x_max = sprite_x_min + sprite_width;   // right edge.  was =spritecol+(spritewidth/2);  Changed to display whole sprite cause /2 loses info
        if(sprite_x_max>=0 && sprite_x_min<box.size.w) {    // if any of the sprite is horizontally within view
          int16_t x_min = sprite_x_min<0 ? 0: sprite_x_min;
          int16_t x_max = sprite_x_max>box.size.w ? box.size.w : sprite_x_max;

          int16_t sprite_y_max = (box.size.h + sprite_height + sprite_vertical_offset)/2;// + (((32*box.size.h) << 16) / (objectdist * cos_lookup(angle)));
          int16_t sprite_y_min = sprite_y_max - sprite_height; // note: sprite is not cos adjusted but offset is (to keep it in place)


          if(sprite_y_max>=0 && sprite_y_min<box.size.h) { // if any of the sprite is vertically within view
            int16_t y_min = sprite_y_min<0 ? 0 : sprite_y_min;
            int16_t y_max = sprite_y_max>box.size.h ? box.size.h : sprite_y_max;
//BEGIN SPRITE DRAWING LOOPS
            uint16_t x_addr, y_addr, x_offset, y_offset, y_addr_start;
            // Calculate y_offset_buffer (y point on texture that is hit) to save time by not calculating it every time in the inner loop
            uint8_t y_offset_buffer[168];
            for(int16_t y=y_min; y<y_max; y++) {
                  y_offset_buffer[y] = ((y - sprite_y_min) * object_dist) / sprite_scale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight) [0-31]
            }
            // END calculate y_offset_buffer
            y_addr_start = (box.origin.y + y_min) * 144; // y location on screen
            for(int16_t x = x_min; x < x_max; x++) {
              if(dist[x]>=object_dist) {  // if not behind wall
                //xoffset = (63-(((x - sprite_xmin) * objectdist) / spritescale)) << sprite[0].bytes_per_row; // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
                x_offset = ((sprite[object[object_distance_list[j]].sprite].width-1)-(((x - sprite_x_min) * object_dist) / sprite_scale)) << sprite[object[object_distance_list[j]].sprite].bytes_per_row; // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
                target = sprite[object[object_distance_list[j]].sprite].data + x_offset; // target = sprite
                x_addr = (box.origin.x + x);          // x location on screen
                y_addr = y_addr_start; //y_addr = (box.origin.y + y_min) * 144; // y location on screen
                for(int16_t y=y_min; y<y_max; y++, y_addr+=144) {
                  //y_offset = ((y - sprite_y_min) * object_dist) / sprite_scale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight) [0-31]
                  y_offset = y_offset_buffer[y];
                  //screen[xaddr+yaddr] = 0b11001100;
                  //screen[xaddr+yaddr] = combine_colors(screen[xaddr+yaddr], 0b11001100);
                  
                  
                  //screen[x_addr + y_addr] = combine_colors(screen[x_addr+y_addr], sprite[object[object_distance_list[j]].sprite].palette[((*(target + (y_offset>>sprite[object[object_distance_list[j]].sprite].pixels_per_byte))) >> (( // ** (7>>sprite[object[object_distance_list[j]].sprite].bits_per_pixel)- ** // (y_offset&(7>>sprite[object[object_distance_list[j]].sprite].bits_per_pixel)))<<sprite[object[object_distance_list[j]].sprite].bits_per_pixel))&sprite[object[object_distance_list[j]].sprite].colormax]);  // make bit white or keep it black
                  
//                     screen[x_addr + y_addr] = combine_colors(screen[x_addr+y_addr], sprite[object[object_distance_list[j]].sprite].palette[((*(target + (y_offset>>sprite[object[object_distance_list[j]].sprite].pixels_per_byte))) >> (((7>>sprite[object[object_distance_list[j]].sprite].bits_per_pixel)-(y_offset&(7>>sprite[object[object_distance_list[j]].sprite].bits_per_pixel)))<<sprite[object[object_distance_list[j]].sprite].bits_per_pixel))&sprite[object[object_distance_list[j]].sprite].colormax]);  // make bit white or keep it black
                  uint8_t fg = sprite[object[object_distance_list[j]].sprite].palette[((*(target + (y_offset>>sprite[object[object_distance_list[j]].sprite].pixels_per_byte))) >> (((7>>sprite[object[object_distance_list[j]].sprite].bits_per_pixel)-(y_offset&(7>>sprite[object[object_distance_list[j]].sprite].bits_per_pixel)))<<sprite[object[object_distance_list[j]].sprite].bits_per_pixel))&sprite[object[object_distance_list[j]].sprite].colormax];
                  screen[x_addr + y_addr] = (shadowtable[((~fg)&0b11000000) + (screen[x_addr + y_addr]&63)]&63) + shadowtable[fg];

                  
                  //uint8_t combine_colors(uint8_t bg_color, uint8_t fg_color) {  return (shadowtable[((~fg_color)&0b11000000) + (bg_color&63)]&63) + shadowtable[fg_color];

                  //screen[xaddr + yaddr] = sprite[0].palette[((*(target + (yoffset>>sprite[0].pixels_per_byte))) >> (((7>>sprite[0].bits_per_pixel)-(yoffset&(7>>sprite[0].bits_per_pixel)))<<sprite[0].bits_per_pixel))&sprite[0].colormax];  // make bit white or keep it black
                     //texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)]];
                } // next y
              } // end display column if in front of wall
            } // next x
//END SPRITE DRAWING LOOPS      
          } // end display if within y bounds
        } // end display if within x bounds
      } // end display if within farthest
    } // end display if not behind you
  } // next obj
// =======================================END SPRITE SECTION OF DRAW3D====================================================
*/
    graphics_release_frame_buffer(ctx, framebuffer);
  }  // endif successfully captured framebuffer

} // end draw 3D function
#endif