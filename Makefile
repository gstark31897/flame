a.out: main.c
	gcc main.c -O3 -lpthread -lGL -lGLEW -lglfw -lm
