#include "ChipData.h"
#include "Decoder.h"
#include <gtest/gtest.h>
#include <stdexcept>

namespace {

TEST(SubroutineInstructions, CallSavesReturnAddressAndJumps) {
    ChipData data;
    Decoder decoder;
    data.programCounter = 0x202;

    auto command = decoder.decode(0x2345);
    command.execute(data);

    EXPECT_EQ(data.stack[0], 0x202);
    EXPECT_EQ(data.stackPointer, 1);
    EXPECT_EQ(data.programCounter, 0x345);
}

TEST(SubroutineInstructions, ReturnPopsAddress) {
    ChipData data;
    Decoder decoder;
    data.stack[0] = 0x202;
    data.stackPointer = 1;
    data.programCounter = 0x346;

    auto command = decoder.decode(0x00EE);
    command.execute(data);

    EXPECT_EQ(data.stackPointer, 0);
    EXPECT_EQ(data.programCounter, 0x202);
}

TEST(SubroutineInstructions, NestedCallsReturnInReverseOrder) {
    ChipData data;
    Decoder decoder;
    auto outerCall = decoder.decode(0x2300);
    auto innerCall = decoder.decode(0x2400);
    auto returnCommand = decoder.decode(0x00EE);

    data.programCounter = 0x202;
    outerCall.execute(data);     // Jump to 0x300; push 0x202.
    data.programCounter = 0x304; // Simulate fetching the nested call.
    innerCall.execute(data);     // Jump to 0x400; push 0x302

    ASSERT_EQ(data.stackPointer, 2);
    EXPECT_EQ(data.programCounter, 0x400);

    returnCommand.execute(data);
    EXPECT_EQ(data.programCounter, 0x304);
    EXPECT_EQ(data.stackPointer, 1);

    returnCommand.execute(data);
    EXPECT_EQ(data.programCounter, 0x202);
    EXPECT_EQ(data.stackPointer, 0);
}

TEST(SubroutineInstructions, LastStackSlotIsUsable) {
    ChipData data;
    Decoder decoder;
    data.stackPointer = 15;
    data.programCounter = 0xABC;

    auto call = decoder.decode(0x2300);
    call.execute(data);
    EXPECT_EQ(data.stack[15], 0xABC);
    EXPECT_EQ(data.stackPointer, 16);
    EXPECT_EQ(data.programCounter, 0x300);

    auto returnCommand = decoder.decode(0x00EE);
    returnCommand.execute(data);
    EXPECT_EQ(data.stackPointer, 15);
    EXPECT_EQ(data.programCounter, 0xABC);
}

TEST(SubroutineInstructions, FullStackRejectsCallWithoutChangingState) {
    ChipData data;
    Decoder decoder;
    data.stack.fill(0xABC);
    data.stackPointer = 16;
    data.programCounter = 0x202;
    const auto originalStack = data.stack;

    auto command = decoder.decode(0x2300);
    EXPECT_THROW(command.execute(data), std::overflow_error);
    EXPECT_EQ(data.stack, originalStack);
    EXPECT_EQ(data.stackPointer, 16);
    EXPECT_EQ(data.programCounter, 0x202);
}

TEST(SubroutineInstructions, EmptyStackRejectsReturnWithoutChangingState) {
    ChipData data;
    Decoder decoder;
    data.programCounter = 0x202;
    const auto originalStack = data.stack;

    auto command = decoder.decode(0x00EE);
    EXPECT_THROW(command.execute(data), std::underflow_error);
    EXPECT_EQ(data.stack, originalStack);
    EXPECT_EQ(data.stackPointer, 0);
    EXPECT_EQ(data.programCounter, 0x202);
}

} // namespace
