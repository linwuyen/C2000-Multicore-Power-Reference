#include "multicore_reference.h"

#include <stddef.h>

static uint32_t mixByte(uint32_t hash, uint8_t value)
{
    return (hash ^ value) * 16777619UL;
}

static uint32_t mix16(uint32_t hash, uint16_t value)
{
    hash = mixByte(hash, (uint8_t)(value & 0xFFU));
    return mixByte(hash, (uint8_t)((value >> 8) & 0xFFU));
}

static uint32_t mix32(uint32_t hash, uint32_t value)
{
    hash = mix16(hash, (uint16_t)(value & 0xFFFFUL));
    return mix16(hash, (uint16_t)((value >> 16) & 0xFFFFUL));
}

uint32_t MulticoreFrame_checksum(const MulticoreFrame *frame)
{
    uint32_t hash = 2166136261UL;

    if (frame == NULL)
    {
        return 0UL;
    }

    hash = mix32(hash, frame->magic);
    hash = mix32(hash, frame->sequence);
    hash = mix16(hash, frame->command);
    hash = mix16(hash, (uint16_t)frame->reference_q15);
    hash = mix16(hash, frame->flags);
    hash = mix16(hash, frame->reserved);
    return hash;
}
