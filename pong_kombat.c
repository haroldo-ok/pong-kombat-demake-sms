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

typedef struct player_info {
	actor act;	
	actor atk;
	
	char last_key;
	char key_pos;
	char key_buffer[4];
} player_info;

player_info player1;
player_info player2;

actor ball;

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

void init_player(player_info *ply, int x) {
	memset(ply, 0, sizeof(player_info));
	init_actor(&ply->act, x, PLAYER_BOTTOM - 16, 1, 3, 2, 1);
}

char has_key_sequence(player_info *ply, char *sequence) {
	char pos = ply->key_pos;
	char *ch = sequence;
	
	while (*ch) {
		if (*ch != ply->key_buffer[pos]) {
			return 0;
		}
		ch++;
		pos = (pos + 1) & 0x03;
	}
	
	return 1;
}

void handle_player_input(player_info *ply, unsigned int joy, unsigned int upKey, unsigned int downKey, unsigned int fireKey) {
	char cur_key = 0;
	
	if (joy & upKey) {
		if (ply->act.y > PLAYER_TOP) ply->act.y -= PLAYER_SPEED;
		cur_key = 'U';
	} else if (joy & downKey) {
		if (ply->act.y < PLAYER_BOTTOM) ply->act.y += PLAYER_SPEED;
		cur_key = 'D';
	} else if (joy & fireKey) {
		cur_key = 'B';
	}
	
	if (cur_key && !ply->last_key) {
		ply->key_buffer[ply->key_pos] = cur_key;
		ply->key_pos = (ply->key_pos + 1) & 0x03;
		
		if (has_key_sequence(ply, "BBBB")) {
			init_actor(&ply->atk, ply->act.x, ply->act.y + 8, 2, 1, 4, 4);
			memset(ply->key_buffer, 0, 4);
		}
	}
	
	ply->last_key = cur_key;
}

void handle_player_ai(player_info *ply) {
	int target_y = ply->act.y - 8;
	
	if (target_y > ball.y) {
		ply->act.y -= PLAYER_SPEED;
	} else if (target_y < ball.y) {
		ply->act.y += PLAYER_SPEED;
	}

	if (ply->act.y < PLAYER_TOP) {
		ply->act.y = PLAYER_TOP;
	} else if (ply->act.y > PLAYER_BOTTOM) {
		ply->act.y = PLAYER_BOTTOM;
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
	draw_actor(&player1.act);
	draw_actor(&player2.act);
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
	
	if (ball.x > player1.act.x && ball.x < player1.act.x + 8 &&
		ball.y > player1.act.y - 16 && ball.y < player1.act.y + 32) {
		ball_ctl.spd_x = abs(ball_ctl.spd_x);
		calculate_ball_deflection(&player1.act);
	}

	if (ball.x > player2.act.x - 16 && ball.x < player2.act.x - 8 &&
		ball.y > player2.act.y - 16 && ball.y < player2.act.y + 32) {
		ball_ctl.spd_x = -abs(ball_ctl.spd_x);
		calculate_ball_deflection(&player2.act);
	}
}

void handle_projectile(player_info *ply, int speed) {
	if (!ply->atk.active) return;
	
	ply->atk.x += speed;
	if (ply->atk.x < 0 || ply->atk.x > SCREEN_W - 16) ply->atk.active = 0;
}

void handle_projectiles() {
	handle_projectile(&player1, 3);
	handle_projectile(&player2, -3);
}

void draw_projectiles() {
	draw_actor(&player1.atk);
	draw_actor(&player2.atk);
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
	
	init_player(&player1, 16);
	init_player(&player2, 256 - 24);

	init_ball();

	while (1) {	
		handle_players_input();
		handle_ball();
		handle_projectiles();
		
		SMS_initSprites();

		draw_players();
		draw_actor(&ball);
		draw_projectiles();
		
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

