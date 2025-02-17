#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr4/LPDDR4.h>

#include <memory>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_LPDDR4_8 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::PDEA,  { 0, 0, 0 }},
            {   30, CmdType::PDXA,  { 0, 0, 0 }},
            {   45, CmdType::PDEP,  { 0, 0, 0 }},
            {   70, CmdType::PDXP,  { 0, 0, 0 }},
            { 85, CmdType::END_OF_SIMULATION },
    };


    // Test variables
    std::unique_ptr<DRAMPower::LPDDR4> ddr;

    virtual void SetUp()
    {
        std::ifstream f(std::string(TEST_RESOURCE_DIR) + "lpddr4.json");

        if(!f.is_open()){
            std::cout << "Error: Could not open memory specification" << std::endl;
            exit(1);
        }
        json data = json::parse(f);
        MemSpecLPDDR4 memSpec(data["memspec"]);

        ddr = std::make_unique<LPDDR4>(memSpec);
    }

    virtual void TearDown()
    {
    }
};

TEST_F(DramPowerTest_LPDDR4_8, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto stats = ddr->getStats();


    // Check cycles count
    ASSERT_EQ(stats.total.cycles.act, 0);
    ASSERT_EQ(stats.total.cycles.pre, 30);
    ASSERT_EQ(stats.total.cycles.selfRefresh, 0);
    ASSERT_EQ(stats.total.cycles.powerDownAct, 30);
    ASSERT_EQ(stats.total.cycles.powerDownPre, 25);

    // Check bank specific ACT cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.act, 0);

    // Check bank specific PRE cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.pre, 30);

    // Check bank specific PDNA cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownAct, 30);

    // Check bank specific PDNP cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.powerDownPre, 25);

    // Check bank specific SREF cycle count
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++)  ASSERT_EQ(stats.bank[b].cycles.selfRefresh, 0);
}

TEST_F(DramPowerTest_LPDDR4_8, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto energy = ddr->calcEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();


    ASSERT_EQ(std::round(total_energy.E_act), 0);
    ASSERT_EQ(std::round(total_energy.E_pre), 0);
    ASSERT_EQ(std::round(energy.E_sref), 0);
    ASSERT_EQ(std::round(energy.E_PDNA), 623);
    ASSERT_EQ(std::round(energy.E_PDNP), 392);
    ASSERT_EQ(std::round(total_energy.E_bg_act), 0);
    ASSERT_EQ(std::round(energy.E_bg_act_shared), 0);
    ASSERT_EQ(std::round(total_energy.E_bg_pre), 935);
    ASSERT_EQ(std::round(total_energy.total() + energy.E_sref + energy.E_PDNA + energy.E_PDNP), 1950);
}
