/**
	@file
	Grids - Topographic Drum Sequencer

	@ingroup	examples	
*/
//
// Grids for Live.
//
// Port of Mutable Instruments Grids for Max4Live
//
// Author: Henri DAVID 2014
// https://github.com/hdavids/Grids
//
// Based on code of Olivier Gillet (ol.gillet@gmail.com)
// original code from https://github.com/pichenettes/eurorack/tree/master/grids
//
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// TODO : fix euclidian mode.

// The standard random() function is not standard on Windows.
// We need to do this to setup the rand_s() function.

#ifdef WIN_VERSION
#define _CRT_RAND_S
#endif

#include "ext.h"
#include "resources.h"
#include <math.h>


typedef struct _grids
{
	t_object	ob;
	t_atom		val;
	t_symbol	*name;
    
    t_uint8 mode;
    
    t_uint8 kNumParts;
	t_uint8 kStepsPerPattern ;
	
	t_uint8 map_x;
	t_uint8 map_y;
	t_uint8 randomness;
	
	t_uint8 euclidean_length[3];
	t_uint8 part_perturbation[3];
	t_uint8 density[3] ;
	t_uint8 euclidean_step[3];
	t_uint8 velocities[3];
	
	t_uint8 step;
	bool first_beat;
	bool beat;
	t_uint8 state;
    
    void *outlet_kick_gate;
    void *outlet_snare_gate;
    void *outlet_hihat_gate;
    void *outlet_kick_accent_gate;
    void *outlet_snare_accent_gate;
    void *outlet_hihat_accent_gate;
    
} t_grids;



void *grids_new(t_symbol *s, long argc, t_atom *argv);
void grids_free(t_grids *x);
void grids_assist(t_grids *x, void *b, long m, long a, char *s);

void grids_in_kick_density(t_grids *x, long n);
void grids_in_snare_density(t_grids *x, long n);
void grids_in_hihat_density(t_grids *x, long n);
void grids_in_map(t_grids *x, long n);
void grids_in_randomness(t_grids *x, long n);
void grids_in_kick_euclidian_length(t_grids *x, long n);
void grids_in_snare_euclidian_length(t_grids *x, long n);
void grids_in_hihat_euclidian_length(t_grids *x, long n);
void grids_in_mode_and_clock(t_grids *x, long n);

t_uint8 grids_read_drum_map(t_grids *grids, t_uint8 instrument);
void grids_tick_clock(t_grids *grids, t_uint8 playHead);
void grids_evaluate_drums(t_grids *grids);
void grids_evaluate_euclidean(t_grids *grids);
void grids_evaluate(t_grids *grids);
void grids_run(t_grids *grids, t_uint8 playHead);
void grids_output (t_grids *grids);


void *grids_class;



void *grids_new(t_symbol *s, long argc, t_atom *argv)
{
    
    t_grids *x = NULL;
    
	if ((x = (t_grids *)object_alloc(grids_class))) {
        
        //inlets
        for(t_uint8 i=9;i>0;i--){
            intin(x, i);
        }
        
        //outlets
        x->outlet_hihat_accent_gate = intout((t_object *)x);
        x->outlet_snare_accent_gate = intout((t_object *)x);
        x->outlet_kick_accent_gate = intout((t_object *)x);
        x->outlet_hihat_gate = intout((t_object *)x);
        x->outlet_snare_gate = intout((t_object *)x);
        x->outlet_kick_gate = intout((t_object *)x);
        
        //configuration
        x-> kNumParts = 3;
        x-> kStepsPerPattern = 32;
        
        //parameters
        x-> map_x = 64;
        x-> map_y = 64;
        x-> randomness = 10;
        x-> mode = 0;
        x-> euclidean_length[0] = 5;
        x-> euclidean_length[1] = 7;
        x-> euclidean_length[2] = 11;
        x-> part_perturbation[0] = 0;
        x-> part_perturbation[1] = 0;
        x-> part_perturbation[2] = 0;
        x-> density[0]=32;
        x-> density[1]=32;
        x-> density[2]=32;
        
        //runing vars
        x-> euclidean_step[0] = 0;
        x-> euclidean_step[1] = 0;
        x-> euclidean_step[2] = 0;
        
        x-> velocities[0] = 0;
        x-> velocities[1] = 0;
        x-> velocities[2] = 0;
        
        x-> state=0;
        
        x-> step=0;
        x-> first_beat=false;
        x-> beat=false;

	}
    
    return x;
}


