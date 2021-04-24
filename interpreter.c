// Simplified non-conforming 1L_a interpreter (https://esolangs.org/wiki/1L_a).
// Differencies from Graue's interpreter (https://github.com/graue/esofiles/blob/master/1l/impl/1l_a.c) :
// - bitwise output (outputs only 01)
// - no input
// This interpreter was created to help build generator.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define byte char
char* readfile(char* fname, int* _size);
void split(char* str, char sep, char*** result, int* count);
void error(char* str) { puts(str); exit(1); }

byte tape[100]={0}; // each byte represents one bit (0 or 1)
int index=2; // index of current cell in tape
int x=0,y=0; // IP location
enum{LEFT,RIGHT,UP,DOWN} dir = DOWN; // IP direction
char** lines; // program code
int n_lines;

int xyok(int x, int y) { return y >= 0 && y < n_lines && x >= 0 && x < (int)strlen(lines[y]); }
void dir2dxdy(int dir, int* dx, int* dy);
int turn(int dir, int turn);

void main(int argc, char* argv[])
{
	if (argc == 1) error("specify filename");
	char* str = readfile(argv[1], 0);
	if(!str) error("cannot read file");
	// \r chars may cause bugs because they are treated as walls
	if (strchr(str, '\r')) error("file must have UNIX line endings");
	split(str, '\n', &lines, &n_lines);

	while(1)
	{
		char c = xyok(x, y) ? lines[y][x] : ' ';

		if(c == ' ')
		{
			switch (dir)
			{
			case LEFT:
				index--;
				if (index < 0) error("index < 0");
				if (index >= sizeof tape) error("index is too large");
				tape[index] = !tape[index];
				if (index == 0)
				{
					if (tape[1]) 
						printf("%d", tape[2]); // bitwise output (nonstandard)
					else
						error("input is not implemented");
				}
				break;
			case UP:
				index++;
				break;
			// RIGHT and DOWN are nops
			}
		}
		else
		{
			// one step back
			int dx, dy;
			dir2dxdy(dir, &dx, &dy);
			x -= dx;
			y -= dy;

			if (index >= sizeof tape) error("index is too large");
			dir = turn(dir, tape[index] ? RIGHT : LEFT);
		}

		int dx, dy;
		dir2dxdy(dir, &dx, &dy);
		x += dx;
		y += dy;
		if (!xyok(x,y)) exit(0);
	}
}

void dir2dxdy(int dir, int* dx, int* dy)
{
	switch (dir)
	{
	case LEFT:
		*dx = -1;
		*dy = 0;
		break;
	case RIGHT:
		*dx = 1;
		*dy = 0;
		break;
	case UP:
		*dx = 0;
		*dy = -1;
		break;
	case DOWN:
		*dx = 0;
		*dy = 1;
		break;
	}
}

int turn(int dir, int turn)
{
	switch (dir)
	{
	case LEFT:
		return turn == LEFT ? DOWN : UP;
	case RIGHT:
		return turn == LEFT ? UP : DOWN;
	case UP:
		return turn == LEFT ? LEFT : RIGHT;
	case DOWN:
		return turn == LEFT ? RIGHT : LEFT;
	}
}