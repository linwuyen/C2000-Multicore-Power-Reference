#ifndef MULTICORE_REFERENCE_H
#define MULTICORE_REFERENCE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MULTICORE_FRAME_MAGIC (0x4D435055UL)
#define MULTICORE_SAFE_ZERO_Q15 (0)

typedef enum
{
    MULTICORE_COMMAND_SAFE_ZERO = 0,
    MULTICORE_COMMAND_SET_REFERENCE = 1
} MulticoreCommand;

typedef enum
{
    MULTICORE_ACCEPTED = 0,
    MULTICORE_REJECT_BAD_MAGIC = 1,
    MULTICORE_REJECT_BAD_CHECKSUM = 2,
    MULTICORE_REJECT_STALE_SEQUENCE = 3,
    MULTICORE_REJECT_BAD_COMMAND = 4
} MulticoreConsumeResult;

typedef struct
{
    uint32_t magic;
    uint32_t sequence;
    uint16_t command;
    int16_t reference_q15;
    uint16_t flags;
    uint16_t reserved;
    uint32_t checksum;
} MulticoreFrame;

typedef struct
{
    uint32_t next_sequence;
} MulticoreCpu1State;

typedef struct
{
    uint32_t last_sequence;
    int16_t applied_reference_q15;
    uint32_t accepted_count;
    uint32_t rejected_count;
    bool safe_zero_active;
    bool has_sequence;
} MulticoreCpu2State;

uint32_t MulticoreFrame_checksum(const MulticoreFrame *frame);

void MulticoreCpu1_init(MulticoreCpu1State *state, uint32_t first_sequence);

MulticoreFrame MulticoreCpu1_buildFrame(
    MulticoreCpu1State *state,
    MulticoreCommand command,
    int16_t reference_q15,
    uint16_t flags);

void MulticoreCpu2_init(MulticoreCpu2State *state);

MulticoreConsumeResult MulticoreCpu2_consumeFrame(
    MulticoreCpu2State *state,
    const MulticoreFrame *frame);

#ifdef __cplusplus
}
#endif

#endif
