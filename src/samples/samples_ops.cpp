// ------------------------------------------------------
// Protrekkr
// Based on Juan Antonio Arguelles Rius's NoiseTrekker.
//
// Copyright (C) 2008-2010 Franck Charlet.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL FRANCK CHARLET OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.
// ------------------------------------------------------

// ------------------------------------------------------
// Includes
#include "include/samples_ops.h"

#if defined(__AROS__)
#include <stdint.h>
#define int32 int32_t
#endif

// ------------------------------------------------------
// Variables
int cur_sample_buffer;
short *Sample_Back[4][2];
int Sample_Back_Channels[4];
int Sample_Back_Size[4];

// ------------------------------------------------------
// Copy part of a sample
int Sample_Copy(int32 range_start, int32 range_end)
{
    int i;
    short *dest_mono;
    short *dest_stereo;
    long copysize = (range_end - range_start);
    char nc = SampleChannels[ped_patsam][ped_split];

    if(copysize)
    {
        // Free both back buffers
        if(Sample_Back[cur_sample_buffer][0]) free(Sample_Back[cur_sample_buffer][0]);
        if(Sample_Back[cur_sample_buffer][1]) free(Sample_Back[cur_sample_buffer][1]);
        Sample_Back[cur_sample_buffer][0] = NULL;
        Sample_Back[cur_sample_buffer][1] = NULL;

        // Copy the data into the back buffer
        Sample_Back[cur_sample_buffer][0] = (short *) malloc(copysize * 2);
        if(!Sample_Back[cur_sample_buffer][0]) return 0;
        if(nc == 2)
        {
            Sample_Back[cur_sample_buffer][1] = (short *) malloc(copysize * 2);
            if(!Sample_Back[cur_sample_buffer][1])
            {
                free(Sample_Back[cur_sample_buffer][0]);
                return 0;
            }
        }
        Sample_Back_Channels[cur_sample_buffer] = nc;
        Sample_Back_Size[cur_sample_buffer] = copysize;

        // Copy the selection into the back buffer
        dest_mono = Sample_Back[cur_sample_buffer][0];
        dest_stereo = Sample_Back[cur_sample_buffer][1];
        for(i = range_start; i < range_end; i++)
        {
            *dest_mono++ = *(RawSamples[ped_patsam][0][ped_split] + i);
            if(nc == 2) *dest_stereo++ = *(RawSamples[ped_patsam][1][ped_split] + i);
        }
        return 1;
    }
    return 0;
}

// ------------------------------------------------------
// Paste a back buffer into a sample
int Sample_Paste(int32 range_start)
{
    int32 i;
    short *NewBuffer[2];
    char nc;
    long p_s;
    long cutsize = Sample_Back_Size[cur_sample_buffer];
    long newsize = SampleNumSamples[ped_patsam][ped_split] + cutsize;

    if(cutsize)
    {
        nc = SampleChannels[ped_patsam][ped_split];

        // Allocate the destination buffer(s)
        // (We need to clear the second one as the back buffer may not be stereo).
        NewBuffer[0] = (short *) malloc(newsize * 2);
        if(!NewBuffer[0]) return 0;

        if(nc == 2)
        {
            NewBuffer[1] = (short *) malloc(newsize * 2);
            if(!NewBuffer[1])
            {
                free(NewBuffer[0]);
                return 0;
            }
            memset(NewBuffer[1], 0, newsize * 2);
        }

        p_s = 0;
        if(range_start > 0)
        {
            // Copy the original data into the new buffer
            for(i = 0; i < range_start; i++)
            {
                *(NewBuffer[0] + p_s) = *(RawSamples[ped_patsam][0][ped_split] + i);
                if(nc == 2) *(NewBuffer[1] + p_s) = *(RawSamples[ped_patsam][1][ped_split] + i);
                p_s++;
            }
        }

        // Copy the back buffer(s) now
        for(i = 0; i < cutsize; i++)
        {
            *(NewBuffer[0] + p_s) = *(Sample_Back[cur_sample_buffer][0] + i);
            if(Sample_Back_Size[cur_sample_buffer] == 2 && nc == 2) *(NewBuffer[1] + p_s) = *(Sample_Back[cur_sample_buffer][1] + i);
            p_s++;
        }

        if((SampleNumSamples[ped_patsam][ped_split] - range_start) > 0)
        {
            // Add the rest of the original data
            for(i = range_start; i < (int32) SampleNumSamples[ped_patsam][ped_split]; i++)
            {
                *(NewBuffer[0] + p_s) = *(RawSamples[ped_patsam][0][ped_split] + i);
                if(nc == 2) *(NewBuffer[1] + p_s) = *(RawSamples[ped_patsam][1][ped_split] + i);
                p_s++;
            }
        }

        // Set the new buffer as current sample
        if(RawSamples[ped_patsam][0][ped_split]) free(RawSamples[ped_patsam][0][ped_split]);
        RawSamples[ped_patsam][0][ped_split] = NewBuffer[0];
        if(nc == 2)
        {
            if(RawSamples[ped_patsam][1][ped_split]) free(RawSamples[ped_patsam][1][ped_split]);
            RawSamples[ped_patsam][1][ped_split] = NewBuffer[1];
        }

        SampleNumSamples[ped_patsam][ped_split] = newsize;
        Status_Box("Done.");
        return 1;
    }
    return 0;
}

