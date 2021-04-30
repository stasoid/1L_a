// https://stackoverflow.com/questions/35205423/list-operations-complexity
using System;
using System.Collections.Generic;
using System.Linq;
using System.Diagnostics;
using System.Threading;
//using System.Text;
//using static System.Math;
using static Program.Dir;

class Program
{
	// tape contents, tape index, IP y coord
	// portion of the state of 1L program that is needed to create next cell
	class ProgState
	{
		public int[] tape = new int[32]; // each element represents one bit (0 or 1)
		public int index; // index of current element in tape
		public int y; // IP y coord
					  // note: start x is always 0
					  //       start dir is alway RIGHT
		public bool Equals(ProgState s) // no need to override
		{
			return y == s.y && index == s.index && tape.SequenceEqual(s.tape);
		}
	}
	// cell that prints one bit
	class Cell
	{
		public char[] code;
		public int width, height;
		public ProgState exit_state;

		public Cell prev; // cell that prints previous bit
	};

	int[] tape; // each element represents one bit (0 or 1)
	int index; // index of current element in tape
	int x, y; // IP location
	public enum Dir { LEFT, RIGHT, UP, DOWN };
	Dir dir; // IP direction

	char[] code;
	int width, height; // width, height of code, they equal to inner_width + 2, inner_height + 2

	// represents one bit of output (0 or 1)
	// cell is allowed to print only one bit to simplify code
	// -1 means no output
	int output;

	// golfing vs speed of generation
	// careful here: increasing only max_steps can make things worse (see generator2.cs.old2), need to increase max_cells too
	int max_steps = 500; //100
	// how many candidates for each bit to keep
	// this can be increased independently of max_steps
	int max_cells = 50; //10
	// HW is created with max_steps = 500, max_cells = 50. 100/10 produces larger program, 1000/100 produces program of the same size.
	// 500/50 runs for 16 minutes, 100/10 - for 2 minutes.

	// another parameter that affects resulting size is start_state.y, see below (HW is created with start_y = 3)
	
	// increasing max_inner_width=9 and max_asterisks=5 doesn't affect resulting size, only makes generator slower
	
	// Ideas:
	// - search for cells that print 2 bits
	// - don't exit the cell at first opportunity, prefer exits closer to the middle (y=2,3) because they tend to produce shorter programs
	// - take arbitrary x0 and x1 in resulting program and try to replace what is inbetween with shorter code that does the same
	//   (golf cell borders more cleverly than just removing columns)

	class Statistics
    {
		public int max_steps; // how many steps are actually taken in cells for which execute returns true
		public int min_cells = 1000000, max_cells; // how many candidates for each bit are actually created
		public int min_inner_width = 100, max_inner_width;
		public int min_asterisks = 100, max_asterisks;
    }
	Statistics stat = new Statistics();

