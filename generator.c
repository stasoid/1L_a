// Generates 1L_a program that prints given string. https://esolangs.org/wiki/1L_a
// Usage: 1Lgen.exe "Hello, World!"
#define _CRT_SECURE_NO_WARNINGS  // strcat -> strcat_s
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define byte char
void split(char* str, char sep, char*** result, int* count);
char* join(char** ary, int count, char* sep);
void error(char* str) { puts(str); exit(1); }

// tape contents, tape index, IP y coord
typedef struct StartState
{
	byte tape[32]; // each byte represents one bit (0 or 1)
	int index; // index of current cell in tape
	int y; // IP y coord
	// note: start x is always 0
	//       start dir is alway RIGHT
} StartState;

// intentional duplication because I don't want to write i->x, i->dir, etc. in execute()
byte tape[32]; // each byte represents one bit (0 or 1)
int index; // index of current cell in tape
int x, y; // IP location
enum { LEFT, RIGHT, UP, DOWN } dir; // IP direction

char* code;
int width, height; // width, height of code, they equal to inner_width + 2, inner_height + 2

// represents one bit of output (0 or 1)
// the program is allowed to print only one bit to simplify code
// -1 means no output
byte output;

void init_global_interpreter_state(const StartState* state, char* inner_code, int inner_width, int inner_height);
bool execute(bool log);
bool next_permutation(char* str, int len);



/*                    XXXX
              _*      X_*Y  inner_width == 2
_*_____*  ->  __  ->  X__Y  inner_height == 4
              __      X__Y  start_y == 4 (the space)
              _*       _*Y  Y acts like a wall only if nothing was printed, otherwise it is space (see allowed_exits)
                      XXXX  _ is space
*/
/*
result is in ::code, ::width, ::height
*/
bool generate(StartState* start_state, int inner_height, byte bit)
{
	int max_inner_width = 9;
	char* inner_code = calloc(max_inner_width * inner_height, 1);
	for (int inner_width = 1; inner_width <= max_inner_width; inner_width++)
	{
		int code_len = inner_width * inner_height;
		// when n_asterisks == code_len entire code consists of asterisks, IP cannot go through, invalid code
		// when n_asterisks == code_len-1 IP can go only straight (assuming inner_width == 1), invalid code
		// so max n_asterisks is code_len-2, it is less than 5 only when inner_width == 1
		int max_asterisks = min(5, code_len - 2);
		for (int n_asterisks = 0; n_asterisks <= max_asterisks; n_asterisks++)
		{
			// lexicographically smallest permutation is the one with all the asterisks at the end, eg. _____***
			memset(inner_code, ' ', code_len);
			memset(inner_code + code_len - n_asterisks, '*', n_asterisks);
			
			while(1) // for all permutations of code
			{
				init_global_interpreter_state(start_state, inner_code, inner_width, inner_height);
				
				if (execute(false) && output == bit
					// without these conditions search for the second cell fails
					&& y != 1 && y != inner_height)
				{
					code[width * y + x] = ' '; // make exit hole
					free(inner_code);
					return true; // result is in ::code, ::width, ::height
				}

				if (!next_permutation(inner_code, code_len)) break;
			}
		}
	}
	free(inner_code);
	return false;
}

// convert code to user-friendly multiline zero-terminated string.
// inserts newline after every run of `width` chars
char* code2str(char* code, int width, int height)
{
	char* str = calloc((width + 1) * height, 1);
	for (int y = 0; y < height; y++)
	{
		memcpy(str + (width + 1) * y, code + width * y, width);
		str[(width + 1) * y + width] = '\n';
	}
	str[(width + 1) * height - 1] = 0; // replace last '\n' with '\0'
	return str;
}

/*
   _
   _
   _
   _
   _   <- start_y
   *
*/
char* get_program_start(int inner_height, int start_y)
{
	int height = inner_height + 2;
	char* code = malloc(height);
	memset(code, ' ', height);
	code[start_y + 1] = '*';
	return code2str(code, 1, height); // leaks `code`, not a problem
}

/*
    _*
    _*
    _*
    _*
    _*
    **
*/
char* get_program_end(int inner_height)
{
	int height = inner_height + 2;
	char* code = malloc(height * 2);
	for (int i = 0; i < height; i++)
	{
		code[2 * i] = ' ';
		code[2 * i + 1] = '*';
	}
	code[2 * (height - 1)] = '*';
	return code2str(code, 2, height); // leaks `code`, not a problem
}

// gap: -1|0|1
/*
   buffer       code2d     buffer
   xxxxxxxx     yyyy       xxxxxxxxyyyy
   xxxxxxxx     yyyy	   xxxxxxxxyyyy
   xxxxxxxx     yyyy	   xxxxxxxxyyyy
   xxxxxxxx  +  yyyy   =   xxxxxxxxyyyy
   xxxxxxxx     yyyy	   xxxxxxxxyyyy
   xxxxxxxx     yyyy	   xxxxxxxxyyyy
*/
void append_2dcode(char** buffer, char* code2d, int gap)
{
	char** lines, **lines2;
	int count, count2; // number of lines
	split(*buffer, '\n', &lines, &count);
	split(code2d, '\n', &lines2, &count2);
	if (count != count2) error("internal error in append_2dcode()");
	
	for (int i = 0; i < count; i++)
	{
		lines[i] = realloc(lines[i], strlen(lines[i]) + gap + strlen(lines2[i]) + 1); // +1: '\0'
		if (gap == 1) strcat(lines[i], " ");
		strcat(lines[i], lines2[i] + (gap == -1 ? 1 : 0));
	}
	
	free(*buffer);
	*buffer = join(lines, count, "\n");
	
	for (int i = 0; i < count; i++)
		free(lines[i]), free(lines2[i]);
	free(lines), free(lines2);
}