int C74_EXPORT main(void)
{
	t_class *c;
	
    //create grids object
	c = class_new("grids", (method)grids_new, (method)grids_free, (long)sizeof(t_grids), 0L /* leave NULL!! */, A_GIMME, 0);
	
    //inlet definition
    class_addmethod(c, (method)grids_in_kick_density,			"in1",		A_LONG, 0);
    class_addmethod(c, (method)grids_in_snare_density,			"in2",		A_LONG, 0);
    class_addmethod(c, (method)grids_in_hihat_density,			"in3",		A_LONG, 0);
    class_addmethod(c, (method)grids_in_map,                    "in4",		A_LONG, 0);
    class_addmethod(c, (method)grids_in_randomness,             "in5",		A_LONG, 0);
    class_addmethod(c, (method)grids_in_kick_euclidian_length,  "in6",		A_LONG, 0);
    class_addmethod(c, (method)grids_in_snare_euclidian_length,	"in7",		A_LONG, 0);
    class_addmethod(c, (method)grids_in_hihat_euclidian_length, "in8",		A_LONG, 0);
    class_addmethod(c, (method)grids_in_mode_and_clock,         "in9",		A_LONG, 0);
        
	CLASS_METHOD_ATTR_PARSE(c, "identify", "undocumented", gensym("long"), 0, "1");
    
    //inlet tooltips
    class_addmethod(c, (method)grids_assist,			"assist",		A_CANT, 0);
	
	CLASS_ATTR_SYM(c, "name", 0, t_grids, name);
	
	class_register(CLASS_BOX, c);
	grids_class = c;
    
	return 0;
}


void grids_assist(t_grids *x, void *b, long m, long a, char *s)
{
    if (m == ASSIST_INLET) {	// Inlets
        switch (a) {
            case 0: sprintf(s, "Not used"); break;
            case 1: sprintf(s, "kick density"); break;
			case 2: sprintf(s, "snare density"); break;
            case 3: sprintf(s, "hihat density"); break;
            case 4: sprintf(s, "map (x:0-127,y:128-256)"); break;
            case 5: sprintf(s, "randomness"); break;
            case 6: sprintf(s, "kick euclidian length"); break;
            case 7: sprintf(s, "snare euclidian length"); break;
            case 8: sprintf(s, "hihat euclidian length"); break;
            case 9: sprintf(s, "mode/clock (-1:drums, -2:euclidian, n>=0:clock)"); break;
        }
    } else {		// Outlets
        switch (a) {
            case 0: sprintf(s, "kick gate"); break;
            case 1: sprintf(s, "snare gate"); break;
            case 2: sprintf(s, "hihat gate"); break;
            case 3: sprintf(s, "kick accent gate"); break;
            case 4: sprintf(s, "snare accent gate"); break;
            case 5: sprintf(s, "hihat accent gate"); break;
        }
    }
}



void grids_free(t_grids *x)
{
}

// Max Inlets

void grids_in_kick_density(t_grids *grids, long n)
{
	if (n >= 0 && n <= 127) grids->density[0] = (t_uint8)n;
}

void grids_in_snare_density(t_grids *grids, long n)
{
	if (n >= 0 && n <= 127) grids->density[1] = (t_uint8)n;
}

void grids_in_hihat_density(t_grids *grids, long n)
{
	if (n >= 0 && n <= 127) grids->density[2] = (t_uint8)n;
}

void grids_in_map(t_grids *grids, long n)
{
    //hack to use one inlet for 2 values
    if(n>=0 && n<=127) grids->map_x=(t_uint8)(n%256);
    if(n>127 && n<=256) grids->map_y=(t_uint8)((n-127)%256);
}

void grids_in_randomness(t_grids *grids, long n)
{
    if(n>=0 && n<=127) grids->randomness=(t_uint8)(n);
}

void grids_in_kick_euclidian_length(t_grids *grids, long n)
{
    if(n>0 && n<=32) grids->euclidean_length[0]=(t_uint8)(n);
}

void grids_in_snare_euclidian_length(t_grids *grids, long n)
{
    if(n>0 && n<=32) grids->euclidean_length[1]=(t_uint8)(n);
}

void grids_in_hihat_euclidian_length(t_grids *grids, long n)
{
    if(n>0 && n<=32) grids->euclidean_length[2]=(t_uint8)(n);
}


// Grids

void grids_in_mode_and_clock(t_grids *grids, long n)
{
    if(n>=0){
        grids_run(grids,(t_uint8)(n%256));
    }else{
        if(n==-1){
            grids->mode=0;
        }else{
            grids->mode=1;
        }
    }
}

void grids_run(t_grids *grids, t_uint8 playHead){
	grids_tick_clock(grids,playHead);
    grids_evaluate(grids);
	grids_output(grids);
}

void grids_tick_clock(t_grids *grids,t_uint8 playHead){
	
	grids->step = playHead%32;
	grids->beat = (grids->step & 0x7) == 0;
	grids->first_beat = grids->step == 0;
    
    if (grids->step & 1) {
        for (uint8_t i = 0; i < grids->kNumParts; ++i) {
            ++grids->euclidean_step[i];
        }
    }
    
}

