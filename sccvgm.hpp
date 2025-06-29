/**
 * sccvgm.hpp -- SCC VGM Driver
 * Copyright (C) 2001-2022 Mitsutaka Okazaki (EMU2149 class)
 * Copyright (c) 2014 Mitsutaka Okazaki (EMU2212 class)
 * Copyright (C) 2025 Yoji Suzuki (SccVgmDriver)
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

namespace scc
{

class EMU2149
{
  private:
    typedef struct
    {
        const uint32_t* voltbl;
        uint8_t reg[0x20];
        int32_t out;
        uint32_t clk, rate, base_incr;
        uint8_t clk_div;
        uint16_t count[3];
        uint8_t volume[3];
        uint16_t freq[3];
        uint8_t edge[3];
        uint8_t tmask[3];
        uint8_t nmask[3];
        uint32_t mask;
        uint32_t base_count;
        uint8_t env_ptr;
        uint8_t env_face;
        uint8_t env_continue;
        uint8_t env_attack;
        uint8_t env_alternate;
        uint8_t env_hold;
        uint8_t env_pause;
        uint16_t env_freq;
        uint32_t env_count;
        uint32_t noise_seed;
        uint8_t noise_scaler;
        uint8_t noise_count;
        uint8_t noise_freq;
        uint32_t realstep;
        uint32_t psgtime;
        uint32_t psgstep;
        uint32_t freq_limit;
        uint8_t adr;
        int16_t ch_out[3];
    } Context;

    Context* psg;

    const uint32_t voltbl[2][32] = {
        /* YM2149 - 32 steps */
        {0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09,
         0x0B, 0x0D, 0x0F, 0x12,
         0x16, 0x1A, 0x1F, 0x25, 0x2D, 0x35, 0x3F, 0x4C, 0x5A, 0x6A, 0x7F, 0x97,
         0xB4, 0xD6, 0xFF, 0xFF},
        /* AY-3-8910 - 16 steps */
        {0x00, 0x00, 0x03, 0x03, 0x04, 0x04, 0x06, 0x06, 0x09, 0x09, 0x0D, 0x0D,
         0x12, 0x12, 0x1D, 0x1D,
         0x22, 0x22, 0x37, 0x37, 0x4D, 0x4D, 0x62, 0x62, 0x82, 0x82, 0xA6, 0xA6,
         0xD0, 0xD0, 0xFF, 0xFF}};

    const uint8_t regmsk[16] = {
        0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0x3f,
        0x1f, 0x1f, 0x1f, 0xff, 0xff, 0x0f, 0xff, 0xff};

    const int GETA_BITS = 24;

    inline int PSG_MASK_CH(int x) { return (1 << (x)); }

  public:
    EMU2149(uint32_t clock, uint32_t rate)
    {
        psg = new Context();
        setVolumeMode(0);
        psg->clk = clock;
        psg->clk_div = 0;
        psg->rate = rate ? rate : 44100;
        internal_refresh();
        setMask(0x00);
    }

    ~EMU2149()
    {
        delete psg;
    }

    uint32_t getFrequency(int ch)
    {
        if (0 <= ch && ch < 3) {
            return psg->freq[ch];
        } else {
            return 0;
        }
    }

    void setClock(uint32_t clock)
    {
        if (psg->clk != clock) {
            psg->clk = clock;
            internal_refresh();
        }
    }

    void setClockDivider(uint8_t enable)
    {
        if (psg->clk_div != enable) {
            psg->clk_div = enable;
            internal_refresh();
        }
    }

    void setRate(uint32_t rate)
    {
        uint32_t r = rate ? rate : 44100;
        if (psg->rate != r) {
            psg->rate = r;
            internal_refresh();
        }
    }

    void setVolumeMode(int type)
    {
        switch (type) {
            case 1:
                psg->voltbl = voltbl[0]; /* YM2149 */
                break;
            case 2:
                psg->voltbl = voltbl[1]; /* AY-3-8910 */
                break;
            default:
                psg->voltbl = voltbl[0]; /* fallback: YM2149 */
                break;
        }
    }

    uint32_t setMask(uint32_t mask)
    {
        uint32_t ret = 0;
        if (psg) {
            ret = psg->mask;
            psg->mask = mask;
        }
        return ret;
    }

    uint32_t toggleMask(uint32_t mask)
    {
        uint32_t ret = 0;
        if (psg) {
            ret = psg->mask;
            psg->mask ^= mask;
        }
        return ret;
    }

    void reset()
    {
        int i;

        psg->base_count = 0;

        for (i = 0; i < 3; i++) {
            psg->count[i] = 0;
            psg->freq[i] = 0;
            psg->edge[i] = 0;
            psg->volume[i] = 0;
            psg->ch_out[i] = 0;
        }

        psg->mask = 0;

        for (i = 0; i < 16; i++)
            psg->reg[i] = 0;
        psg->adr = 0;

        psg->noise_seed = 0xffff;
        psg->noise_scaler = 0;
        psg->noise_count = 0;
        psg->noise_freq = 0;

        psg->env_ptr = 0;
        psg->env_freq = 0;
        psg->env_count = 0;
        psg->env_pause = 1;

        psg->out = 0;
    }

    uint8_t readIO()
    {
        return (uint8_t)(psg->reg[psg->adr]);
    }

    uint8_t readReg(uint32_t reg)
    {
        return (uint8_t)(psg->reg[reg & 0x1f]);
    }

    void writeIO(uint32_t adr, uint32_t val)
    {
        if (adr & 1)
            writeReg(psg->adr, val);
        else
            psg->adr = val & 0x1f;
    }

    void writeReg(uint32_t reg, uint32_t val)
    {
        int c;

        if (reg > 15)
            return;

        val &= regmsk[reg];

        psg->reg[reg] = (uint8_t)val;

        switch (reg) {
            case 0:
            case 2:
            case 4:
            case 1:
            case 3:
            case 5:
                c = reg >> 1;
                psg->freq[c] = ((psg->reg[c * 2 + 1] & 15) << 8) + psg->reg[c * 2];
                break;

            case 6:
                psg->noise_freq = val & 31;
                break;

            case 7:
                psg->tmask[0] = (val & 1);
                psg->tmask[1] = (val & 2);
                psg->tmask[2] = (val & 4);
                psg->nmask[0] = (val & 8);
                psg->nmask[1] = (val & 16);
                psg->nmask[2] = (val & 32);
                break;

            case 8:
            case 9:
            case 10:
                psg->volume[reg - 8] = val << 1;
                break;

            case 11:
            case 12:
                psg->env_freq = (psg->reg[12] << 8) + psg->reg[11];
                break;

            case 13:
                psg->env_continue = (val >> 3) & 1;
                psg->env_attack = (val >> 2) & 1;
                psg->env_alternate = (val >> 1) & 1;
                psg->env_hold = val & 1;
                psg->env_face = psg->env_attack;
                psg->env_pause = 0;
                psg->env_ptr = psg->env_face ? 0 : 0x1f;
                break;

            case 14:
            case 15:
            default:
                break;
        }

        return;
    }
    int16_t calc()
    {
        /* Simple rate converter (See README for detail). */
        while (psg->realstep > psg->psgtime) {
            psg->psgtime += psg->psgstep;
            update_output();
            psg->out += mix_output();
            psg->out >>= 1;
        }
        psg->psgtime -= psg->realstep;
        return psg->out;
    }

  private:
    void internal_refresh()
    {
        uint32_t f_master = psg->clk;

        if (psg->clk_div) {
            f_master /= 2;
        }

        psg->base_incr = 1 << GETA_BITS;
        psg->realstep = f_master;
        psg->psgstep = psg->rate * 8;
        psg->psgtime = 0;
        psg->freq_limit = (uint32_t)(f_master / 16 / (psg->rate / 2));
    }
    inline void update_output()
    {
        int i, noise;
        uint8_t incr;

        psg->base_count += psg->base_incr;
        incr = (psg->base_count >> GETA_BITS);
        psg->base_count &= (1 << GETA_BITS) - 1;

        /* Envelope */
        psg->env_count += incr;

        if (psg->env_count >= psg->env_freq) {
            if (!psg->env_pause) {
                if (psg->env_face)
                    psg->env_ptr = (psg->env_ptr + 1) & 0x3f;
                else
                    psg->env_ptr = (psg->env_ptr + 0x3f) & 0x3f;
            }

            if (psg->env_ptr & 0x20) /* if carry or borrow */
            {
                if (psg->env_continue) {
                    if (psg->env_alternate ^ psg->env_hold)
                        psg->env_face ^= 1;
                    if (psg->env_hold)
                        psg->env_pause = 1;
                    psg->env_ptr = psg->env_face ? 0 : 0x1f;
                } else {
                    psg->env_pause = 1;
                    psg->env_ptr = 0;
                }
            }

            if (psg->env_freq >= incr)
                psg->env_count -= psg->env_freq;
            else
                psg->env_count = 0;
        }

        /* Noise */
        psg->noise_count += incr;
        if (psg->noise_count >= psg->noise_freq) {
            psg->noise_scaler ^= 1;
            if (psg->noise_scaler) {
                if (psg->noise_seed & 1)
                    psg->noise_seed ^= 0x24000;
                psg->noise_seed >>= 1;
            }

            if (psg->noise_freq >= incr)
                psg->noise_count -= psg->noise_freq;
            else
                psg->noise_count = 0;
        }
        noise = psg->noise_seed & 1;

        /* Tone */
        for (i = 0; i < 3; i++) {
            psg->count[i] += incr;
            if (psg->count[i] >= psg->freq[i]) {
                psg->edge[i] = !psg->edge[i];

                if (psg->freq[i] >= incr)
                    psg->count[i] -= psg->freq[i];
                else
                    psg->count[i] = 0;
            }

            if (0 < psg->freq_limit && psg->freq[i] <= psg->freq_limit && psg->nmask[i]) {
                /* Mute the channel if the pitch is higher than the Nyquist frequency at the current sample rate,
                 * to prevent aliased or broken tones from being generated. Of course, this logic doesn't exist
                 * on the actual chip, but practically all tones higher than the Nyquist frequency are usually
                 * removed by a low-pass circuit somewhere, so we here halt the output. */
                continue;
            }

            if (psg->mask & PSG_MASK_CH(i)) {
                psg->ch_out[i] = 0;
                continue;
            }

            if ((psg->tmask[i] || psg->edge[i]) && (psg->nmask[i] || noise)) {
                if (!(psg->volume[i] & 32))
                    psg->ch_out[i] = (psg->voltbl[psg->volume[i] & 31] << 4);
                else
                    psg->ch_out[i] = (psg->voltbl[psg->env_ptr] << 4);
            } else {
                psg->ch_out[i] = 0;
            }
        }
    }

    inline int16_t mix_output()
    {
        return (int16_t)(psg->ch_out[0] + psg->ch_out[1] + psg->ch_out[2]);
    }
};

