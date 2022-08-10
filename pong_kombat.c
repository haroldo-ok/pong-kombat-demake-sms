#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lib/SMSlib.h"
#include "lib/PSGlib.h"
#include "actor.h"
#include "data.h"

#define PLAYER_TOP (4)
#define PLAYER_LEFT (0)
#define PLAYER_RIGHT (256)
#define PLAYER_BOTTOM (SCREEN_H - 32 - 4)
#define PLAYER_SPEED (3)

#define BACKGROUND_BASE_TILE (192)

actor player1;
actor player2;
actor ball;
actor projectile;

struct ball_ctl {
	signed char spd_x, spd_y;
} ball_ctl;

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

void handle_player_input(actor *act, unsigned int joy, unsigned int upKey, unsigned int downKey, unsigned int fireKey) {
	if (joy & upKey) {
		if (act->y > PLAYER_TOP) act->y -= PLAYER_SPEED;
	} else if (joy & downKey) {
		if (act->y < PLAYER_BOTTOM) act->y += PLAYER_SPEED;
	}
}

void handle_player_ai(actor *act) {
	int target_y = act->y - 8;
	
	if (target_y > ball.y) {
		act->y -= PLAYER_SPEED;
	} else if (target_y < ball.y) {
		act->y += PLAYER_SPEED;
	}

	if (act->y < PLAYER_TOP) {
		act->y = PLAYER_TOP;
	} else if (act->y > PLAYER_BOTTOM) {
		act->y = PLAYER_BOTTOM;
	}
}

void handle_players_input() {
	static unsigned int joy;	
	joy = SMS_getKeysStatus();
	
	handle_player_input(&player1, joy, PORT_A_KEY_UP, PORT_A_KEY_DOWN, PORT_A_KEY_1 | PORT_A_KEY_2);
	handle_player_ai(&player2);
	//handle_player_input(&player2, joy, PORT_B_KEY_UP, PORT_B_KEY_DOWN, PORT_B_KEY_1 | PORT_B_KEY_2);
}

void draw_players() {
	draw_actor(&player1);
	draw_actor(&player2);
}

void init_ball() {
	ball.active = 0;
}

void calculate_ball_deflection(actor *ply) {
	int delta_y = (ply->y + 16) - (ball.y + 8);
	
	ball_ctl.spd_y -= delta_y / 6;
	if (ball_ctl.spd_y > 3) {
		ball_ctl.spd_y = 3;
	} else if (ball_ctl.spd_y < -3) {
		ball_ctl.spd_y = -3;
	}
}

void handle_ball() {
	if (!ball.active) {
		init_actor(&ball, (SCREEN_W >> 1) - 8, (SCREEN_H >> 1) - 8, 2, 1, 48 + ((rand() % 4) << 2), 1);
		ball_ctl.spd_x = rand() & 1 ? 1 : -1;
		ball_ctl.spd_y = rand() & 1 ? 1 : -1;
	}
	
	ball.x += ball_ctl.spd_x;
	ball.y += ball_ctl.spd_y;
	
	if (ball.x < 0 || ball.x > SCREEN_W - 8) ball.active = 0;	
	if (ball.y < 0 || ball.y > SCREEN_H - 16) ball_ctl.spd_y = -ball_ctl.spd_y;
	
	if (ball.x > player1.x && ball.x < player1.x + 8 &&
		ball.y > player1.y - 16 && ball.y < player1.y + 32) {
		ball_ctl.spd_x = abs(ball_ctl.spd_x);
		calculate_ball_deflection(&player1);
	}

	if (ball.x > player2.x - 16 && ball.x < player2.x - 8 &&
		ball.y > player2.y - 16 && ball.y < player2.y + 32) {
		ball_ctl.spd_x = -abs(ball_ctl.spd_x);
		calculate_ball_deflection(&player2);
	}
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
	
	init_actor(&player1, 16, PLAYER_BOTTOM - 16, 1, 3, 2, 1);
	init_actor(&player2, 256 - 24, PLAYER_BOTTOM - 16, 1, 3, 2, 1);

	init_ball();

	init_actor(&projectile, 64, PLAYER_TOP + 16, 2, 1, 4, 4);

	while (1) {	
		handle_players_input();
		handle_ball();
		
		SMS_initSprites();

		draw_players();
		draw_actor(&ball);
		draw_actor(&projectile);
		
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

