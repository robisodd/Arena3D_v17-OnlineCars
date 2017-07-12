// ------------------------------------------------------------------------ //
//  Black and White Drawing Functions
// ------------------------------------------------------------------------ //
#ifdef PBL_BW
#include "global.h"

//extern uint8_t map[];

//extern SquareTypeStruct squaretype[]; // note squaretype[0]=out of bounds ceiling/floor rendering
//extern PlayerStruct player;
//extern PlayerStruct object;
//extern RayStruct ray;

//extern GBitmap *texture[MAX_TEXTURES];
//extern TextureStruct texture[MAX_TEXTURES];
//extern GBitmap *sprite_image[1];
//extern GBitmap *sprite_mask[1];
//extern Layer *graphics_layer;

// ------------------------------------------------------------------------ //
//  Drawing to screen functions
// ------------------------------------------------------------------------ //
void fill_window(GContext *ctx, uint8_t *data) {
  for(uint16_t y=0, yaddr=0; y<168; y++, yaddr+=20)
    for(uint16_t x=0; x<19; x++)
//       ((uint8_t*)(((GBitmap*)ctx)->addr))[yaddr+x] = data[y%8];
    ((uint8_t*)*(size_t*)ctx)[yaddr+x] = data[y%8];
}

void draw_textbox(GContext *ctx, GRect textframe, char *text) {
    graphics_context_set_fill_color(ctx, GColorBlack);   graphics_fill_rect(ctx, textframe, 0, GCornerNone);  //Black Solid Rectangle
    graphics_context_set_stroke_color(ctx, GColorWhite); graphics_draw_rect(ctx, textframe);                //White Rectangle Border  
    graphics_context_set_text_color(ctx, GColorWhite);  // White Text
    graphics_draw_text(ctx, text, fonts_get_system_font(FONT_KEY_GOTHIC_14), textframe, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);  //Write Text
}


// 1-pixel-per-square map:
//   for (int16_t x = 0; x < MAP_SIZE; x++) for (int16_t y = 0; y < MAP_SIZE; y++) {graphics_context_set_stroke_color(ctx, map[y*MAP_SIZE+x]>0?1:0); graphics_draw_pixel(ctx, GPoint(x, y));}
void draw_map(GContext *ctx, GRect box, int32_t zoom) {
  // note: Currently doesn't handle drawing beyond screen boundaries
  // zoom = pixel size of each square
//   uint32_t *ctx32 = ((uint32_t*)(((GBitmap*)ctx)->addr));  // framebuffer pointer (screen memory)
  uint32_t *ctx32 = (uint32_t*)*(size_t*)ctx;  // framebuffer pointer (screen memory)
  
  uint32_t xbit;
  int32_t x, y, yaddr, xaddr, xonmap, yonmap, yonmapinit;
  
  xonmap = ((player.x*zoom)>>6) - (box.size.w/2);  // Divide by ZOOM to get map X coord, but rounds [-ZOOM to 0] to 0 and plots it, so divide by ZOOM after checking if <0
  yonmapinit = ((player.y*zoom)>>6) - (box.size.h/2);
  for(x=0; x<box.size.w; x++, xonmap++) {
    xaddr = (x+box.origin.x) >> 5;        // X memory address
    xbit = ~(1<<((x+box.origin.x) & 31)); // X bit shift level (normally wouldn't invert it with ~, but ~ is used more often than not)
    if(xonmap>=0 && xonmap<(MAP_SIZE*zoom)) {
      yonmap = yonmapinit;
      yaddr = box.origin.y * 5;           // Y memory address
      for(y=0; y<box.size.h; y++, yonmap++, yaddr+=5) {
        if(yonmap>=0 && yonmap<(MAP_SIZE*zoom)) {               // If within Y bounds
          if(map[(((yonmap/zoom)*MAP_SIZE))+(xonmap/zoom)]>127) //   Map shows a wall >127
            ctx32[xaddr + yaddr] |= ~xbit;                     //     White dot (un-invert xbit by inverting it again)
          else                                                 //   Map shows <= 0
            ctx32[xaddr + yaddr] &= xbit;                      //     Black dot
        } else {                                               // Else: Out of Y bounds
          ctx32[xaddr + yaddr] &= xbit;                        //   Black dot
        }
      }
    } else {                                // Out of X bounds: Black vertical stripe
      for(yaddr=box.origin.y*5; yaddr<((box.size.h + box.origin.y)*5); yaddr+=5)
        ctx32[xaddr + yaddr] &= xbit;
    }
  }

  graphics_context_set_fill_color(ctx, (time_ms(NULL, NULL) % 250)>125?GColorBlack:GColorWhite);                      // Flashing dot
  graphics_fill_rect(ctx, GRect((box.size.w/2)+box.origin.x - 1, (box.size.h/2)+box.origin.y - 1, 3, 3), 0, GCornerNone); // Square Cursor

  graphics_context_set_stroke_color(ctx, GColorWhite); graphics_draw_rect(ctx, GRect(box.origin.x-1, box.origin.y-1, box.size.w+2, box.size.h+2)); // White Border
}