class EMU2212
{
  public:
    enum class Type {
        Standard,
        Enhanced,
    };

  private:
    inline int SCC_MASK_CH(int x) { return (1 << (x)); }
    const int GETA_BITS = 22;

    typedef struct {
        uint32_t clk, rate, base_incr;
        int16_t out;
        Type type;
        uint32_t mode;
        uint32_t active;
        uint32_t base_adr;
        uint32_t mask;

        uint32_t realstep;
        uint32_t scctime;
        uint32_t sccstep;

        uint32_t incr[5];

        int8_t wave[5][32];

        uint32_t count[5];
        uint32_t freq[5];
        uint32_t phase[5];
        uint32_t volume[5];
        uint32_t offset[5];
        uint8_t reg[0x100 - 0xC0];

        int ch_enable;
        int ch_enable_next;

        int cycle_4bit;
        int cycle_8bit;
        int refresh;
        int rotate[5];

        int16_t ch_out[5];
    } Context;

    Context* scc;

  public:
    EMU2212(uint32_t c, uint32_t r)
    {
        scc = new Context();
        scc->clk = c;
        scc->rate = r ? r : 44100;
        internal_refresh();
        scc->type = Type::Enhanced;
    }

    ~EMU2212()
    {
        delete scc;
    }

    uint32_t getFrequency(int ch)
    {
        if (0 <= ch && ch < 6) {
            return scc->freq[ch];
        } else {
            return 0;
        }
    }

