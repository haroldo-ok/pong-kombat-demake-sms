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

	char score;
	actor score_actor;
	
	char last_key;
	char key_pos;
	char key_buffer[4];
} player_info;

player_info player1;
player_info player2;

actor ball;
actor finish_him;

char is_stage_fatality;

struct ball_ctl {
	signed char spd_x, spd_y;
} ball_ctl;

char frames_elapsed;


// Forward declarations
extern void update_score(player_info *ply, int delta);


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
	init_actor(&ply->score_actor, x + (x < 128 ? 48 : -48), PLAYER_TOP + 8, 1, 1, 172, 1);
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
		
		if (!finish_him.active && has_key_sequence(ply, "BBBB")) {
			init_actor(&ply->atk, ply->act.x, ply->act.y + 8, 2, 1, 4, 4);
			memset(ply->key_buffer, 0, 4);
		}
		
		if (finish_him.active && has_key_sequence(ply, "DDDD")) is_stage_fatality = 1;
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
	if (!ball.active && !finish_him.active) {
		init_actor(&ball, (SCREEN_W >> 1) - 8, (SCREEN_H >> 1) - 8, 2, 1, 48 + ((rand() % 4) << 2), 1);
		ball_ctl.spd_x = rand() & 1 ? 1 : -1;
		ball_ctl.spd_y = rand() & 1 ? 1 : -1;
	}
	
	ball.x += ball_ctl.spd_x;
	ball.y += ball_ctl.spd_y;
	
	if (ball.x < 0 || ball.x > SCREEN_W - 8) {
		ball.active = 0;
		update_score(ball.x < 0 ? &player2 : &player1, 1);
	}
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

void handle_projectile(player_info *ply, player_info *enm, int speed) {
	if (!ply->atk.active) return;
	
	ply->atk.x += speed;
	if (ply->atk.x < 0 || ply->atk.x > SCREEN_W - 16) {
		ply->atk.active = 0;
		return;
	}

	if (ply->atk.x > enm->act.x - 8 && ply->atk.x < enm->act.x + 8 &&
		ply->atk.y > enm->act.y - 14 && ply->atk.y < enm->act.y + 30) {
		ply->atk.active = 0;
		update_score(ply, 1);
	}
}

void handle_projectiles() {
	handle_projectile(&player1, &player2, 3);
	handle_projectile(&player2, &player1, -3);
}

void draw_projectiles() {
	draw_actor(&player1.atk);
	draw_actor(&player2.atk);
}

void update_score(player_info *ply, int delta) {
	ply->score += delta;
	ply->score_actor.base_tile = 172 + (ply->score << 1);
}

void draw_scores() {
	draw_actor(&player1.score_actor);
	draw_actor(&player2.score_actor);
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

	init_actor(&finish_him, (SCREEN_W - 48) >> 1, PLAYER_TOP + 48, 6, 1, 116, 1);
	finish_him.active = 0;

	is_stage_fatality = 0;
	while (!is_stage_fatality) {	
		handle_players_input();
		handle_ball();
		handle_projectiles();
		
		if (player1.score == 9 || player2.score == 9) {
			finish_him.active = 1;
			ball.active = 0;
			player1.atk.active = 0;
			player2.atk.active = 0;
		}
		
		SMS_initSprites();

		draw_players();
		draw_actor(&ball);
		draw_projectiles();
		draw_scores();
		draw_actor(&finish_him);
		
		SMS_finalizeSprites();
		SMS_waitForVBlank();
		SMS_copySpritestoSAT();
	}
}

void title_sequence() {
	SMS_displayOff();

	clear_sprites();
	SMS_loadPSGaidencompressedTiles(title_tiles_psgcompr, 0);
	SMS_loadTileMap(0, 0, title_tilemap_bin, title_tilemap_bin_size);
	SMS_loadBGPalette(title_palette_bin);
	
	SMS_displayOn();

	wait_button_release();
	wait_button_press();
	wait_button_release();
}

void fatality_sequence() {
	SMS_displayOff();

	clear_sprites();
	SMS_loadPSGaidencompressedTiles(the_pit_fatality_tiles_psgcompr, 0);
	SMS_loadTileMap(0, 0, the_pit_fatality_tilemap_bin, the_pit_fatality_tilemap_bin_size);
	SMS_loadBGPalette(the_pit_fatality_palette_bin);
	
	SMS_displayOn();

	wait_frames(180);
}

void main() {	
	while (1) {
		title_sequence();
		gameplay_loop();
		fatality_sequence();
	}
}

SMS_EMBED_SEGA_ROM_HEADER(9999,0); // code 9999 hopefully free, here this means 'homebrew'
SMS_EMBED_SDSC_HEADER(0,2, 2022, 8, 12, "Haroldo-OK\\2022", "Pong Kombat Demake",
  "MS-DOS \"Pong Kombat\" demade for the Sega Master System.\n"
  "Originally made for the \"So Bad it's Good' Jam 2022\" - https://itch.io/jam/sbigjam2022");

