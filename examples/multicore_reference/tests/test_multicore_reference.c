#include "multicore_reference.h"

#include <assert.h>
#include <stdio.h>

static void testValidReferenceAndSafeZero(void)
{
    MulticoreCpu1State cpu1;
    MulticoreCpu2State cpu2;
    MulticoreFrame frame;

    MulticoreCpu1_init(&cpu1, 100U);
    MulticoreCpu2_init(&cpu2);
    assert(cpu2.safe_zero_active);

    frame = MulticoreCpu1_buildFrame(
        &cpu1,
        MULTICORE_COMMAND_SET_REFERENCE,
        12345,
        0x0001U);
    assert(frame.sequence == 100U);
    assert(MulticoreCpu2_consumeFrame(&cpu2, &frame) == MULTICORE_ACCEPTED);
    assert(!cpu2.safe_zero_active);
    assert(cpu2.applied_reference_q15 == 12345);

    frame = MulticoreCpu1_buildFrame(
        &cpu1,
        MULTICORE_COMMAND_SAFE_ZERO,
        22222,
        0U);
    assert(frame.reference_q15 == MULTICORE_SAFE_ZERO_Q15);
    assert(MulticoreCpu2_consumeFrame(&cpu2, &frame) == MULTICORE_ACCEPTED);
    assert(cpu2.safe_zero_active);
    assert(cpu2.applied_reference_q15 == MULTICORE_SAFE_ZERO_Q15);
    assert(cpu2.accepted_count == 2U);
}

static void testCorruptionFailsSafe(void)
{
    MulticoreCpu1State cpu1;
    MulticoreCpu2State cpu2;
    MulticoreFrame frame;

    MulticoreCpu1_init(&cpu1, 1U);
    MulticoreCpu2_init(&cpu2);
    frame = MulticoreCpu1_buildFrame(
        &cpu1,
        MULTICORE_COMMAND_SET_REFERENCE,
        16000,
        0U);
    frame.reference_q15++;

    assert(
        MulticoreCpu2_consumeFrame(&cpu2, &frame) ==
        MULTICORE_REJECT_BAD_CHECKSUM);
    assert(cpu2.safe_zero_active);
    assert(cpu2.applied_reference_q15 == MULTICORE_SAFE_ZERO_Q15);
    assert(cpu2.rejected_count == 1U);
}

static void testStaleSequenceFailsSafe(void)
{
    MulticoreCpu1State cpu1;
    MulticoreCpu2State cpu2;
    MulticoreFrame first;
    MulticoreFrame second;

    MulticoreCpu1_init(&cpu1, 50U);
    MulticoreCpu2_init(&cpu2);
    first = MulticoreCpu1_buildFrame(
        &cpu1,
        MULTICORE_COMMAND_SET_REFERENCE,
        1000,
        0U);
    second = MulticoreCpu1_buildFrame(
        &cpu1,
        MULTICORE_COMMAND_SET_REFERENCE,
        2000,
        0U);

    assert(MulticoreCpu2_consumeFrame(&cpu2, &second) == MULTICORE_ACCEPTED);
    assert(
        MulticoreCpu2_consumeFrame(&cpu2, &first) ==
        MULTICORE_REJECT_STALE_SEQUENCE);
    assert(cpu2.safe_zero_active);
    assert(cpu2.applied_reference_q15 == MULTICORE_SAFE_ZERO_Q15);
}

static void testBadCommandFailsSafe(void)
{
    MulticoreCpu1State cpu1;
    MulticoreCpu2State cpu2;
    MulticoreFrame frame;

    MulticoreCpu1_init(&cpu1, 7U);
    MulticoreCpu2_init(&cpu2);
    frame = MulticoreCpu1_buildFrame(
        &cpu1,
        MULTICORE_COMMAND_SET_REFERENCE,
        123,
        0U);
    frame.command = 99U;
    frame.checksum = MulticoreFrame_checksum(&frame);

    assert(
        MulticoreCpu2_consumeFrame(&cpu2, &frame) ==
        MULTICORE_REJECT_BAD_COMMAND);
    assert(cpu2.safe_zero_active);
}

int main(void)
{
    testValidReferenceAndSafeZero();
    testCorruptionFailsSafe();
    testStaleSequenceFailsSafe();
    testBadCommandFailsSafe();
    puts("PASS: public multicore reference");
    return 0;
}