    void reset()
    {
        int i, j;

        if (scc == NULL)
            return;

        scc->mode = 0;
        scc->active = 0;
        scc->base_adr = 0x9000;

        for (i = 0; i < 5; i++) {
            for (j = 0; j < 5; j++)
                scc->wave[i][j] = 0;
            scc->count[i] = 0;
            scc->freq[i] = 0;
            scc->phase[i] = 0;
            scc->volume[i] = 0;
            scc->offset[i] = 0;
            scc->rotate[i] = 0;
            scc->ch_out[i] = 0;
        }

        memset(scc->reg, 0, 0x100 - 0xC0);

        scc->mask = 0;

        scc->ch_enable = 0xff;
        scc->ch_enable_next = 0xff;

        scc->cycle_4bit = 0;
        scc->cycle_8bit = 0;
        scc->refresh = 0;

        scc->out = 0;

        return;
    }

    void set_rate(uint32_t r)
    {
        scc->rate = r ? r : 44100;
        internal_refresh();
    }

    void set_type(Type type)
    {
        scc->type = type;
    }

    int16_t calc()
    {
        while (scc->realstep > scc->scctime) {
            scc->scctime += scc->sccstep;
            update_output();
        }
        scc->scctime -= scc->realstep;

        return mix_output();
    }