void grids_output (t_grids *grids){
	if((grids->state & 1)>0){
        outlet_int(grids->outlet_kick_gate,grids->velocities[0]);
	}
	if((grids->state & 2)>0){
		outlet_int(grids->outlet_snare_gate,grids->velocities[1]);
	}
    
	if((grids->state & 4)>0){
		outlet_int(grids->outlet_hihat_gate,grids->velocities[2]);
	}
    
    if((grids->state & 8)>0){
        outlet_int(grids->outlet_kick_accent_gate,grids->velocities[0]);
	}
	if((grids->state & 16)>0){
		outlet_int(grids->outlet_snare_accent_gate,grids->velocities[1]);
	}
	if((grids->state & 32)>0){
		outlet_int(grids->outlet_hihat_accent_gate,grids->velocities[2]);
	}
}


void grids_evaluate(t_grids *grids) {
	grids->state = 0;
	if (grids->mode == 1) {
		grids_evaluate_euclidean(grids);
	} else {
		grids_evaluate_drums(grids);
	}
}


void grids_evaluate_drums(t_grids *grids) {
    // At the beginning of a pattern, decide on perturbation levels.
    if (grids->step == 0) {
        for (int i = 0; i < grids->kNumParts; ++i) {
            t_uint8 randomness = grids->randomness >> 2;
#ifdef WIN_VERSION
			unsigned int rand;
            rand_s(&rand);
#else
			t_uint8 rand = random();
#endif
            t_uint8 rand2 = (t_uint8)(rand%256);
            grids->part_perturbation[i] = (rand2*randomness)>>8;//U8U8MulShift8(rand, randomness);
        }
    }
    
    t_uint8 instrument_mask = 1;
    t_uint8 accent_bits = 0;
    for (int i = 0; i < grids->kNumParts; ++i) {
        t_uint8 level = grids_read_drum_map(grids, i);
        if (level < 255 - grids->part_perturbation[i]) {
            level += grids->part_perturbation[i];
        } else {
            // The sequencer from Anushri uses a weird clipping rule here. Comment
            // this line to reproduce its behavior.
            level = 255;
        }
        t_uint8 threshold = 255-grids->density[i]*2;
        if (level > threshold) {
            if (level > 192) {
                accent_bits |= instrument_mask;
            }
            grids->velocities[i] = level / 2;
            grids->state |= instrument_mask;
        }
        instrument_mask <<= 1;
    }
    grids->state |= accent_bits << 3;
}


t_uint8 grids_read_drum_map(t_grids *grids, t_uint8 instrument) {
    t_uint8 x = grids->map_x;
    t_uint8 y = grids->map_y;
    t_uint8 step = grids->step;
	t_uint8 maxValue=127;
	int i = (int)floor(x*3.0/127);
	int j = (int)floor(y*3.0 / 127);
	t_uint8* a_map = drum_map[i][j];
	t_uint8* b_map = drum_map[i + 1][j];
	t_uint8* c_map = drum_map[i][j + 1];
	t_uint8* d_map = drum_map[i + 1][j + 1];
	int offset = (instrument * grids->kStepsPerPattern) + step;
	t_uint8 a = a_map[offset];
	t_uint8 b = b_map[offset];
	t_uint8 c = c_map[offset];
	t_uint8 d = d_map[offset];
	t_uint8 r = ((a*x+b*(maxValue-x))*y + (c*x+d*(maxValue-x))*(maxValue-y))/maxValue/maxValue;
	return r;
}

void grids_evaluate_euclidean(t_grids *grids) {
    // Refresh only on sixteenth notes.
    if (grids->step & 1) {
        return;
    }
    

    
    // Euclidean pattern generation
    t_uint8 instrument_mask = 1;
    t_uint8 reset_bits = 0;
    for (int i = 0; i < grids->kNumParts; ++i) {
        grids->velocities[i] = 100;
        
        t_uint8 length = grids->euclidean_length[i];//my length is directly the right length. (grids->euclidean_length[i] >> 3) + 1;
        while (grids->euclidean_step[i] >= length) {
            grids->euclidean_step[i] -= length;
        }
        
        t_uint8 density = grids->density[i] >> 2;//my density is 127 max. grids->density[i] >> 3;
        t_uint16 address = (length - 1)* 32 + density;//U8U8Mul(length - 1, 32) + density;
        while (grids->euclidean_step[i] >= length) {
            grids->euclidean_step[i] -= length;
        }
        t_uint32 step_mask = 1L << (t_uint32)grids->euclidean_step[i];//static_cast<uint32_t>(grids->euclidean_step[i]);//
        t_uint32 pattern_bits = grids_res_euclidean[address];
        if (pattern_bits & step_mask) {
            grids->state |= instrument_mask;
        }
        
        if (grids->euclidean_step[i] == 0) {
            reset_bits |= instrument_mask;
        }
        instrument_mask <<= 1;
    }
    
    grids->state |= reset_bits << 3;
}

