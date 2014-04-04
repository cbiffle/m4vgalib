#ifndef COPY_WORDS_H
#define COPY_WORDS_H

#include "etl/armv7m/types.h"

/*
 * Moves some number of aligned words using the fastest method I could think up.
 */
void copy_words(etl::armv7m::Word const *source,
                etl::armv7m::Word *dest,
                etl::armv7m::Word count);

#endif  // COPY_WORDS_H