    void write(uint32_t adr, uint32_t val)
    {
        val = val & 0xFF;

        if (scc->type == Type::Enhanced && (adr & 0xFFFE) == 0xBFFE) {
            scc->base_adr = 0x9000 | ((val & 0x20) << 8);
            return;
        }

        if (adr < scc->base_adr) return;
        adr -= scc->base_adr;

        if (adr == 0) {
            if (val == 0x3F) {
                scc->mode = 0;
                scc->active = 1;
            } else if (val & 0x80 && scc->type == Type::Enhanced) {
                scc->mode = 1;
                scc->active = 1;
            } else {
                scc->mode = 0;
                scc->active = 0;
            }
            return;
        }

        if (!scc->active || adr < 0x800 || 0x8FF < adr) return;

        if (scc->type == Type::Standard) {
            write_standard(adr, val);
        } else {
            if (scc->mode)
                write_enhanced(adr, val);
            else
                write_standard(adr, val);
        }
    }

    void writeReg(uint32_t adr, uint32_t val)
    {
        int ch;
        uint32_t freq;

        adr &= 0xFF;

        if (adr < 0xA0) {
            ch = (adr & 0xF0) >> 5;
            if (!scc->rotate[ch]) {
                scc->wave[ch][adr & 0x1F] = (int8_t)val;
                if (scc->mode == 0 && ch == 3)
                    scc->wave[4][adr & 0x1F] = (int8_t)val;
            }
        } else if (0xC0 <= adr && adr <= 0xC9) {
            scc->reg[adr - 0xC0] = val;
            ch = (adr & 0x0F) >> 1;
            if (adr & 1)
                scc->freq[ch] = ((val & 0xF) << 8) | (scc->freq[ch] & 0xFF);
            else
                scc->freq[ch] = (scc->freq[ch] & 0xF00) | (val & 0xFF);

            if (scc->refresh)
                scc->count[ch] = 0;
            freq = scc->freq[ch];
            if (scc->cycle_8bit)
                freq &= 0xFF;
            if (scc->cycle_4bit)
                freq >>= 8;
            if (freq <= 8)
                scc->incr[ch] = 0;
            else
                scc->incr[ch] = scc->base_incr / (freq + 1);
        } else if (0xD0 <= adr && adr <= 0xD4) {
            scc->reg[adr - 0xC0] = val;
            scc->volume[adr & 0x0F] = (uint8_t)(val & 0xF);
        } else if (adr == 0xE0) {
            scc->reg[adr - 0xC0] = val;
            scc->mode = (uint8_t)val & 1;
        } else if (adr == 0xE1) {
            scc->reg[adr - 0xC0] = val;
            scc->ch_enable_next = (uint8_t)val & 0x1F;
        } else if (adr == 0xE2) {
            scc->reg[adr - 0xC0] = val;
            scc->cycle_4bit = val & 1;
            scc->cycle_8bit = val & 2;
            scc->refresh = val & 32;
            if (val & 64)
                for (ch = 0; ch < 5; ch++)
                    scc->rotate[ch] = 0x1F;
            else
                for (ch = 0; ch < 5; ch++)
                    scc->rotate[ch] = 0;
            if (val & 128)
                scc->rotate[3] = scc->rotate[4] = 0x1F;
        }
    }

