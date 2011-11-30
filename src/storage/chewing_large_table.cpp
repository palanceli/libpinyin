/* 
 *  libpinyin
 *  Library to deal with pinyin.
 *  
 *  Copyright (C) 2011 Peng Wu <alexepico@gmail.com>
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "chewing_large_table.h"
#include <assert.h>
#include "pinyin_phrase2.h"


/* internal class definition */

namespace pinyin{
class ChewingLengthIndexLevel{

protected:
    GArray * m_chewing_array_indexes;

public:
    /* constructor/destructor */
    ChewingLengthIndexLevel();
    ~ChewingLengthIndexLevel();

    /* load/store method */
    bool load(MemoryChunk * chunk, table_offset_t offset, table_offset_t end);
    bool store(MemoryChunk * new_chunk, table_offset_t offset,
               table_offset_t & end);

    /* search method */
    int search(pinyin_option_t options, int phrase_length,
               /* in */ ChewingKey keys[],
               /* out */ PhraseIndexRanges ranges);

    /* add/remove index method */
    int add_index(int phrase_length, /* in */ ChewingKey keys[],
                  /* in */ phrase_token_t token);
    int remove_index(int phrase_length, /* in */ ChewingKey keys[],
                     /* in */ phrase_token_t token);
};


template<size_t phrase_length>
class ChewingArrayIndexLevel{
protected:
    MemoryChunk m_chunk;

    /* compress consecutive tokens */
    int convert(pinyin_option_t options,
                ChewingKey keys[],
                PinyinIndexItem2<phrase_length> * begin,
                PinyinIndexItem2<phrase_length> * end,
                PhraseIndexRanges ranges);

public:
    /* load/store method */
    bool load(MemoryChunk * chunk, table_offset_t offset, table_offset_t end);
    bool store(MemoryChunk * new_chunk, table_offset_t offset,
               table_offset_t & end);

    /* search method */
    int search(pinyin_option_t options, /* in */ChewingKey keys[],
               /* out */ PhraseIndexRanges ranges);

    /* add/remove index method */
    int add_index(/* in */ ChewingKey keys[], /* in */ phrase_token_t token);
    int remove_index(/* in */ ChewingKey keys[],
                     /* in */ phrase_token_t token);
};

};


using namespace pinyin;

/* class implementation */

ChewingBitmapIndexLevel::ChewingBitmapIndexLevel(pinyin_option_t options)
    : m_options(options) {
    memset(m_chewing_length_indexes, 0, sizeof(m_chewing_length_indexes));
}

void ChewingBitmapIndexLevel::reset() {
    for (int k = CHEWING_ZERO_INITIAL; k < CHEWING_NUMBER_OF_INITIALS; ++k)
        for (int l = CHEWING_ZERO_MIDDLE; l < CHEWING_NUMBER_OF_MIDDLES; ++l)
            for (int m = CHEWING_ZERO_FINAL; m < CHEWING_NUMBER_OF_FINALS; ++m)
                for (int n = CHEWING_ZERO_TONE; n < CHEWING_NUMBER_OF_TONES;
                     ++n) {
                    ChewingLengthIndexLevel * & length_array =
                        m_chewing_length_indexes[k][l][m][n];
                    if (length_array)
                        delete length_array;
                    length_array = NULL;
                }
}


/* search methods */

int ChewingBitmapIndexLevel::search(int phrase_length,
                                    /* in */ ChewingKey keys[],
                                    /* out */ PhraseIndexRanges ranges) const {
    assert(phrase_length > 0);
    return initial_level_search(phrase_length, keys, ranges);
}

int ChewingBitmapIndexLevel::initial_level_search (int phrase_length,
    /* in */ ChewingKey keys[], /* out */ PhraseIndexRanges ranges) const {

/* macros */
#define MATCH(AMBIGUITY, ORIGIN, ANOTHER) case ORIGIN:                  \
    {                                                                   \
        result |= middle_and_final_level_search(ORIGIN, phrase_length,  \
                                                keys, ranges);          \
        if (m_options & AMBIGUITY) {                                    \
            result |= middle_and_final_level_search(ANOTHER,            \
                                                    phrase_length,      \
                                                    keys, ranges);      \
        }                                                               \
        return result;                                                  \
    }

    /* deal with ambiguities */
    int result = SEARCH_NONE;
    const ChewingKey & first_key = keys[0];

    switch(first_key.m_initial) {
        MATCH(PINYIN_AMB_C_CH, CHEWING_C, CHEWING_CH);
        MATCH(PINYIN_AMB_C_CH, CHEWING_CH, CHEWING_C);
        MATCH(PINYIN_AMB_Z_ZH, CHEWING_Z, CHEWING_ZH);
        MATCH(PINYIN_AMB_Z_ZH, CHEWING_ZH, CHEWING_Z);
        MATCH(PINYIN_AMB_S_SH, CHEWING_S, CHEWING_SH);
        MATCH(PINYIN_AMB_S_SH, CHEWING_SH, CHEWING_S);
        MATCH(PINYIN_AMB_L_R, CHEWING_R, CHEWING_L);
        MATCH(PINYIN_AMB_L_N, CHEWING_N, CHEWING_L);
        MATCH(PINYIN_AMB_F_H, CHEWING_F, CHEWING_H);
        MATCH(PINYIN_AMB_F_H, CHEWING_H, CHEWING_F);
        MATCH(PINYIN_AMB_G_K, CHEWING_G, CHEWING_K);
        MATCH(PINYIN_AMB_G_K, CHEWING_K, CHEWING_G);

    case CHEWING_L:
        {
            result |= middle_and_final_level_search
                (CHEWING_L, phrase_length, keys, ranges);

            if (m_options & PINYIN_AMB_L_N)
                result |= middle_and_final_level_search
                    (CHEWING_N, phrase_length, keys,ranges);

            if (m_options & PINYIN_AMB_L_R)
                result |= middle_and_final_level_search
                    (CHEWING_R, phrase_length, keys, ranges);
            return result;
        }
    default:
        {
            result |= middle_and_final_level_search
                ((ChewingInitial) first_key.m_initial,
                 phrase_length, keys, ranges);
            return result;
        }
    }
#undef MATCH
    return result;
}


