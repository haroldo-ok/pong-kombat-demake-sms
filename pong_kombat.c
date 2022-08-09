#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "data.h"

#define PLAYER_TOP (0)
#define PLAYER_LEFT (0)
#define PLAYER_RIGHT (256 - 16)
#define PLAYER_BOTTOM (SCREEN_H - 16)
#define PLAYER_SPEED (3)

#define BACKGROUND_BASE_TILE (192)

actor player;
char frames_elapsed;

void load_standard_palettes() {
	SMS_loadBGPalette(the_pit_palette_bin);
	SMS_loadSpritePalette(sprites_palette_bin);
	SMS_setSpritePaletteColor(0, 0);
}

void wait_button_press() {
	do {
		SMS_waitForVBlank();
	} while (!(SMS_getKeysStatus() & (PORT_A_KEY_1 | PORT_A_KEY_2)));
}

void wait_button_release() {
	do {
		SMS_waitForVBlank();
	} while (SMS_getKeysStatus() & (PORT_A_KEY_1 | PORT_A_KEY_2));
}

void handle_player_input() {
	static unsigned char joy;	
	joy = SMS_getKeysStatus();

	if (joy & PORT_A_KEY_LEFT) {
		if (player.x > PLAYER_LEFT) player.x -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_RIGHT) {
		if (player.x < PLAYER_RIGHT) player.x += PLAYER_SPEED;
	}

	if (joy & PORT_A_KEY_UP) {
		if (player.y > PLAYER_TOP) player.y -= PLAYER_SPEED;
	} else if (joy & PORT_A_KEY_DOWN) {
		if (player.y < PLAYER_BOTTOM) player.y += PLAYER_SPEED;
	}

}

void draw_player() {
	draw_actor(&player);
}

void clear_tilemap() {
	SMS_setNextTileatXY(0, 0);
	for (int i = (SCREEN_CHAR_W * SCROLL_CHAR_H); i; i--) {
		SMS_setTile(0);
	}
}

void draw_background() {
	SMS_loadPSGaidencompressedTiles(the_pit_tiles_psgcompr, BACKGROUND_BASE_TILE);	
	SMS_loadBGPalette(the_pit_palette_bin);
	SMS_setSpritePaletteColor(0, 0);
	SMS_setBGPaletteColor(0, 0);

	clear_tilemap();
	SMS_setBGScrollX(0);
	SMS_setBGScrollY(0);

	// Draws the background.
	unsigned int *t = the_pit_tilemap_bin;
	for (char y = 0; y != SCREEN_CHAR_H; y++) {
		SMS_setNextTileatXY(0, y);
		for (char x = 0; x != SCREEN_CHAR_W; x++) {
			SMS_setTile(*t + BACKGROUND_BASE_TILE);
			t++;
		}
	}

}

void interrupt_handler() {
	PSGFrame();
	PSGSFXFrame();
	frames_elapsed++;
}

void gameplay_loop() {	
	SMS_useFirstHalfTilesforSprites(1);
	SMS_setSpriteMode(SPRITEMODE_TALL);
	SMS_VDPturnOnFeature(VDPFEATURE_HIDEFIRSTCOL);

	SMS_displayOff();
	SMS_loadPSGaidencompressedTiles(sprites_tiles_psgcompr, 0);
	load_standard_palettes();
	
	draw_background();
	
	SMS_setLineInterruptHandler(&interrupt_handler);
	SMS_setLineCounter(180);
	SMS_enableLineInterrupt();

	SMS_displayOn();
	
	init_actor(&player, 116, PLAYER_BOTTOM - 16, 1, 3, 2, 1);
	player.animation_delay = 20;
	
	while (1) {	
		handle_player_input();
		
		SMS_initSprites();

		draw_player();
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
	}
}

void main() {	
	while (1) {
		gameplay_loop();
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,1, 2022, 8, 9, "Haroldo-OK\\2022", "Pong Kombat Demake",
  "MS-DOS \"Pong Kombat\" demade for the Sega Master System.\n"
  "Originally made for the \"So Bad it's Good' Jam 2022\" - https://itch.io/jam/sbigjam2022");