    uint32_t read(uint32_t adr)
    {
        if (scc->type == Type::Enhanced && (adr & 0xFFFE) == 0xBFFE)
            return (scc->base_adr >> 8) & 0x20;

        if (adr < scc->base_adr) return 0;
        adr -= scc->base_adr;

        if (adr == 0) {
            if (scc->mode)
                return 0x80;
            else
                return 0x3F;
        }
        if (!scc->active || adr < 0x800 || 0x8FF < adr) return 0;
        return scc->type == Type::Standard || !scc->mode ? read_standard(adr) : read_enhanced(adr);
    }

    uint32_t readReg(uint32_t adr)
    {
        if (adr < 0xA0)
            return scc->wave[adr >> 5][adr & 0x1f];
        else if (0xC0 < adr && adr < 0xF0)
            return scc->reg[adr - 0xC0];
        else
            return 0;
    }

    void setMask(uint32_t mask) { scc->mask = mask; }
    void toggleMask(uint32_t mask) { scc->mask ^= mask; }

    // mapper function for VGM format
    inline void write_waveform1(uint32_t adr, uint32_t val) { writeReg(adr & 0x7F, val); }
    inline void write_waveform2(uint32_t adr, uint32_t val) { writeReg((adr & 0x1F) | 0x60, val); }
    inline void write_frequency(uint32_t adr, uint32_t val) { writeReg((adr & 0x0F) | 0xC0, val); }
    inline void write_volume(uint32_t adr, uint32_t val) { writeReg((adr & 0x0F) | 0xD0, val); }
    inline void write_keyoff(uint32_t val) { writeReg(0xE1, val); }
    inline void write_test(uint32_t val) { writeReg(0xE2, val); }

  private:
    void internal_refresh()
    {
        scc->base_incr = 2 << GETA_BITS;
        scc->realstep = (uint32_t)((1 << 31) / scc->rate);
        scc->sccstep = (uint32_t)((1 << 31) / (scc->clk / 2));
        scc->scctime = 0;
    }

    inline void update_output()
    {
        int i;

        for (i = 0; i < 5; i++) {
            scc->count[i] = (scc->count[i] + scc->incr[i]);

            if (scc->count[i] & (1 << (GETA_BITS + 5))) {
                scc->count[i] &= ((1 << (GETA_BITS + 5)) - 1);
                scc->offset[i] = (scc->offset[i] + 31) & scc->rotate[i];
                scc->ch_enable &= ~(1 << i);
                scc->ch_enable |= scc->ch_enable_next & (1 << i);
            }

            if (scc->ch_enable & (1 << i)) {
                scc->phase[i] = ((scc->count[i] >> (GETA_BITS)) + scc->offset[i]) & 0x1F;
                if (!(scc->mask & SCC_MASK_CH(i)))
                    scc->ch_out[i] += (scc->volume[i] * scc->wave[i][scc->phase[i]]) & 0xfff0;
            }

            scc->ch_out[i] >>= 1;
        }
    }

    inline int16_t mix_output()
    {
        scc->out = scc->ch_out[0] + scc->ch_out[1] + scc->ch_out[2] + scc->ch_out[3] + scc->ch_out[4];
        return (int16_t)scc->out;
    }

    inline void write_standard(uint32_t adr, uint32_t val)
    {
        adr &= 0xFF;

        if (adr < 0x80) /* wave */
        {
            writeReg(adr, val);
        } else if (adr < 0x8A) /* freq */
        {
            writeReg(adr + 0xC0 - 0x80, val);
        } else if (adr < 0x8F) /* volume */
        {
            writeReg(adr + 0xD0 - 0x8A, val);
        } else if (adr == 0x8F) /* ch enable */
        {
            writeReg(0xE1, val);
        } else if (0xE0 <= adr) /* flags */
        {
            writeReg(0xE2, val);
        }
    }