// ------------------------------------------------------
// Cut part of a sample
int Sample_Cut(int32 range_start, int32 range_end, int do_copy)
{
    int32 i;
    short *NewBuffer[2];
    char nc;
    long p_s;
    long cutsize = (range_end - range_start);
    long newsize = SampleNumSamples[ped_patsam][ped_split] - cutsize;

    if(newsize)
    {
        Status_Box("Cutting sample...");
        Stop_Current_Sample();
        AUDIO_Stop();
        SDL_Delay(100);

        nc = SampleChannels[ped_patsam][ped_split];

        // Allocate the wav with the minus the block to cut
        NewBuffer[0] = (short *) malloc(newsize * 2);
        if(!NewBuffer[0]) return 0;
        if(nc == 2)
        {
            NewBuffer[1] = (short *) malloc(newsize * 2);
            if(!NewBuffer[1])
            {
                free(NewBuffer[0]);
                return 0;
            }
        }

        if(do_copy)
        {
            if(!Sample_Copy(range_start, range_end))
            {
                free(NewBuffer[1]);
                free(NewBuffer[0]);
                return 0;
            }
        }

        p_s = 0;
        if(range_start > 0)
        {
            // Copy the data located before the range start
            for(i = 0; i < range_start; i++)
            {
                *(NewBuffer[0] + p_s) = *(RawSamples[ped_patsam][0][ped_split] + i);
                if(nc == 2) *(NewBuffer[1] + p_s) = *(RawSamples[ped_patsam][1][ped_split] + i);
                p_s++;
            }
        }

        if((SampleNumSamples[ped_patsam][ped_split] - range_end) > 0)
        {
            // Add the data located after the range end
            for(i = range_end; i < (int32) SampleNumSamples[ped_patsam][ped_split]; i++)
            {
                *(NewBuffer[0] + p_s) = *(RawSamples[ped_patsam][0][ped_split] + i);
                if(nc == 2) *(NewBuffer[1] + p_s) = *(RawSamples[ped_patsam][1][ped_split] + i);
                p_s++;
            }
        }

        // Set the new buffer as current sample
        if(RawSamples[ped_patsam][0][ped_split]) free(RawSamples[ped_patsam][0][ped_split]);
        RawSamples[ped_patsam][0][ped_split] = NewBuffer[0];
        if(nc == 2)
        {
            if(RawSamples[ped_patsam][1][ped_split]) free(RawSamples[ped_patsam][1][ped_split]);
            RawSamples[ped_patsam][1][ped_split] = NewBuffer[1];
        }

        SampleNumSamples[ped_patsam][ped_split] = newsize;
        Status_Box("Done.");
        return 1;
    }
    else
    {
        Status_Box("You cannot cut entire sample, use 'delete' on instrument instead.");
        return 0;
    }
}

// ------------------------------------------------------
// Adjust DC of a sample
void Sample_DC_Adjust(int32 range_start, int32 range_end)
{
    int32 i;
    char nc = SampleChannels[ped_patsam][ped_split];
    float l_shift = 0;
    float r_shift = 0;

    Status_Box("Calculating shifting-factor...");

    for(i = range_start; i < range_end + 1; i++)
    {
        l_shift += *(RawSamples[ped_patsam][0][ped_split] + i);
        if(nc == 2) r_shift += *(RawSamples[ped_patsam][1][ped_split] + i);
    }

    l_shift /= (range_end + 1) - range_start;
    r_shift /= (range_end + 1) - range_start;

    Status_Box("Re-building waves...");

    for(i = range_start; i < range_end + 1; i++)
    {
        float bleak = *(RawSamples[ped_patsam][0][ped_split] + i);
        bleak -= l_shift;

        if(bleak > 32767) bleak = 32767;
        if(bleak < -32767) bleak = -32767;
        *(RawSamples[ped_patsam][0][ped_split] + i) = (short) bleak;

        if(nc == 2)
        {
            bleak = *(RawSamples[ped_patsam][1][ped_split] + i);
            bleak -= r_shift;

            if(bleak > 32767) bleak = 32767;
            if(bleak < -32767) bleak = -32767;
            *(RawSamples[ped_patsam][1][ped_split] + i) = (short) bleak;
        }
    }

    draw_sampled_wave = TRUE;
    Status_Box("Selection calibrated.");
}

