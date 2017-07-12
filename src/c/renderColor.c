// ------------------------------------------------------------------------ //
//  Color Drawing Functions
// ------------------------------------------------------------------------ //
#if defined(PBL_COLOR) && defined(PBL_RECT)
#include "global.h"

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

uint8_t combine_colors(uint8_t bg_color, uint8_t fg_color) {
  return (shadowtable[((~fg_color)&0b11000000) + (bg_color&63)]&63) + shadowtable[fg_color];
}

// Shade a rectangular region
void shadow_rect(GContext *ctx, GRect rect, uint8_t alpha) {
  //alpha |= 0b00111111; // sanitize alpha input // commented out cause I'm careful and don't need no safety
  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
  if(framebuffer) {   // if successfully captured the framebuffer
    uint8_t* screen = gbitmap_get_data(framebuffer);
    for(uint16_t y_addr=rect.origin.y*144, row=0; row<rect.size.h; y_addr+=144, ++row)
      for(uint16_t x_addr=rect.origin.x, x=0; x<rect.size.w; ++x_addr, ++x)
      screen[y_addr+x_addr] = shadowtable[alpha & screen[y_addr+x_addr]];
    graphics_release_frame_buffer(ctx, framebuffer);
  }
}

//screen[y_addr+x_addr] = combine_colors(screen[y_addr+x_addr], color);
//shadow_rect(ctx, GRect(0, 74, 144, 20), 0b01111111);  // shadow bar in the middle




















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
void draw_map(GContext *ctx, GRect box, int32_t zoom) {
  // note: Currently doesn't handle drawing beyond screen boundaries
  // zoom = pixel size of each square
  shadow_rect(ctx, box, SOME_SHADOW);
  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
  if(framebuffer) {   // if successfully captured the framebuffer
    uint8_t* screen = gbitmap_get_data(framebuffer);
    int32_t x, y, yaddr, xaddr, xonmap, yonmap, yonmapinit;

    xonmap = ((player.x*zoom)>>6) - (box.size.w/2);  // Divide by ZOOM to get map X coord, but rounds [-ZOOM to 0] to 0 and plots it, so divide by ZOOM after checking if <0
    yonmapinit = ((player.y*zoom)>>6) - (box.size.h/2);
    for(x=0; x<box.size.w; x++, xonmap++) {
      xaddr = x+box.origin.x;        // X memory address

      if(xonmap>=0 && xonmap<(MAP_SIZE*zoom)) {
        yonmap = yonmapinit;
        yaddr = box.origin.y * 144;           // Y memory address

        for(y=0; y<box.size.h; y++, yonmap++, yaddr+=144)
          if(yonmap>=0 && yonmap<(MAP_SIZE*zoom))             // If within Y bounds
            if(map[(((yonmap/zoom)*MAP_SIZE))+(xonmap/zoom)]>127) //   Map shows a wall >127
              screen[xaddr + yaddr] = 0b11111111;              //     White dot
      }
    }

    graphics_release_frame_buffer(ctx, framebuffer);
  }  // endif successfully captured framebuffer

  graphics_context_set_fill_color(ctx, (GColor){.argb=((time_ms(NULL, NULL) % 250)>125?0b11000000:0b11111111)});          // Flashing dot (250 is rate, 125 (1/2 of 250) means half the time black, half white)
  graphics_fill_rect(ctx, GRect((box.size.w/2)+box.origin.x - 1, (box.size.h/2)+box.origin.y - 1, 3, 3), 0, GCornerNone); // Square Cursor

  graphics_context_set_stroke_color(ctx, GColorWhite); graphics_draw_rect(ctx, GRect(box.origin.x-1, box.origin.y-1, box.size.w+2, box.size.h+2)); // White Border
}




