#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr5/LPDDR5.h>

#include <memory>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_LPDDR5_7 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::SREFEN,  { 0, 0, 0 }},
            {   40, CmdType::SREFEX,  { 0, 0, 0 }},
            { 100, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::LPDDR5> ddr;

    virtual void SetUp()
    {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "lpddr5.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }
        json data = json::parse(f);
        MemSpecLPDDR5 memSpec(data["memspec"]);

        ddr = std::make_unique<LPDDR5>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR5_7, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto stats = ddr->getStats();

    // Check bank command count: ACT
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].counter.act, 0);

    // Check bank command count: REFA
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].counter.refAllBank, 1);


    // Check cycles count
    ASSERT_EQ(stats.total.cycles.act, 25);
    ASSERT_EQ(stats.total.cycles.pre, 60);
    ASSERT_EQ(stats.total.cycles.selfRefresh, 15);

    // Check bank specific ACT cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.act, 25);

    // Check bank specific PRE cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.pre, 60);

    // Check bank specific SREF cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.selfRefresh, 15);

}

TEST_F(DramPowerTest_LPDDR5_7, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto energy = ddr->calcEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();

    ASSERT_EQ(std::round(total_energy.E_act), 0);
    ASSERT_EQ(std::round(total_energy.E_pre), 0);
    ASSERT_EQ(std::round(total_energy.E_ref_AB), 1699);
    ASSERT_EQ(std::round(energy.E_sref), 280);
    ASSERT_EQ(std::round(total_energy.E_bg_act), 1024);
    ASSERT_EQ(std::round(energy.E_bg_act_shared), 793);
    ASSERT_EQ(std::round(total_energy.E_bg_pre), 1869);
    ASSERT_EQ(std::round(total_energy.total() + energy.E_sref), 4873);
}