	static void Main(string[] args)
    {
		Program p = new Program();
		p.main(args);
	}
	void main(string[] args)
	{
		Stopwatch stopwatch = new Stopwatch();
		stopwatch.Start();

		int inner_height = 4;
		ProgState start_state = new ProgState { index = 2, y = 3 }; // y = 1..inner_height, y=1 always fails
		Cell root = new Cell { height = inner_height + 2, exit_state = start_state }; // dummy root cell
		Cell[] cells = { root };
		string str = "Hello, World!";
		//bool separate_cells = true;
		int len = str.Length * 8;

		for (int i = 0; i < len; i++)
		{
			int bit = (str[i / 8] >> (7 - i % 8)) & 1;
			cells = find_next(cells, bit);
			if (cells == null) error("failed to generate");

			stat.min_cells = min(cells.Length, stat.min_cells);
			stat.max_cells = max(cells.Length, stat.max_cells);

			// to speed things up, keep only first max_cells cells with shortest total width
			Array.Sort(cells, (cell1, cell2) => total_width(cell1) - total_width(cell2));
			Array.Resize(ref cells, min(cells.Length, max_cells));

			// show progress
			if (i % 8 == 7) // if finished one char
				Console.Error.Write(str[i / 8]); // print it
		}
		Console.Error.WriteLine();

		Cell[] prog = new Cell[len];
		Cell cell = cells[0];
		for (int i = 0; i < len; i++, cell = cell.prev) prog[len - i - 1] = cell;
		string result = get_program_start(inner_height, start_state.y);
		string result_with_gaps = result;
		for (int i = 0; i < len; i++)
		{
			cell = prog[i];
			string code_ = code2str(cell.code, cell.width, cell.height);
			int gap = /*separate_cells ? 1 :*/ (i == 0 ? 0 : -1);
			result = append_2dcode(result, code_, gap);
			result_with_gaps = append_2dcode(result_with_gaps, code_, 1);
		}
        Console.WriteLine(result);            //  <---  !!!
		Console.WriteLine(result_with_gaps);

		stopwatch.Stop();
		print_time(stopwatch.ElapsedMilliseconds);
	}
	void print_time(long ms)
	{
		int one_minute = 60 * 1000;
		if (ms < one_minute)
			Console.WriteLine("Time: {0} sec", ms / 1000);
		else
		{
			long min = ms / one_minute;
			long sec = (ms % one_minute) / 1000;
			Console.WriteLine("Time: {0} min {1} sec", min, sec);
		}
	}
	char[] slice(char[] ary, int start, int count)
	{
		char[] ret = new char[count];
		Array.Copy(ary, start, ret, 0, count);
		return ret;
	}
	// convert code to user-friendly multiline string.
	// inserts newline after every run of `width` chars
	string code2str(char[] code, int width, int height)
	{
		string ret = "";
		for (int y = 0; y < height; y++)
        {
			ret += new string(slice(code, width * y, width)) + "\n";
		}
        return ret.Remove(ret.Length - 1); // remove last \n
    }
    /*
	   _
	   _
	   _
	   _
	   _   <- start_y
	   *
	*/
    string get_program_start(int inner_height, int start_y)
	{
		int height = inner_height + 2;
		char[] code = new char[height];
		set(code, ' ');
		code[start_y + 1] = '*';
		return code2str(code, 1, height);
	}
	void error(string msg)
    {
		Console.WriteLine(msg);
		Environment.Exit(1);
	}
	// gap: -1|0|1
	/*
	   code         newcode    return
	   xxxxxxxx     yyyy       xxxxxxxxyyyy
	   xxxxxxxx     yyyy	   xxxxxxxxyyyy
	   xxxxxxxx     yyyy	   xxxxxxxxyyyy
	   xxxxxxxx  +  yyyy   =   xxxxxxxxyyyy
	   xxxxxxxx     yyyy	   xxxxxxxxyyyy
	   xxxxxxxx     yyyy	   xxxxxxxxyyyy
	*/
	string append_2dcode(string code, string newcode, int gap)
    {
		var lines = code.Split('\n');
		var lines2 = newcode.Split('\n');
        if (lines.Length != lines2.Length) error("internal error in append_2dcode()");

		for (int i = 0; i < lines.Length; i++)
		{
			if (gap == 1) lines[i] += " ";
			string ln = gap == -1 ? lines2[i].Remove(0, 1) : lines2[i]; // or .Substring(1)
			lines[i] += ln;
		}

		return String.Join("\n", lines);

	}

	int total_width(Cell last_cell)
	{
		int w = 0;
		for (Cell cell = last_cell; cell != null; cell = cell.prev) w += cell.width;
		return w;
	}
	Cell[] find_next(Cell[] prev_cells, int bit)
	{
		int inner_height = prev_cells[0].height - 2;

		Cell[] cells = null;
		foreach (Cell prev_cell in prev_cells)
		{
			Cell[] more_cells = generate(prev_cell.exit_state, inner_height, bit);
			if (more_cells.Length == 0) continue;
			foreach (Cell cell in more_cells) cell.prev = prev_cell;

			if (cells != null)
			{
				foreach (Cell cell in more_cells)
				{
					int k = Array.FindIndex(cells, cell_ => cell_.exit_state.Equals(cell.exit_state));

					if (k != -1) // cell and existing_cell have similar exit states, choose the one with shortest total width
					{
						if (total_width(cell) < total_width(cells[k])) // if cell's total_width is shorter than existing_cell...
							cells[k] = cell; // ...replace it
					}
					else // found a cell with new exit state
					{
						Array.Resize(ref cells, cells.Length + 1);
						cells[cells.Length - 1] = cell;
					}
				}
			}
			else
			{
				cells = more_cells;
			}
		}
		return cells;
	}

	Cell[] generate(ProgState start_state, int inner_height, int bit)
	{
		Cell[] cells = new Cell[0]; // retval

		int max_inner_width = 9;//9
		for (int inner_width = 1; inner_width <= max_inner_width; inner_width++)
		{
			int code_len = inner_width * inner_height;
			char[] inner_code = new char[code_len];
			int max_asterisks = min(5, code_len - 2);
			for (int n_asterisks = 0; n_asterisks <= max_asterisks; n_asterisks++)
			{
				// lexicographically smallest permutation is the one with all the asterisks at the end, eg. _____***
				set(inner_code, ' ');
				set(inner_code, '*', code_len - n_asterisks, n_asterisks);

				while (true) // for all permutations of code
				{
					init_interpreter(start_state, inner_code, inner_width, inner_height);

					if (execute() && output == bit)
					{
						code[width * y + x] = ' '; // make exit hole
						// Two Clone() calls below are not necessary, but I do them to clarify things.
						ProgState exit_state = new ProgState { tape = (int[])tape.Clone(), index = index, y = y };
						int i = Array.FindIndex(cells, cell => cell.exit_state.Equals(exit_state));
						if (i == -1) // found a cell with new exit_state
						{
							// if found first cell then limit max_inner_width, so
							// we are looking only for cells with current inner_width and current inner_width plus one
							if (cells.Length == 0)
								max_inner_width = min(inner_width + 1, max_inner_width);

							Array.Resize(ref cells, cells.Length + 1);
							cells[cells.Length - 1] = new Cell { code = (char[])code.Clone(), width = width, height = height, exit_state = exit_state };

							stat.min_inner_width = min(inner_width, stat.min_inner_width);
							stat.max_inner_width = max(inner_width, stat.max_inner_width);
							stat.min_asterisks = min(n_asterisks, stat.min_asterisks);
							stat.max_asterisks = max(n_asterisks, stat.max_asterisks);
						}
					}
					
					if (!next_permutation(inner_code)) break;
				}
			}
		}
		return cells;
	}
	