void fill_box(uint8_t *screen, GRect box, uint8_t color) {
  for(uint16_t addr=(box.origin.y*144)+box.origin.x, y=0; y<box.size.h; addr+=144-box.size.w,++y)
    for(uint16_t x=0; x<box.size.w; ++x, ++addr)
      screen[addr] = color;
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


uint8_t y_offset_bytes[168];
uint8_t y_offset_bits[168];

//int32_t Q1=0, Q2=0, Q3=0, Q4=0, Q5=0;
// implement more options: draw_3D_wireframe?  draw_3D_shaded?
//void draw_3D(GContext *ctx, GRect box, uint8_t border_color) { //, int32_t zoom) {
void draw_3D(GContext *ctx, GRect box) { //, int32_t zoom) {
  uint8_t border_color = 0;
  //mid_y = (box.size.h/2) or maybe box.origin.y + (box.size.h/2) (middle in view or pixel on screen)
  //mid_x = (box.size.w/2) or maybe box.origin.x + (box.size.w/2)
  //int32_t dx, dy;
  int16_t angle;
  int32_t farthest = 0; //colh, z;
  int32_t y, colheight, halfheight, bottom_half;
  uint32_t x, addr, addr2, xoffset, yoffset;
  uint8_t *target;
  uint8_t *palette;
  uint8_t txt;

  int32_t dist[144];                 // Array of non-cos adjusted distance for each vertical wall segment -- for sprite rendering
  halfheight = (box.size.h-1)>>1;         // Subtract one in case height is an even number
  bottom_half = (box.size.h&1) ? 0 : 144; // whether bottom-half column starts at the same center pixel or one below (due to odd/even box.size.h)

  // Draw background
  //graphics_context_set_fill_color(ctx, GColorBlack);  graphics_fill_rect(ctx, box, 0, GCornerNone); // Black background
  //graphics_context_set_fill_color(ctx, GColorCobaltBlue);  graphics_fill_rect(ctx, box, 0, GCornerNone); // Blue background
  // Draw Sky from horizion on up, rotate based upon player angle
  //graphics_context_set_fill_color(ctx, 1); graphics_fill_rect(ctx, GRect(box.origin.x, box.origin.y, box.size.w, box.size.h/2), 0, GCornerNone);    // White Sky  (Lightning?  Daytime?)
  //graphics_context_set_stroke_color(ctx, GColorOrange); graphics_draw_rect(ctx, GRect(box.origin.x-1, box.origin.y-1, box.size.w+2, box.size.h+2)); // Draw Box around view (not needed if fullscreen)

  GBitmap* framebuffer = graphics_capture_frame_buffer(ctx);
  if(framebuffer) {   // if successfully captured the framebuffer
    uint8_t *screen = gbitmap_get_data(framebuffer);

    fill_box(screen, box, 0b11110011);    // Fill with Purple background for designing

    if(border_color) draw_border(screen, box, border_color); // Draw Box around view (not needed if fullscreen)


    x = box.origin.x;  // X screen coordinate
    for(int16_t col = 0; col < box.size.w; ++col, ++x) {        // Begin RayTracing Loop
      angle = atan2_lookup((64*col/box.size.w) - 32, 64);    // dx = (64*(col-(box.size.w/2)))/box.size.w; dy = 64; angle = atan2_lookup(dx, dy);

      shoot_ray(player.x, player.y, player.facing + angle);  //Shoot rays out of player's eyes.  pew pew.
      ray.hit &= 127;                                        // Whether ray hit a block (>127) or not (<128), set ray.hit to valid block type [0-127]
      if(ray.dist > (uint32_t)farthest) farthest = ray.dist; // farthest (furthest?) wall (for sprite rendering. only render sprites closer than farthest wall)
      dist[col] = (uint32_t)ray.dist;                        // save distance of this column for sprite rendering later
      ray.dist *= cos_lookup(angle);                         // multiply by cos to stop fisheye lens (should be >>16 to get actual dist, but that's all done below)
      colheight = (box.size.h << 21) /  ray.dist;    // half wall segment height = box.size.h * wallheight * 64(the "zoom factor") / (distance >> 16) // now /2 (<<21 instead of 22)
      if(colheight>halfheight) colheight=halfheight; // Make sure line isn't drawn beyond bounding box




      //draw_walls()

      uint8_t alpha=0b11111111;  // Extra shading ANDed to texture's alpha (which is probably 0b11xxxxxx)
      //       zzz=ray.dist>>16;
      //       if(zzz>0b0000000010000000/*32*/)  alpha=0b10111111;
      //       if(zzz>0b0000001000000000/*64*/)  alpha=0b01111111;
      //       if(zzz>0b0000010000000000/*128*/) alpha=0b00111111;
      // end shade calculation


      // Draw Color Walls
      txt = squaretype[ray.hit].face[ray.face];
      palette = texture[txt].palette;
      target = texture[txt].data + (ray.offset<<texture[txt].bytes_per_row) + (1<<(texture[txt].bytes_per_row-1)); // put pointer in the middle of row texture.y=ray.offset (pointer = texture's upper left [0,0] + y*)

      addr = x + ((box.origin.y + halfheight) * 144); // address of screen pixel vertically centered at column X
      addr2 = addr + bottom_half;                         // If box.size.h is even, there's no center pixel (else top and bottom half_column_heights are different), so start bottom column one pixel lower (or not, if h is odd)
      y=0; yoffset=0;  // y is y +/- from vertical center, yoffset is the screen memory position of y (and is always = y*144)
      for(; y<=colheight; y++, yoffset+=144) {
        xoffset =  ((y * ray.dist / box.size.h) >> 16); // xoffset = which pixel of the texture is hit (0-31).  See Footnote 2
        screen[addr  - yoffset] = shadowtable[alpha & palette[(*(target - 1 - (xoffset>>texture[txt].pixels_per_byte)) >> ((                                 (xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)]];  // Draw Top Half
        screen[addr2 + yoffset] = shadowtable[alpha & palette[(*(target     + (xoffset>>texture[txt].pixels_per_byte)) >> (((7>>texture[txt].bits_per_pixel)-(xoffset&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel)&texture[txt].colormax)]];  // Draw Bottom Half (Texture is horizontally mirrored top half)
      }
      // End Draw Walls

      // Draw Floor/Ceiling
      int32_t temp_x = (((box.size.h << 5) * cos_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
      int32_t temp_y = (((box.size.h << 5) * sin_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
      for(; y<=halfheight; y++, yoffset+=144) {       // y and yoffset continue from wall top&bottom to view edge (unless wall is taller than view edge)
        int32_t map_x = player.x + (temp_x / y);     // map_x & map_y = spot on map the screen pixel points to
        int32_t map_y = player.y + (temp_y / y);     // map_y = player.y + dist_y, dist = (height/2 * 64 * (sin if y, cos if x) / i) (/cos to un-fisheye)
        ray.hit = getmap(map_x, map_y) & 127;        // ceiling/ground of which cell is hit.  &127 shouldn't be needed since it *should* be hitting a spot without a wall

        uint8_t alpha=0b11111111;
        //         if(y<64) alpha=0b10111111;
        //         if(y<16)  alpha=0b01111111;
        //         if(y<4)  alpha=0b00111111;

        map_x&=63; map_y&=63; // Get position on texture
        txt=squaretype[ray.hit].ceiling; screen[addr  - yoffset] = shadowtable[alpha & texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)]];
        txt=squaretype[ray.hit].floor;   screen[addr2 + yoffset] = shadowtable[alpha & texture[txt].palette[(((*(texture[txt].data + ((63-map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)]];
      } // End Floor/Ceiling

      ray.hit = dist[col]; // just to stop compiler from complaining if not rendering sprites
    } // End RayTracing Loop




    
    
    
    
    
    



// =======================================BEGIN SPRITE SECTION OF DRAW3D====================================================    
    update_and_sort_sprites();
    
    for(uint8_t j=0; j<number_of_existing_objects; ++j) {
      ObjectStruct *current_object = &object[object_distance_list[j]];
      int32_t object_dist = current_object->dist;  // note: make sure to NOT use cosine adjusted distance!  Else sprite 1px in front of you becomes super big and even if it's hundreds of px to the side it will still display on the screen
      
      if(cos_lookup(current_object->angle)>0) { // if object is in front of player.  note: if cos(angle)=0 then object is straight right or left of the player
        if(farthest>=object_dist) { // if ANY wall is further (or =) than object distance, then some part might be visible, so display it
          TextureStruct *current_sprite = &sprite[current_object->sprite];

          int32_t sprite_col = (box.size.w/2) + ((sin_lookup(current_object->angle)*box.size.w)>>16);  // column on screen of sprite center
          int32_t sprite_scale = (box.size.h);// * 20 / 10;
          int32_t sprite_width  =          (sprite_scale * current_sprite->height) / object_dist; // sprite_scale=box.size.h.  this should use box.size.w, but wanna keep sprite h/w proportionate
          int32_t sprite_height =          (sprite_scale * current_sprite->width ) / object_dist; // 
          int32_t sprite_vertical_offset = (sprite_scale * current_object->offset) / object_dist; // 

          int16_t sprite_x_min = sprite_col - (sprite_width / 2); // left edge of sprite on screen
          int16_t sprite_x_max = sprite_x_min + sprite_width;     // right edge.  was =spritecol+(spritewidth/2);  Changed to display whole sprite cause /2 loses info
          if(sprite_x_max>=0 && sprite_x_min<box.size.w) {        // if any of the sprite is horizontally within view
            int16_t x_min = sprite_x_min<0 ? 0: sprite_x_min;
            int16_t x_max = sprite_x_max>box.size.w ? box.size.w : sprite_x_max;

            int16_t sprite_y_max = (box.size.h + sprite_height + sprite_vertical_offset)/2;// + (((32*box.size.h) << 16) / (objectdist * cos_lookup(angle)));
            int16_t sprite_y_min = sprite_y_max - sprite_height; // note: sprite is not cos adjusted but offset is (to keep it in place)

            if(sprite_y_max>=0 && sprite_y_min<box.size.h) { // if any of the sprite is vertically within view
              int16_t y_min = sprite_y_min<0 ? 0 : sprite_y_min;
              int16_t y_max = sprite_y_max>box.size.h ? box.size.h : sprite_y_max;

//BEGIN SPRITE DRAWING LOOPS
              // Calculate which of the 8 faces should be drawn
              angle = atan2_lookup(player.x - current_object->x, player.y - current_object->y) + current_object->facing - (65536/4) + (32768) - (65536/16); // angle = atan2_lookup(dx, dy);
              uint16_t rotation = ((uint16_t)angle)>>13;

              uint16_t addr_start = ((box.origin.y + y_min) * 144) + (box.origin.x); // y location on screen + x location on screen (upper left corner)
              uint8_t *data = current_sprite->data + (rotation * 768);

              // Pre-calculate y_offsets (y point on texture that is hit) to save time by not calculating it every time in the inner loop
              for(int16_t y=y_min; y<y_max; y++) {
                uint16_t y_offset = ((y - sprite_y_min) * object_dist) / sprite_scale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight) [0-31]
                y_offset_bytes[y] = y_offset>>1;
                y_offset_bits[y]  = (1-(y_offset&1))<<2;
              }
              // END pre-calculate y_offsets

              for(int16_t x = x_min; x < x_max; x++) {
                if(dist[x]>=object_dist) {  // if not behind wall
                  uint16_t addr = addr_start + x;
                  target = data + ((47-(((x - sprite_x_min) * object_dist) / sprite_scale)) << current_sprite->bytes_per_row); // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
                  for(int16_t y=y_min; y<y_max; y++, addr+=144) {
                    uint8_t fg = current_sprite->palette[((*(target + y_offset_bytes[y])) >> y_offset_bits[y])&15]; // Get color (fg: foreground color)
                    screen[addr] = (shadowtable[((~fg)&0b11000000) + (screen[addr]&63)]&63) + shadowtable[fg];      // Plot pixel
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
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
// // =======================================BEGIN SPRITE SECTION OF DRAW3D====================================================    
//   update_and_sort_sprites();

//   int32_t sprite_col, object_dist;
//   for(uint8_t j=0; j<number_of_objects; ++j) {
//     object_dist = object[sprite_list[j]].dist;  // note: make sure to NOT use cosine adjusted distance!  Else sprite 1px in front of you becomes super big and even if it's hundreds of px to the side it will still display on the screen
    
//     if(cos_lookup(object[sprite_list[j]].angle)>0) { // if object is in front of player.  note: if cos(angle)=0 then object is straight right or left of the player
//       if(farthest>=object_dist) { // if ANY wall is further (or =) than object distance, then some part might be visible, so display it
//         sprite_col = (box.size.w/2) + ((sin_lookup(object[sprite_list[j]].angle)*box.size.w)>>16);  // column on screen of sprite center
//         int32_t sprite_scale = (box.size.h);// * 20 / 10;
//         int32_t sprite_width  =          (sprite_scale * sprite[object[sprite_list[j]].sprite].width)    / object_dist; // sprite_scale=box.size.h.  this should use box.size.w, but wanna keep sprite h/w proportionate
//         int32_t sprite_height =          (sprite_scale * sprite[object[sprite_list[j]].sprite].height)   / object_dist; // 
//         int32_t sprite_vertical_offset = (sprite_scale * object[sprite_list[j]].offset) / object_dist; // 

//         int16_t sprite_x_min = sprite_col - (sprite_width/2); // left edge of sprite on screen
//         int16_t sprite_x_max = sprite_x_min + sprite_width;   // right edge.  was =spritecol+(spritewidth/2);  Changed to display whole sprite cause /2 loses info
//         if(sprite_x_max>=0 && sprite_x_min<box.size.w) {    // if any of the sprite is horizontally within view
//           int16_t x_min = sprite_x_min<0 ? 0: sprite_x_min;
//           int16_t x_max = sprite_x_max>box.size.w ? box.size.w : sprite_x_max;

//           int16_t sprite_y_max = (box.size.h + sprite_height + sprite_vertical_offset)/2;// + (((32*box.size.h) << 16) / (objectdist * cos_lookup(angle)));
//           int16_t sprite_y_min = sprite_y_max - sprite_height; // note: sprite is not cos adjusted but offset is (to keep it in place)


//           if(sprite_y_max>=0 && sprite_y_min<box.size.h) { // if any of the sprite is vertically within view
//             int16_t y_min = sprite_y_min<0 ? 0 : sprite_y_min;
//             int16_t y_max = sprite_y_max>box.size.h ? box.size.h : sprite_y_max;
// //BEGIN SPRITE DRAWING LOOPS
//             uint16_t x_offset, y_offset, addr, addr_start;
//             // Calculate y_offset_buffer (y point on texture that is hit) to save time by not calculating it every time in the inner loop
//             uint8_t y_offset_buffer[168];
//             for(int16_t y=y_min; y<y_max; y++) {
//                   y_offset_buffer[y] = ((y - sprite_y_min) * object_dist) / sprite_scale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight) [0-31]
//             }
//             // END calculate y_offset_buffer
//             addr_start = ((box.origin.y + y_min) * 144) + (box.origin.x); // y location on screen + x location on screen (upper left corner)
//             for(int16_t x = x_min; x < x_max; x++) {
//               if(dist[x]>=object_dist) {  // if not behind wall
//                 //xoffset = (63-(((x - sprite_xmin) * objectdist) / spritescale)) << sprite[0].bytes_per_row; // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
//                 x_offset = ((sprite[object[sprite_list[j]].sprite].width-1)-(((x - sprite_x_min) * object_dist) / sprite_scale)) << sprite[object[sprite_list[j]].sprite].bytes_per_row; // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
//                 target = sprite[object[sprite_list[j]].sprite].data + x_offset; // target = sprite
//                 //x_addr = (box.origin.x + x);          // x location on screen
//                 //y_addr = y_addr_start; //y_addr = (box.origin.y + y_min) * 144; // y location on screen
//                 addr = addr_start + x;
//                 for(int16_t y=y_min; y<y_max; y++, addr+=144) {
//                   //y_offset = ((y - sprite_y_min) * object_dist) / sprite_scale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight) [0-31]
//                   y_offset = y_offset_buffer[y];
//                   //screen[xaddr+yaddr] = 0b11001100;
//                   //screen[xaddr+yaddr] = combine_colors(screen[xaddr+yaddr], 0b11001100);
                  
                  
//                   //screen[x_addr + y_addr] = combine_colors(screen[x_addr+y_addr], sprite[object[sprite_list[j]].sprite].palette[((*(target + (y_offset>>sprite[object[sprite_list[j]].sprite].pixels_per_byte))) >> (( /*(7>>sprite[object[sprite_list[j]].sprite].bits_per_pixel)-*/ (y_offset&(7>>sprite[object[sprite_list[j]].sprite].bits_per_pixel)))<<sprite[object[sprite_list[j]].sprite].bits_per_pixel))&sprite[object[sprite_list[j]].sprite].colormax]);  // make bit white or keep it black
                  
// //                     screen[x_addr + y_addr] = combine_colors(screen[x_addr+y_addr], sprite[object[sprite_list[j]].sprite].palette[((*(target + (y_offset>>sprite[object[sprite_list[j]].sprite].pixels_per_byte))) >> (((7>>sprite[object[sprite_list[j]].sprite].bits_per_pixel)-(y_offset&(7>>sprite[object[sprite_list[j]].sprite].bits_per_pixel)))<<sprite[object[sprite_list[j]].sprite].bits_per_pixel))&sprite[object[sprite_list[j]].sprite].colormax]);  // make bit white or keep it black
//                   uint8_t fg = sprite[object[sprite_list[j]].sprite].palette[((*(target + (y_offset>>sprite[object[sprite_list[j]].sprite].pixels_per_byte))) >> (((7>>sprite[object[sprite_list[j]].sprite].bits_per_pixel)-(y_offset&(7>>sprite[object[sprite_list[j]].sprite].bits_per_pixel)))<<sprite[object[sprite_list[j]].sprite].bits_per_pixel))&sprite[object[sprite_list[j]].sprite].colormax];
//                   screen[addr] = (shadowtable[((~fg)&0b11000000) + (screen[addr]&63)]&63) + shadowtable[fg];

                  
//                   //uint8_t combine_colors(uint8_t bg_color, uint8_t fg_color) {  return (shadowtable[((~fg_color)&0b11000000) + (bg_color&63)]&63) + shadowtable[fg_color];

//                   //screen[xaddr + yaddr] = sprite[0].palette[((*(target + (yoffset>>sprite[0].pixels_per_byte))) >> (((7>>sprite[0].bits_per_pixel)-(yoffset&(7>>sprite[0].bits_per_pixel)))<<sprite[0].bits_per_pixel))&sprite[0].colormax];  // make bit white or keep it black
//                      //texture[txt].palette[(((*(texture[txt].data + ((   map_x)<<texture[txt].bytes_per_row) + (map_y>>texture[txt].pixels_per_byte))) >> (((7>>texture[txt].bits_per_pixel)-(map_y&(7>>texture[txt].bits_per_pixel)))<<texture[txt].bits_per_pixel))&texture[txt].colormax)]];
//                 } // next y
//               } // end display column if in front of wall
//             } // next x
// //END SPRITE DRAWING LOOPS      
//           } // end display if within y bounds
//         } // end display if within x bounds
//       } // end display if within farthest
//     } // end display if not behind you
//   } // next obj
// // =======================================END SPRITE SECTION OF DRAW3D====================================================
    graphics_release_frame_buffer(ctx, framebuffer);
  }  // endif successfully captured framebuffer

} // end draw 3D function
#endif