uint8_t y_offset_bytes[168];
uint8_t y_offset_bits[168];


// implement more options
//draw_3D_wireframe?  draw_3D_shaded?
//void draw_3D(GContext *ctx, GRect box, uint8_t border_color) { //, int32_t zoom) {
void draw_3D(GContext *ctx, GRect box) { //, int32_t zoom) {
  uint8_t border_color = 0;
//mid_y = (box.size.h/2) or maybe box.origin.y + (box.size.h/2) (middle in view or pixel on screen)
//mid_x = (box.size.w/2) or maybe box.origin.x + (box.size.w/2)
  int32_t dx, dy;
  int16_t angle;
  int32_t farthest = 0; //colh, z;
  int32_t y, colheight, halfheight, bottom_half;
  uint32_t x, addr, addr2, xaddr, yaddr, xbit, xoffset, yoffset;
  uint32_t *target;
  int32_t dist[144];                 // Array of non-cos adjusted distance for each vertical wall segment -- for sprite rendering

  halfheight = (box.size.h-1)>>1;         // Subtract one in case height is an even number
  bottom_half = (box.size.h&1) ? 0 : 5; // whether bottom-half column starts at the same center pixel or one below (due to odd/even box.size.h)

//   uint32_t *ctx32 = ((uint32_t*)(((GBitmap*)ctx)->addr));  // framebuffer pointer (screen memory)
  uint32_t *ctx32 = (uint32_t*)*(size_t*)ctx;  // framebuffer pointer (screen memory)
  
  // Draw Box around view (not needed if fullscreen)
  //TODO: Draw straight to framebuffer
  if(border_color) {graphics_context_set_stroke_color(ctx, GColorBlack); graphics_draw_rect(ctx, GRect(box.origin.x-1, box.origin.y-1, box.size.w+2, box.size.h+2));}  //White Rectangle Border

  // Draw background
    graphics_context_set_fill_color(ctx, GColorBlack);  graphics_fill_rect(ctx, box, 0, GCornerNone); // Black background
    // Draw Sky from horizion on up, rotate based upon player angle
    //graphics_context_set_fill_color(ctx, 1); graphics_fill_rect(ctx, GRect(box.origin.x, box.origin.y, box.size.w, box.size.h/2), 0, GCornerNone); // White Sky  (Lightning?  Daytime?)

   x = box.origin.x;  // X screen coordinate
  for(int16_t col = 0; col < box.size.w; ++col, ++x) {        // Begin RayTracing Loop
    angle = atan2_lookup((64*col/box.size.w) - 32, 64);    // dx = (64*(col-(box.size.w/2)))/box.size.w; dy = 64; angle = atan2_lookup(dx, dy);
    
    shoot_ray(player.x, player.y, player.facing + angle);  //Shoot rays out of player's eyes.  pew pew.
    ray.hit &= 127;                                        // Whether ray hit a block (>127) or not (<128), set ray.hit to valid block type [0-127]
    if(ray.dist > (uint32_t)farthest) farthest = ray.dist; // farthest (furthest?) wall (for sprite rendering. only render sprites closer than farthest wall)
    dist[col] = (uint32_t)ray.dist;                        // save distance of this column for sprite rendering later
    ray.dist *= cos_lookup(angle);                         // multiply by cos to stop fisheye lens (should be >>16 to get actual dist, but that's all done below)
//  ray.dist <<= 16;    // use this if commenting out "ray.dist*=cos" above, cause it >>16's a lot below
    
      // Calculate amount of shade
      //z =  ray.dist >> 16; //z=(ray.dist*cos_lookup(angle))/TRIG_MAX_RATIO;  // z = distance
      //z -= 64; if(z<0) z=0;   // Make everything 1 block (64px) closer (solid white without having to be nearly touching)
      //z = sqrt_int(z,10) >> 1; // z was 0-RANGE(max dist visible), now z = 0 to 12: 0=close 10=distant.  Square Root makes it logarithmic
      //z -= 2; if(z<0) z=0;    // Closer still (zWas=zNow: 0-64=0, 65-128=2, 129-192=3, 256=4, 320=6, 384=6, 448=7, 512=8, 576=9, 640=10)

    colheight = (box.size.h << 21) /  ray.dist;    // half wall segment height = box.size.h * wallheight * 64(the "zoom factor") / (distance >> 16) // now /2 (<<21 instead of 22)
    if(colheight>halfheight) colheight=halfheight; // Make sure line isn't drawn beyond bounding box (also halve it cause of 2 32bit textures)

    // Draw Wall
    // Texture the Ray hit, point to 1st half of texture (half, cause a 64x64px texture menas there's 2 uint32_t per texture row.  Also why <<1 (i.e. *2) below)
    target = (uint32_t*)(texture[squaretype[ray.hit].face[ray.face]].data) + (ray.offset<<1);
    //target = (uint32_t*)(texture[squaretype[ray.hit].face[ray.face]].data) + (ray.offset<<(texture[squaretype[ray.hit].face[ray.face]].bytes_per_row-2));

    addr = (x >> 5) + ((box.origin.y + halfheight) * 5); // 32bit memory word containing pixel vertically centered at X. (Address=xaddr + yaddr = (Pixel.X/32) + 5*Pixel.Y)
    xbit = x & 31;        // X bit-shift amount (for which bit within screen memory's 32bit word the pixel exists)
    addr2 = addr + bottom_half;                         // If box.size.h is even, there's no center pixel (else top and bottom half_column_heights are different), so start bottom column one pixel lower (or not, if h is odd)

    if(texture[squaretype[ray.hit].face[ray.face]].bytes_per_row==3) {// If wall texture exists
      y=0; yoffset=0;  // y is y +/- from vertical center, yoffset is the screen memory position of y (and is always = y*5)
      for(; y<=colheight; ++y, yoffset+=5) {
        xoffset = (y * ray.dist / box.size.h) >> 16; // xoffset = which pixel of the texture is hit (0-31).  See Footnote 2
        ctx32[addr  - yoffset] |= (((*target >> (31-xoffset))&1) << xbit);  // Draw Top Half
        ctx32[addr2 + yoffset] |= (((*(target+1)  >> xoffset)&1) << xbit);  // Draw Bottom Half
      }
    } else {
      y=colheight+1; yoffset=y*5;  // y is y +/- from vertical center, yoffset is the screen memory position of y (and is always = y*5)
    } // End Draw Wall
    
    // Draw Floor/Ceiling
    int32_t temp_x = (((box.size.h << 5) * cos_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
    int32_t temp_y = (((box.size.h << 5) * sin_lookup(player.facing + angle)) / cos_lookup(angle)); // Calculate now to save time later
    for(; y<=halfheight; y++, yoffset+=5) {         // y and yoffset continue from wall top&bottom to view edge (unless wall is taller than view edge)
      int32_t map_x = player.x + (temp_x / y);     // map_x & map_y = spot on map the screen pixel points to
      int32_t map_y = player.y + (temp_y / y);     // map_y = player.y + dist_y, dist = (height/2 * 64 * (sin if y, cos if x) / i) (/cos to un-fisheye)
      ray.hit = getmap(map_x, map_y) & 127;        // ceiling/ground of which cell is hit.  &127 shouldn't be needed since it *should* be hitting a spot without a wall
      
      if(texture[squaretype[ray.hit].ceiling].bytes_per_row==3) // If ceiling texture exists (else just show sky)
        ctx32[addr - yoffset] |= (((*( ((uint32_t*)texture[squaretype[ray.hit].ceiling].data + ((   (map_x&63)) << 1)) + ((map_y&63) >> 5)) >> (map_y&31))&1) << xbit);
      if(texture[squaretype[ray.hit].floor  ].bytes_per_row==3) // If floor texture exists (else just show abyss)
        ctx32[addr2 + yoffset] |= (((*( ((uint32_t*)texture[squaretype[ray.hit].floor ].data + ((63-(map_x&63)) << 1)) + ((map_y&63) >> 5)) >> (map_y&31))&1) << xbit);  //63- does horizontal mirroring of texture
    } // End Floor/Ceiling

    ray.hit = dist[col]; // just to stop compiler from complaining if not rendering sprites (feel free to comment out this line when sprites are enabled)
  } //End For (End RayTracing Loop)
    
  
  
  
  
  
  
  
  
  
  
  
  
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
//               // Calculate which of the 8 faces should be drawn
//               angle = atan2_lookup(player.x - current_object->x, player.y - current_object->y) + current_object->facing - (65536/4) + (32768) - (65536/16); // angle = atan2_lookup(dx, dy);
//               uint16_t rotation = ((uint16_t)angle)>>13;

//               uint16_t addr_start = ((box.origin.y + y_min) * 144) + (box.origin.x); // y location on screen + x location on screen (upper left corner)
//               uint8_t *data = current_sprite->data + (rotation * 768);

//               // Pre-calculate y_offsets (y point on texture that is hit) to save time by not calculating it every time in the inner loop
//               for(int16_t y=y_min; y<y_max; y++) {
//                 uint16_t y_offset = ((y - sprite_y_min) * object_dist) / sprite_scale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight) [0-31]
//                 y_offset_bytes[y] = y_offset>>1;
//                 y_offset_bits[y]  = (1-(y_offset&1))<<2;
//               }
//               // END pre-calculate y_offsets

//               for(int16_t x = x_min; x < x_max; x++) {
//                 if(dist[x]>=object_dist) {  // if not behind wall
//                   uint16_t addr = addr_start + x;
//                   //target = data + ((47-(((x - sprite_x_min) * object_dist) / sprite_scale)) << current_sprite->bytes_per_row); // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
//                   for(int16_t y=y_min; y<y_max; y++, addr+=144) {
//                     uint8_t fg = current_sprite->palette[((*(target + y_offset_bytes[y])) >> y_offset_bits[y])&15]; // Get color (fg: foreground color)
//                     screen[addr] = (shadowtable[((~fg)&0b11000000) + (screen[addr]&63)]&63) + shadowtable[fg];      // Plot pixel
//                   } // next y
//                 } // end display column if in front of wall
//               } // next x
              
              
              
              
              for(int16_t x = x_min; x < x_max; x++) {
                if(dist[x]>=object_dist) {  // if not behind wall
                  xaddr = (box.origin.x + x) >> 5;
                  xbit  = (box.origin.x + x) & 31;
                  //// 2017/07/12 ////xoffset = ((x - sprite_xmin) * objectdist) / spritescale; // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
                  //uint16_t addr_start = ((box.origin.y + y_min) * 144) + (box.origin.x); 
                  
                  //// 2017/07/12 ////mask   = (uint32_t*)sprite_mask[0]->addr + xoffset;  // mask = mask
                  //// 2017/07/12 ////target = (uint32_t*)sprite_image[0]->addr + xoffset; // target = sprite
                  yaddr = (box.origin.y + y_min) * 5;

                  for(int16_t y=y_min; y<y_max; y++, yaddr+=5) {
                    //graphics_draw_pixel(ctx, GPoint(box.origin.x + x, box.origin.y + y));
                    //// 2017/07/12 ////yoffset = ((y - sprite_ymin) * objectdist) / spritescale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight)
                    //// 2017/07/12 ////if(((*mask >> yoffset) & 1) == 1) {   // If mask point isn't clear, then draw point.  TODO: try removing == 1
                      ctx32[xaddr + yaddr] &= ~(1 << xbit);  // blacken bit
                      //ctx32[xaddr + yaddr] |= 1 << xbit;     // whiten bit
                      //// 2017/07/12 //// ctx32[xaddr + yaddr] |= ((*(target) >> yoffset)&1) << xbit;  // make bit white or keep it black
                      //ctx32[xaddr + yaddr] |= ((*((uint32_t*)sprite_image[0]->addr + xoffset) >> yoffset)&1) << xbit;  // make bit white or keep it black
                    //// 2017/07/12 ////}
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
  
  
  
  
  
  
  
  
//   // =======================================BEGIN BW SPRITES====================================================    
//   // Draw Sprites!
//   // Sort sprites by distance from player
//   // draw sprites in order from farthest to closest
//   // start from sprites closer than "farthest wall"
//   // sprite:
//   // x
//   // y
//   // angle
//   // distance
//   // type
//   // d


 
//   uint8_t numobjects=1;  // number of sprites
//   int32_t spritecol, objectdist;  //, xoffset, yoffset;
// //if(false)  // enable/disable drawing of sprites
//   for(uint8_t obj=0; obj<numobjects; obj++) {
//     dx = object.x - player.x;
//     dy = object.y - player.y;
//     angle = atan2_lookup(dy, dx); // angle = angle between player's x,y and sprite's x,y
//     objectdist =  (((dx^(dx>>31)) - (dx>>31)) > ((dy^(dy>>31)) - (dy>>31))) ? (dx<<16) / cos_lookup(angle) : (dy<<16) / sin_lookup(angle);
// //     objectdist = (abs32(dx)>abs32(dy)) ? (dx<<16) / cos_lookup(angle) : (dy<<16) / sin_lookup(angle);
//     angle = angle - player.facing;  // angle is now angle between center view column and object. <0=left of center, 0=center column, >0=right of center
    
//     if(cos_lookup(angle)>0) { // if object is in front of player.  note: if angle = 0 then object is straight right or left of the player
//       if(farthest>=objectdist) { // if ANY wall is further (or =) than object distance, then display it
//         spritecol = (box.size.w/2) + ((sin_lookup(angle)*box.size.w)>>16);  // column on screen of sprite center

//         int32_t objectwidth = 32;           // 32 pixels wide  TODO: maybe other sized objects?  Besides scaling?
//         int32_t objectheight = 32;//16          // 32 pixels tall
//         int32_t objectverticaloffset = 64-objectheight;//+32;//16; // normally center dot is vertically centered, + or - how far to move it.
// //         int32_t spritescale = box.size.h ;// * 20 / 10;
        
//         //objectdist = (objectdist * cos_lookup(angle)) >> 16;
        
//         int32_t spritescale = (box.size.h);// * 20 / 10;
//         int32_t spritewidth  = (spritescale * objectwidth) / objectdist;   // should be box.size.w, but wanna keep sprite h/w proportionate
//         int32_t spriteheight = (spritescale * objectheight)/ objectdist;  // note: make sure to use not-cosine adjusted distance!
// //         int32_t halfspriteheight = spriteheight/2;
//         int32_t spriteverticaloffset = (objectverticaloffset * spritescale) / objectdist; // fisheye adjustment
// //         int32_t spriteverticaloffset = ((((objectverticaloffset * spritescale) + (32*box.size.h)) << 16) / (objectdist * cos_lookup(angle))); // floor it
        
        
//         int16_t sprite_xmin = spritecol - (spritewidth/2);
//         int16_t sprite_xmax = sprite_xmin + spritewidth;  // was =spritecol+(spritewidth/2);  Changed to display whole sprite cause /2 loses info
//         if(sprite_xmax>=0 && sprite_xmin<box.size.w) {    // if any of the sprite is horizontally within view
//           int16_t xmin = sprite_xmin<0 ? 0: sprite_xmin;
//           int16_t xmax = sprite_xmax>box.size.w ? box.size.w : sprite_xmax;


// // Half through floor
// //int32_t objectheight = 16;          // 32 pixels tall
// //int32_t objectverticaloffset = 64-objectheight;//+32;//16; // normally center dot is vertically centered, + or - how far to move it.
          
// // perfectly puts 32x32 sprite on ceiling
// //int32_t objectwidth = 32;
// //int32_t objectheight = 64;
// //int32_t objectverticaloffset = 0;
// //int32_t spritescale = box.size.h;
// //int32_t spritewidth  = (spritescale * objectwidth) / objectdist;
// //int32_t spriteheight = (spritescale * objectheight) / objectdist;
// //int32_t spriteverticaloffset = ((objectverticaloffset * spritescale) << 16) / (objectdist * cos_lookup(angle)); // fisheye adjustment
// //int16_t sprite_ymax = spriteverticaloffset + ((box.size.h + spriteheight)/2);// + (((32*box.size.h) << 16) / (objectdist * cos_lookup(angle)));
// //int16_t sprite_ymin = sprite_ymax - spriteheight; // note: sprite is not cos adjusted but offset is (to keep it in place)

          
//           int16_t sprite_ymax = (box.size.h + spriteheight + spriteverticaloffset)/2;// + (((32*box.size.h) << 16) / (objectdist * cos_lookup(angle)));
//           int16_t sprite_ymin = sprite_ymax - spriteheight; // note: sprite is not cos adjusted but offset is (to keep it in place)
          
// //           int16_t sprite_ymin = halfheight + spriteverticaloffset - spriteheight; // note: sprite is not cos adjusted but offset is (to keep it in place)
// //           int16_t sprite_ymax = halfheight + spriteverticaloffset;
    

          
//           if(sprite_ymax>=0 && sprite_ymin<box.size.h) { // if any of the sprite is vertically within view
//             int16_t ymin = sprite_ymin<0 ? 0 : sprite_ymin;
//             int16_t ymax = sprite_ymax>box.size.h ? box.size.h : sprite_ymax;
// ///BEGIN DRAWING LOOPS
//             for(int16_t x = xmin; x < xmax; x++) {
//               if(dist[x]>=objectdist) {  // if not behind wall
//                 xaddr = (box.origin.x + x) >> 5;
//                 xbit  = (box.origin.x + x) & 31;
//                 xoffset = ((x - sprite_xmin) * objectdist) / spritescale; // x point hit on texture -- make sure to use the original object dist, not the cosine adjusted one
//                 mask   = (uint32_t*)sprite_mask[0]->addr + xoffset;  // mask = mask
//                 target = (uint32_t*)sprite_image[0]->addr + xoffset; // target = sprite
//                 yaddr = (box.origin.y + ymin) * 5;
                
//                 for(int16_t y=ymin; y<ymax; y++, yaddr+=5) {
//                   //graphics_draw_pixel(ctx, GPoint(box.origin.x + x, box.origin.y + y));
//                   yoffset = ((y - sprite_ymin) * objectdist) / spritescale; // y point hit on texture column (was = (objectheight*(y-sprite_ymin))/spriteheight)
//                   if(((*mask >> yoffset) & 1) == 1) {   // If mask point isn't clear, then draw point.  TODO: try removing == 1
//                     ctx32[xaddr + yaddr] &= ~(1 << xbit);  // blacken bit
//                   //ctx32[xaddr + yaddr] |= 1 << xbit;     // whiten bit
//                     ctx32[xaddr + yaddr] |= ((*(target) >> yoffset)&1) << xbit;  // make bit white or keep it black
//                   //ctx32[xaddr + yaddr] |= ((*((uint32_t*)sprite_image[0]->addr + xoffset) >> yoffset)&1) << xbit;  // make bit white or keep it black
//                   }
//                 } // next y
//               } // end display column if in front of wall
//             } // next x
// //END DRAWING LOOPS      
//           } // end display if within y bounds
//         } // end display if within x bounds
//       } // end display if within farthest
//     } // end display if not behind you
//   } // next obj
//   // =======================================END SPRITES====================================================
  
  
} // end draw 3D function
#endif