fb3-2: fb3_2.l fb3_2.y fb3_2.h
	bison -d fb3_2.y
	flex  -o fb3_2.lex.c fb3_2.l
	cc -o $@ fb3_2.tab.c fb3_2.lex.c fb3_2funcs.c -lfl -lm