int ChewingBitmapIndexLevel::middle_and_final_level_search
(ChewingInitial initial, int phrase_length, /* in */ ChewingKey keys[],
 /* out */ PhraseIndexRanges ranges) const {

/* macros */
#define MATCH(AMBIGUITY, ORIGIN, ANOTHER) case ORIGIN:                  \
    {                                                                   \
        result = tone_level_search                                      \
            (initial, middle,                                           \
             ORIGIN, phrase_length, keys, ranges);                      \
        if (m_options & AMBIGUITY) {                                    \
            result |= tone_level_search                                 \
                (initial, middle,                                        \
                 ANOTHER, phrase_length, keys, ranges);                 \
        }                                                               \
        return result;                                                  \
    }

    int result = SEARCH_NONE;
    const ChewingKey & first_key = keys[0];
    const ChewingMiddle middle = (ChewingMiddle)first_key.m_middle;

    switch(first_key.m_final) {
    case CHEWING_ZERO_FINAL:
        {
            if (middle == CHEWING_ZERO_MIDDLE) { /* in-complete pinyin */
                if (!(m_options & PINYIN_INCOMPLETE))
                    return result;
                for (int m = CHEWING_I; m < CHEWING_NUMBER_OF_MIDDLES; ++m)
                    for (int n = CHEWING_A; n < CHEWING_NUMBER_OF_FINALS;
                         ++n) {
                        result |= tone_level_search
                            (initial, (ChewingMiddle) m, (ChewingFinal) n,
                             phrase_length, keys, ranges);
                    }
                return result;
            } else { /* normal pinyin */
                result |= tone_level_search
                    (initial, middle, (ChewingFinal)first_key.m_final,
                     phrase_length, keys, ranges);
                return result;
            }
        }

        MATCH(PINYIN_AMB_AN_ANG, CHEWING_AN, CHEWING_ANG);
	MATCH(PINYIN_AMB_AN_ANG, CHEWING_ANG, CHEWING_AN);
	MATCH(PINYIN_AMB_EN_ENG, CHEWING_EN, CHEWING_ENG);
	MATCH(PINYIN_AMB_EN_ENG, CHEWING_ENG, CHEWING_EN);
	MATCH(PINYIN_AMB_IN_ING, PINYIN_IN, PINYIN_ING);
	MATCH(PINYIN_AMB_IN_ING, PINYIN_ING, PINYIN_IN);

    default:
        {
            result |= tone_level_search
                (initial, middle, (ChewingFinal) first_key.m_final,
                 phrase_length, keys, ranges);
            return result;
        }
    }
#undef MATCH
    return result;
}


int ChewingBitmapIndexLevel::tone_level_search
(ChewingInitial initial, ChewingMiddle middle, ChewingFinal final,
 int phrase_length, /* in */ ChewingKey keys[],
 /* out */ PhraseIndexRanges ranges) const {

    int result = SEARCH_NONE;
    const ChewingKey & first_key = keys[0];

    switch (first_key.m_tone) {
    case CHEWING_ZERO_TONE:
        {
            /* deal with zero tone in chewing large table. */
            for (int i = CHEWING_ZERO_TONE; i < CHEWING_NUMBER_OF_TONES; ++i) {
                ChewingLengthIndexLevel * phrases =
                    m_chewing_length_indexes
                    [initial][middle][final][(ChewingTone)i];
                if (phrases)
                    result |= phrases->search
                        (m_options, phrase_length - 1, keys + 1, ranges);
            }
            return result;
        }
    default:
        {
            ChewingLengthIndexLevel * phrases =
                m_chewing_length_indexes
                [initial][middle][final][CHEWING_ZERO_TONE];
            if (phrases)
                result |= phrases->search
                    (m_options, phrase_length - 1, keys + 1, ranges);

            phrases = m_chewing_length_indexes
                [initial][middle][final][(ChewingTone) first_key.m_tone];
            if (phrases)
                result |= phrases->search
                    (m_options, phrase_length - 1, keys + 1, ranges);
            return result;
        }
    }
    return result;
}


int ChewingLengthIndexLevel::search(pinyin_option_t options, int phrase_length,
                                    /* in */ ChewingKey keys[],
                                    /* out */ PhraseIndexRanges ranges) {
    assert(FALSE);
}

ChewingLengthIndexLevel::~ChewingLengthIndexLevel() {
    assert(FALSE);
}
