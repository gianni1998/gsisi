#pragma once

#include "IR.hpp"

class IROptimiser {
public:
    void optimise(IRProgram& program) const;

private:
    void constantFold(IRFunction& function) const;
    void simplifyBranches(IRFunction& function) const;
    void removeUnreachableBlocks(IRFunction& function) const;
    void removeDeadInstructions(IRFunction& function) const;

    static bool producesSideEffects(const IRInstruction& instruction) noexcept;
};