#include <gtest/gtest.h>

#include "DRAMPower/command/Command.h"

#include <DRAMPower/standards/lpddr4/LPDDR4.h>

#include <memory>

#include <fstream>


using namespace DRAMPower;

class DramPowerTest_LPDDR4_5 : public ::testing::Test {
protected:
    // Test pattern
    std::vector<Command> testPattern = {
            {   0, CmdType::ACT,  { 0, 0, 0 }},
            {   15, CmdType::RD,  { 0, 0, 0 }},
            {   20, CmdType::ACT,  { 3, 0, 0 }},
            {   35, CmdType::RD,  { 3, 0, 0 }},
            {   40, CmdType::RD,  { 0, 0, 0 }},
            {   50, CmdType::PREA,  { 0, 0, 0 }},
            {   65, CmdType::REFA,  { 0, 0, 0 }},
            { 100, CmdType::END_OF_SIMULATION },
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

TEST_F(DramPowerTest_LPDDR4_5, Counters_and_Cycles){
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto stats = ddr->getStats();

    // Check bank command count: ACT
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0 || b == 3)
            ASSERT_EQ(stats.bank[b].counter.act, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.act, 0);
    }

    // Check bank command count: RD
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if (b == 0)
            ASSERT_EQ(stats.bank[b].counter.reads, 2);
        else if( b == 3)
            ASSERT_EQ(stats.bank[b].counter.reads, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.reads, 0);
    };

    // Check bank command count: PRE
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0 || b == 3)
            ASSERT_EQ(stats.bank[b].counter.pre, 1);
        else
            ASSERT_EQ(stats.bank[b].counter.pre, 0);
    }

    // Check bank command count: REFA
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        ASSERT_EQ(stats.bank[b].counter.refAllBank, 1);
    }

    // Check cycles count
    ASSERT_EQ(stats.total.cycles.act, 75);
    ASSERT_EQ(stats.total.cycles.pre, 25);

    // Check bank specific ACT cycle count;
    for(auto b = 0; b < ddr->memSpec.numberOfBanks; b++){
        if (b == 0)
            ASSERT_EQ(stats.bank[b].cycles.act, 75);
        else if(b == 3)
            ASSERT_EQ(stats.bank[b].cycles.act, 55);
        else
            ASSERT_EQ(stats.bank[b].cycles.act, 25);
    }

    // Check bank specific PRE cycle count
    for(auto b = 1; b < ddr->memSpec.numberOfBanks; b++){
        if(b == 0)
            ASSERT_EQ(stats.bank[b].cycles.pre, 25);
        else if (b == 3)
            ASSERT_EQ(stats.bank[b].cycles.pre, 45);
        else
            ASSERT_EQ(stats.bank[b].cycles.pre, 75);
    }
}

TEST_F(DramPowerTest_LPDDR4_5, Energy) {
    for (const auto& command : testPattern) {
        ddr->doCommand(command);
    }

    auto energy = ddr->calcEnergy(testPattern.back().timestamp);
    auto total_energy = energy.total_energy();

    ASSERT_EQ(std::round(total_energy.E_act), 392);
    ASSERT_EQ(std::round(total_energy.E_pre), 415);
    ASSERT_EQ(std::round(total_energy.E_RD), 1357);
    ASSERT_EQ(std::round(total_energy.E_ref_AB), 1699);
    ASSERT_EQ(std::round(total_energy.E_ref_SB), 0);
    ASSERT_EQ(std::round(total_energy.E_ref_PB), 0);
    ASSERT_EQ(std::round(total_energy.E_ref_2B), 0);
    ASSERT_EQ(std::round(total_energy.E_bg_act), 2657);
    ASSERT_EQ(std::round(energy.E_bg_act_shared), 2380);
    ASSERT_EQ(std::round(total_energy.E_bg_pre), 779);
    ASSERT_EQ(std::round(total_energy.total()), 7299);
}
