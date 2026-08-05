#include "multicore_reference.h"

#include <string.h>

void MulticoreCpu2_init(MulticoreCpu2State *state)
{
    if (state == NULL)
    {
        return;
    }

    (void)memset(state, 0, sizeof(*state));
    state->safe_zero_active = true;
    state->applied_reference_q15 = MULTICORE_SAFE_ZERO_Q15;
}

static MulticoreConsumeResult reject(
    MulticoreCpu2State *state,
    MulticoreConsumeResult result)
{
    state->rejected_count++;
    state->safe_zero_active = true;
    state->applied_reference_q15 = MULTICORE_SAFE_ZERO_Q15;
    return result;
}

MulticoreConsumeResult MulticoreCpu2_consumeFrame(
    MulticoreCpu2State *state,
    const MulticoreFrame *frame)
{
    if ((state == NULL) || (frame == NULL))
    {
        return MULTICORE_REJECT_BAD_MAGIC;
    }
    if (frame->magic != MULTICORE_FRAME_MAGIC)
    {
        return reject(state, MULTICORE_REJECT_BAD_MAGIC);
    }
    if (frame->checksum != MulticoreFrame_checksum(frame))
    {
        return reject(state, MULTICORE_REJECT_BAD_CHECKSUM);
    }
    if (state->has_sequence && (frame->sequence <= state->last_sequence))
    {
        return reject(state, MULTICORE_REJECT_STALE_SEQUENCE);
    }
    if ((frame->command != (uint16_t)MULTICORE_COMMAND_SAFE_ZERO) &&
        (frame->command != (uint16_t)MULTICORE_COMMAND_SET_REFERENCE))
    {
        return reject(state, MULTICORE_REJECT_BAD_COMMAND);
    }

    state->last_sequence = frame->sequence;
    state->has_sequence = true;
    state->accepted_count++;

    if (frame->command == (uint16_t)MULTICORE_COMMAND_SAFE_ZERO)
    {
        state->safe_zero_active = true;
        state->applied_reference_q15 = MULTICORE_SAFE_ZERO_Q15;
    }
    else
    {
        state->safe_zero_active = false;
        state->applied_reference_q15 = frame->reference_q15;
    }

    return MULTICORE_ACCEPTED;
}
