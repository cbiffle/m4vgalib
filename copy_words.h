#ifndef COPY_WORDS_H
#define COPY_WORDS_H

#include "lib/armv7m/types.h"

/*
 * Moves some number of aligned words using the fastest method I could think up.
 */
void copy_words(armv7m::Word const *source,
                armv7m::Word *dest,
                armv7m::Word count);

#endif  // COPY_WORDS_H