    inline void write_enhanced(uint32_t adr, uint32_t val)
    {
        adr &= 0xFF;

        if (adr < 0xA0) /* wave */
        {
            writeReg(adr, val);
        } else if (adr < 0xAA) /* freq */
        {
            writeReg(adr + 0xC0 - 0xA0, val);
        } else if (adr < 0xAF) /* volume */
        {
            writeReg(adr + 0xD0 - 0xAA, val);
        } else if (adr == 0xAF) /* ch enable */
        {
            writeReg(0xE1, val);
        } else if (0xC0 <= adr && adr <= 0xDF) /* flags */
        {
            writeReg(0xE2, val);
        }
    }

    inline uint32_t read_enhanced(uint32_t adr)
    {
        adr &= 0xFF;
        if (adr < 0xA0)
            return readReg(adr);
        else if (adr < 0xAA)
            return readReg(adr + 0xC0 - 0xA0);
        else if (adr < 0xAF)
            return readReg(adr + 0xD0 - 0xAA);
        else if (adr == 0xAF)
            return readReg(0xE1);
        else if (0xC0 <= adr && adr <= 0xDF)
            return readReg(0xE2);
        else
            return 0;
    }

    inline uint32_t read_standard(uint32_t adr)
    {
        adr &= 0xFF;
        if (adr < 0x80)
            return readReg(adr);
        else if (0xA0 <= adr && adr <= 0xBF)
            return readReg(0x80 + (adr & 0x1F));
        else if (adr < 0x8A)
            return readReg(adr + 0xC0 - 0x80);
        else if (adr < 0x8F)
            return readReg(adr + 0xD0 - 0x8A);
        else if (adr == 0x8F)
            return readReg(0xE1);
        else if (0xE0 <= adr)
            return readReg(0xE2);
        else
            return 0;
    }
};

class VgmDriver
{
  private:
    enum EmulatorType {
        ET_PSG = 0,
        ET_SCC,
        ET_Length
    };

    struct Emulator {
        EMU2149* psg;
        EMU2212* scc;
    } emu;

    struct VgmContext {
        uint32_t clocks[ET_Length];
        const uint8_t* data;
        size_t size;
        int version;
        int cursor;
        int loopOffset;
        int wait;
        bool end;
        uint32_t loopCount;
    } vgm;

    int masterVolume;
    short waveMax;
    short waveMin;

  public:
    VgmDriver(int rate = 44100)
    {
        emu.psg = new EMU2149(3579545, rate);
        emu.scc = new EMU2212(3579545, rate);
        masterVolume = 600;
        this->setWaveSize(95);
    }

    ~VgmDriver()
    {
        delete emu.psg;
        delete emu.scc;
    }

    void setMasterVolume(int masterVolume)
    {
        this->masterVolume = masterVolume;
    }

    void setWaveSize(int waveSizeInPercent)
    {
        if (waveSizeInPercent < 0) {
            waveSizeInPercent = 0;
        } else if (100 < waveSizeInPercent) {
            waveSizeInPercent = 100;
        }
        this->waveMax = (short)((32767 * waveSizeInPercent) / 100);
        this->waveMin = (short)((-32768 * waveSizeInPercent) / 100);
    }

    bool load(const uint8_t* data, size_t size)
    {
        this->reset();
        if (size < 0x100) {
            return false;
        }
        if (0 != memcmp("Vgm ", data, 4)) {
            return false;
        }

        memcpy(&vgm.version, &data[0x08], 4);
        if (vgm.version < 0x161) {
            return false; // require version 1.61 or later
        }

        vgm.data = data;
        vgm.size = size;

        memcpy(&vgm.clocks[ET_PSG], &data[0x74], 4);
        memcpy(&vgm.clocks[ET_SCC], &data[0x9C], 4);

        if (!vgm.clocks[ET_PSG] && !vgm.clocks[ET_SCC]) {
            return false; // require PSG or SCC, or both
        }

        if (vgm.clocks[ET_PSG]) {
            emu.psg->setVolumeMode(2);
            emu.psg->setClockDivider(1);
        }

        if (vgm.clocks[ET_SCC]) {
            emu.scc->set_type(EMU2212::Type::Standard);
        }

        memcpy(&vgm.cursor, &data[0x34], 4);
        vgm.cursor += 0x40 - 0x0C;
        memcpy(&vgm.loopOffset, &data[0x1C], 4);
        vgm.loopOffset += vgm.loopOffset ? 0x1C : 0;
        return true;
    }

