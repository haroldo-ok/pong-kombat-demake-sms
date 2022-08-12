PRJNAME := pong_kombat
OBJS := data.rel actor.rel pong_kombat.rel

all: $(PRJNAME).sms

data.c: data/* data/sprites_tiles.psgcompr data/the_pit_tiles.psgcompr data/the_pit_fatality_tiles.psgcompr
	folder2c data data
	
data/sprites_tiles.psgcompr: data/img/sprites.png
	BMP2Tile.exe data/img/sprites.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/sprites_tiles.psgcompr -savepalette data/sprites_palette.bin
	
data/the_pit_tiles.psgcompr: data/img/the_pit.png
	BMP2Tile.exe data/img/the_pit.png -palsms -fullpalette -savetiles data/the_pit_tiles.psgcompr -savetilemap data/the_pit_tilemap.bin -savepalette data/the_pit_palette.bin

data/the_pit_fatality_tiles.psgcompr: data/img/the_pit_fatality.png
	BMP2Tile.exe data/img/the_pit_fatality.png -palsms -fullpalette -savetiles data/the_pit_fatality_tiles.psgcompr -savetilemap data/the_pit_fatality_tilemap.bin -savepalette data/the_pit_fatality_palette.bin

data/%.path: data/path/%.spline.json
	node tool/convert_splines.js $< $@

data/%.bin: data/map/%.tmx
	node tool/convert_map.js $< $@
	
%.vgm: %.wav
	psgtalk -r 512 -u 1 -m vgm $<

%.rel : %.c
	sdcc -c -mz80 --peep-file lib/peep-rules.txt $<

$(PRJNAME).sms: $(OBJS)
	sdcc -o $(PRJNAME).ihx -mz80 --no-std-crt0 --data-loc 0xC000 lib/crt0_sms.rel $(OBJS) SMSlib.lib lib/PSGlib.rel
	ihx2sms $(PRJNAME).ihx $(PRJNAME).sms	

clean:
	rm *.sms *.sav *.asm *.sym *.rel *.noi *.map *.lst *.lk *.ihx data.*