// ------------------------------------------------------
// Maximize a sample
void Sample_Maximize(int32 range_start, int32 range_end)
{
    int32 i;
    char nc = SampleChannels[ped_patsam][ped_split];
    float l_shift = 0;

    Status_Box("Searching highest peak...");

    for(i = range_start; i < range_end + 1; i++)
    {
        if(abs(*(RawSamples[ped_patsam][0][ped_split] + i)) > l_shift)
        {
            l_shift = *(RawSamples[ped_patsam][0][ped_split] + i);
        }
        if(nc == 2)
        {
            if(abs(*(RawSamples[ped_patsam][1][ped_split] + i)) > l_shift)
            {
                l_shift = *(RawSamples[ped_patsam][1][ped_split] + i);
            }
        }
    }

    l_shift = 32768.0f / l_shift;

    Status_Box("Amplifying...");

    for(i = range_start; i < range_end + 1; i++)
    {
        float bleak = *(RawSamples[ped_patsam][0][ped_split] + i);
        bleak *= l_shift;

        if(bleak > 32767) bleak = 32767;
        if(bleak < -32767) bleak = -32767;
        *(RawSamples[ped_patsam][0][ped_split] + i) = (short) bleak;

        if(nc == 2)
        {
            bleak = *(RawSamples[ped_patsam][1][ped_split] + i);
            bleak *= l_shift;

            if(bleak > 32767) bleak = 32767;
            if(bleak < -32767) bleak = -32767;
            *(RawSamples[ped_patsam][1][ped_split] + i) = (short) bleak;
        }
    }

    draw_sampled_wave = TRUE;
    Status_Box("Selection maximized.");
}

// ------------------------------------------------------
// Fade a sample in
void Sample_FadeIn(int32 range_start, int32 range_end)
{
    int i;
    char nc = SampleChannels[ped_patsam][ped_split];
    float c_vol = 0.0f;
    float const coef_vol = 1.0f / ((range_end + 1) - range_start);

    Status_Box("Fade In Selection...");

    for(i = range_start; i < range_end + 1; i++)
    {
        float bleak = *(RawSamples[ped_patsam][0][ped_split] + i);
        bleak *= c_vol;
        if(bleak > 32767) bleak = 32767;
        if(bleak < -32767) bleak = -32767;

        *(RawSamples[ped_patsam][0][ped_split] + i) = (short) bleak;

        if(nc == 2)
        {
            bleak = *(RawSamples[ped_patsam][1][ped_split] + i);
            bleak *= c_vol;

            if(bleak > 32767) bleak = 32767;
            if(bleak < -32767) bleak = -32767;
            *(RawSamples[ped_patsam][1][ped_split] + i) = (short) bleak;
        }
        c_vol += coef_vol;
    }

    draw_sampled_wave = TRUE;
    Status_Box("Done.");
}

// ------------------------------------------------------
// Fade a sample out
void Sample_FadeOut(int32 range_start, int32 range_end)
{
    int32 i;
    Status_Box("Fade Out Selection...");
    SDL_Delay(100);

    char nc = SampleChannels[ped_patsam][ped_split];

    float c_vol = 1.0f;
    float const coef_vol = 1.0f / ((range_end + 1) - range_start);

    for(i = range_start; i < range_end + 1; i++)
    {
        float bleak = *(RawSamples[ped_patsam][0][ped_split] + i);
        bleak *= c_vol;
        if(bleak > 32767) bleak = 32767;
        if(bleak < -32767) bleak = -32767;

        *(RawSamples[ped_patsam][0][ped_split] + i) = (short) bleak;

        if(nc == 2)
        {
            bleak = *(RawSamples[ped_patsam][1][ped_split] + i);
            bleak *= c_vol;

            if(bleak > 32767) bleak = 32767;
            if(bleak < -32767) bleak = -32767;
            *(RawSamples[ped_patsam][1][ped_split] + i) = (short) bleak;
        }
        c_vol -= coef_vol;
    }

    draw_sampled_wave = TRUE;
    Status_Box("Finished.");
}

// ------------------------------------------------------
// Half a sample
void Sample_Half(int32 range_start, int32 range_end)
{
    int32 i;

    Status_Box("Halving Selection Volume...");

    char nc = SampleChannels[ped_patsam][ped_split];

    float c_vol = 0.5f;

    for(i = range_start; i < range_end + 1; i++)
    {
        float bleak = *(RawSamples[ped_patsam][0][ped_split] + i);
        bleak *= c_vol;

        *(RawSamples[ped_patsam][0][ped_split] + i) = (short) bleak;
        if(nc == 2)
        {
            bleak = *(RawSamples[ped_patsam][1][ped_split] + i);
            bleak *= c_vol;
            *(RawSamples[ped_patsam][1][ped_split] + i) = (short) bleak;
        }
    }

    draw_sampled_wave = TRUE;
    Status_Box("Done.");
}