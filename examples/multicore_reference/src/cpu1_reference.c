#include "multicore_reference.h"

#include <string.h>

void MulticoreCpu1_init(MulticoreCpu1State *state, uint32_t first_sequence)
{
    if (state != NULL)
    {
        state->next_sequence = first_sequence;
    }
}

MulticoreFrame MulticoreCpu1_buildFrame(
    MulticoreCpu1State *state,
    MulticoreCommand command,
    int16_t reference_q15,
    uint16_t flags)
{
    MulticoreFrame frame;
    (void)memset(&frame, 0, sizeof(frame));

    frame.magic = MULTICORE_FRAME_MAGIC;
    frame.sequence = (state != NULL) ? state->next_sequence : 0UL;
    frame.command = (uint16_t)command;
    frame.reference_q15 =
        (command == MULTICORE_COMMAND_SAFE_ZERO) ? MULTICORE_SAFE_ZERO_Q15 : reference_q15;
    frame.flags = flags;
    frame.checksum = MulticoreFrame_checksum(&frame);

    if (state != NULL)
    {
        state->next_sequence++;
    }
    return frame;
}