	int min(int a, int b) { return a < b ? a : b; }
	int max(int a, int b) { return a > b ? a : b; }

	// https://stackoverflow.com/a/12768718
	bool next_permutation<T>(IList<T> a) where T : IComparable
	{
		if (a.Count < 2) return false;
		var k = a.Count - 2;

		while (k >= 0 && a[k].CompareTo(a[k + 1]) >= 0) k--;
		if (k < 0) return false;

		var l = a.Count - 1;
		while (l > k && a[l].CompareTo(a[k]) <= 0) l--;

		var tmp = a[k];
		a[k] = a[l];
		a[l] = tmp;

		var i = k + 1;
		var j = a.Count - 1;
		while (i < j)
		{
			tmp = a[i];
			a[i] = a[j];
			a[j] = tmp;
			i++;
			j--;
		}

		return true;
	}

	void set(char[] ary, char val, int start = 0, int len = -1)
    {
		if (len == -1) len = ary.Length - start;
		for (int i = start; i < start+len; i++) ary[i] = val;
	}
	void init_interpreter(ProgState state, char[] inner_code, int inner_width, int inner_height)
	{
		tape = (int[])state.tape.Clone(); // Clone() is important here
		index = state.index;
		x = 0;
		y = state.y;
		dir = RIGHT;

		// init code/width/height
		width = inner_width + 2;
		height = inner_height + 2;
		int len = width * height;

		Array.Resize(ref code, len);
		set(code, ' ');

		set(code, '*', 0, width); // top edge
		set(code, '*', width* (height - 1), width); // bottom
		for (int y = 1; y < height - 1; y++)
		{
			code[width * y] = '*'; // left edge
			code[width * y + width - 1] = '*'; // right
			Array.Copy(inner_code, inner_width * (y - 1), code, width * y + 1, inner_width);
		}

		code[y * width] = ' '; // entry point; y is Program.y

		output = -1;
	}

	// returns true if printed one bit and successfully "exited" (reached right edge at correct place, see allowed_exits)
	// returns false otherwise
	bool execute()
	{
		bool[] allowed_exits = new bool[height];
		for (int i = 1; i < height - 1; i++) allowed_exits[i] = true;

		int dx, dy;
		int steps = 0;
		while (true)
		{
			char c = code[width * y + x];
			if (c == ' ')
			{
				switch (dir)
				{
					case LEFT:
						index--;
						if (index < 0) return false;
						if(index >= tape.Length) Array.Resize(ref tape, index+1);
						tape[index] = (tape[index] == 0 ? 1 : 0);
						if (index == 0)
						{
							if (tape[1] == 1)
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
				(dx, dy) = dir2dxdy(dir);
				x -= dx;
				y -= dy;

				if (index >= tape.Length) Array.Resize(ref tape, index + 1);
				dir = turn(dir, tape[index] == 1 ? RIGHT : LEFT);
			}

			(dx, dy) = dir2dxdy(dir);
			x += dx;
			y += dy;

			// the only way out is through entry point, so !xyok(x,y) is reduced to x < 0
			if (x < 0) return false;

			if (x == width - 1) // reached right edge
			{
				// if at right edge and already printed smth and allowed to exit here then return success
				if (output != -1 && allowed_exits[y]) 
				{
					stat.max_steps = max(steps, stat.max_steps);
					return true;
				}
				allowed_exits[y] = false;
				if (!allowed_exits.Contains(true)) return false; // no more allowed exits left
			}

			if (steps++ >= max_steps) return false;
		}
	}

	(int dx, int dy) dir2dxdy(Dir dir)
	{
		switch (dir)
		{
			case LEFT:	return (dx: -1, dy: 0);
			case RIGHT:	return (dx: 1, dy: 0);
			case UP:	return (dx: 0, dy: -1);
			case DOWN:	return (dx: 0, dy: 1);
		}
		return (0,0); // unreachable
	}

	Dir turn(Dir dir, Dir turn)
	{
		switch (dir)
		{
			case LEFT:	return turn == LEFT ? DOWN : UP;
			case RIGHT:	return turn == LEFT ? UP : DOWN;
			case UP:	return turn == LEFT ? LEFT : RIGHT;
			case DOWN:	return turn == LEFT ? RIGHT : LEFT;
		}
		return 0; // unreachable
	}
}
