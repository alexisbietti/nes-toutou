toutou.nes: crt0.o toutou.o nrom_128_horz.cfg
	ld65 -C nrom_128_horz.cfg -o toutou.nes crt0.o toutou.o runtime.lib

toutou.s: toutou.c
	cc65 -Oi toutou.c --add-source

toutou.o: toutou.s
	ca65 toutou.s

crt0.o: toutou.chr
	ca65 crt0.s

clean:
	rm toutou.nes crt0.o toutou.o toutou.s

start: toutou.nes
	fceux toutou.nes