    void reset()
    {
        memset(&vgm, 0, sizeof(vgm));
        emu.psg->reset();
        emu.scc->reset();
    }

    void render(int16_t* buf, int samples)
    {
        if (!vgm.data) {
            memset(buf, 0, samples * 2);
            return;
        }
        int cursor = 0;
        while (cursor < samples) {
            if (vgm.wait < 1) {
                this->execute();
            }
            vgm.wait--;
            int w = 0;
            if (vgm.clocks[ET_PSG]) {
                w += emu.psg->calc();
            }
            if (vgm.clocks[ET_SCC]) {
                w += emu.scc->calc();
            }
            w *= masterVolume;
            w /= 100;
            if (waveMax < w) {
                w = waveMax;
            } else if (w < waveMin) {
                w = waveMin;
            }
            buf[cursor] = w;
            cursor++;
        }
    }

    bool isPlaying() { return !vgm.end; }
    uint32_t getLoopCount() { return vgm.loopCount; }
    uint32_t getFrequencyPSG(int ch) { return emu.psg->getFrequency(ch); }
    uint32_t getFrequencySCC(int ch) { return emu.psg->getFrequency(ch); }

  private:
    void execute()
    {
        if (!vgm.data || vgm.end) {
            return;
        }
        while (vgm.wait < 1) {
            uint8_t cmd = vgm.data[vgm.cursor++];
            switch (cmd) {
                case 0x31: // AY-3-8910 stereo mask (ignore)
                    vgm.cursor++;
                    // emu.psg->setMask(vgm.data[vgm.cursor++]);
                    break;
                case 0xA0: {
                    // AY-3-8910 reigster
                    uint8_t addr = vgm.data[vgm.cursor++];
                    uint8_t value = vgm.data[vgm.cursor++];
                    emu.psg->writeReg(addr, value);
                    break;
                }
                case 0xD2: {
                    // SCC1
                    uint8_t port = vgm.data[vgm.cursor++] & 0x7F;
                    uint8_t offset = vgm.data[vgm.cursor++];
                    uint8_t data = vgm.data[vgm.cursor++];
                    switch (port) {
                        case 0x00: emu.scc->write_waveform1(offset, data); break;
                        case 0x01: emu.scc->write_frequency(offset, data); break;
                        case 0x02: emu.scc->write_volume(offset, data); break;
                        case 0x03: emu.scc->write_keyoff(data); break;
                        case 0x04: emu.scc->write_waveform2(offset, data); break;
                        case 0x05: emu.scc->write_test(data); break;
                    }
                    break;
                }
                case 0x61: {
                    // Wait nn samples
                    unsigned short nn;
                    memcpy(&nn, &vgm.data[vgm.cursor], 2);
                    vgm.cursor += 2;
                    vgm.wait += nn;
                    break;
                }
                case 0x62: vgm.wait += 735; break;
                case 0x63: vgm.wait += 882; break;
                case 0x66: {
                    // End of sound data
                    if (vgm.loopOffset) {
                        vgm.cursor = vgm.loopOffset;
                        vgm.loopCount++;
                        break;
                    } else {
                        vgm.end = true;
                        return;
                    }
                }

                case 0xDD:
                case 0xDE:
                case 0xDF:
                case 0xFD:
                case 0xFE:
                case 0xFF:
                    // Skip: Furnace outputs thies unsupport commands (use for labels?)
                    break;

                default:
                    // Error: Detected an unsupported command
                    vgm.end = true;
                    return;
            }
        }
    }
};

}; // namespace scc