int main(int argc, char* argv[])
{
	if (argc == 1) error("Example usage: 1Lgen.exe \"Hello, World!\"");
	unsigned char* str = argv[1];

	int inner_height = 4;
	StartState start_state = { .tape = {0}, .index = 2, .y = inner_height };

	char* result = get_program_start(inner_height, start_state.y);

	int len = strlen(str) * 8;
	for (int i = 0; i < len; i++)
	{
		byte bit = (str[i / 8] >> (7 - i % 8)) & 1;
		bool b = generate(&start_state, inner_height, bit);
		
		if (b) append_2dcode(&result, code2str(code, width, height), i == 0 ? 0 : -1); // -!!i
		else error("failed to generate");

		memcpy(start_state.tape, tape, sizeof tape);
		start_state.index = index;
		start_state.y = y;
	}

	append_2dcode(&result, get_program_end(inner_height), 0);
	puts(result);
}


#define swap(a,b) {char c=a; a=b; b=c;}
void reverse(char* s, int n)
{
	for (int i = 0; i < n/2; i++) swap(s[i], s[n-i-1]);
}

// g<generate all numbers with n bits set>
// https://www.techiedelight.com/std_next_permutation-overview-implementation/
// Function to rearrange the specified string as lexicographically greater
// permutation. It returns false if the string cannot be rearranged as a
// lexicographically greater permutation; otherwise, it returns true.
// n == strlen(s);    must be n >= 2
bool next_permutation(char* s, int n)
{
	// Find the largest index `i` such that `s[i-1]` is less than `s[i]`
	int i = n - 1;
	while (s[i - 1] >= s[i])
	{
		// if `i` is the first index of the string, we are already at the last
		// possible permutation (string is sorted in reverse order)
		if (--i == 0) return false;
	}

	// If we reach here, substring `s[i…n-1]` is sorted in reverse order.
	// Find the highest index `j` to the right of index `i` such that `s[j] > s[i-1]`.
	int j = n - 1;
	while (j > i && s[j] <= s[i - 1]) j--;

	// Swap character at index `i-1` with index `j`
	swap(s[i - 1], s[j]);

	// Reverse substring `s[i…n-1]`
	reverse(s + i, n - i);
	return true;
}

void dir2dxdy(int dir, int* dx, int* dy);
int turn(int dir, int turn);

void init_global_interpreter_state(const StartState* state, char* inner_code, int inner_width, int inner_height)
{
	memcpy(tape, state->tape, sizeof tape);
	index = state->index;
	x = 0;
	y = state->y;
	dir = RIGHT;

	// init code/width/height
	width = inner_width + 2;
	height = inner_height + 2;
	int len = width * height;

	code = realloc(code, len);
	memset(code, ' ', len);

	memset(code, '*', width); // top edge
	memset(code + width * (height - 1), '*', width); // bottom
	for (int y = 1; y < height-1; y++)
	{
		code[width * y] = '*'; // left edge
		code[width * y + width - 1] = '*'; // right
		memcpy(code + width * y + 1, inner_code + inner_width * (y - 1), inner_width);
	}

	code[y * width] = ' '; // entry point; y is ::y

	output = -1;
}

// returns true if printed one bit and successfully "exited" (reached right edge at correct place, see allowed_exits)
// returns false otherwise
bool execute(bool log)
{
	if (height > 8) { puts("internal error"); exit(1); }
	int allowed_exits[8] = { 0 };
	for (int i = 1; i <= height - 2; i++) allowed_exits[i] = 1;

	int max_steps = 100;
	int n = 0;
	while (1)
	{
		char c = code[width * y + x];
		if (log) printf("%d,%d\n", x, y);

		if (c == ' ')
		{
			switch (dir)
			{
			case LEFT:
				index--;
				if (index < 0 || index >= sizeof tape) return false;
				tape[index] = !tape[index];
				if (index == 0)
				{
					if (tape[1])
					{
						// the program is allowed to print only one bit to simplify code
						if (output != -1) return false;
						output = tape[2]; // output one bit
					}
					else
						return false; // no input
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

			if (index >= sizeof tape) return false;
			dir = turn(dir, tape[index] ? RIGHT : LEFT);
		}

		int dx, dy;
		dir2dxdy(dir, &dx, &dy);
		x += dx;
		y += dy;

		// the only way out is through entry point, so !xyok(x,y) is reduced to x < 0
		if (x < 0) return false;

		if (x == width - 1) // reached right edge
		{
			// if at right edge and already printed smth and allowed to exit here then return success
			if (output != -1 && allowed_exits[y]) return true;
			allowed_exits[y] = 0;
			int i;
			for (i = 0; i < 8; i++) if (allowed_exits[i]) break;
			if (i == 8) return false; // no more allowed exits left
		}

		if (n++ >= max_steps) return false;
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
	return 0; // no warning
}
