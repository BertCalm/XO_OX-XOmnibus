/*
    XOmnibus Test Runner
    ====================
    Main entry point that runs all test suites and reports aggregate results.
    Returns 0 on success (all tests pass), non-zero on failure.
*/

#include "DSPTests/DSPStabilityTests.h"
#include "CouplingTests/CouplingMatrixTests.h"
#include "PresetTests/PresetRoundTripTests.h"

#include <juce_core/juce_core.h>
#include <iostream>

int main()
{
    // Initialize JUCE's message manager (required for some JUCE types)
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::cout << "\n";
    std::cout << "##################################################\n";
    std::cout << "#          XOmnibus Test Suite                    #\n";
    std::cout << "##################################################\n";

    int totalFailures = 0;

    totalFailures += dsp_tests::runAll();
    totalFailures += coupling_tests::runAll();
    totalFailures += preset_tests::runAll();

    std::cout << "\n##################################################\n";
    if (totalFailures == 0)
    {
        std::cout << "#  ALL TESTS PASSED                              #\n";
    }
    else
    {
        std::cout << "#  " << totalFailures << " TEST(S) FAILED"
                  << std::string(38 - std::to_string(totalFailures).length(), ' ')
                  << "#\n";
    }
    std::cout << "##################################################\n\n";

    return totalFailures;
}
