#pragma once

class Channel_Map {
public:
	int free();
	int play(int note, int vel);
	int stop(int note);
	int reset();
	
	Channel_Map(void)
	{
		int incr;
		ch_free = 16;

		for (incr = 0; incr < 16; incr++)
		{
			ch_state[incr] = 0; // 0=free for assignment, 1=in use, 2=delayed stop
			ch_note[incr] = 128; // 0...127 == MIDI notes, 128 == unnassigned 
			ch_bend[incr] = 8192; // Middle of pitch bend range
			ch_age[incr] = 0;
		}

	}

private:
	int ch_free;
	int ch_state[16]; // 0=free for assignment, 1=in use, 2=delayed stop
	int ch_note[16];
	int ch_bend[16];
	int ch_age[16];
	
};


int Channel_Map::free()
{
	return ch_free;
}

int Channel_Map::play(int note, int vel)
{
	return 0;
}

int Channel_Map::stop(int note)
{
	return 0;
}

int Channel_Map::reset()
{
	return 0;
}
