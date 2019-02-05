#pragma once

enum class Mode
{
    GM,
    MU1000,
    MSGS,
    SC88,
    SonarXG,
    SonarGS,
};

struct Instrument
{
    unsigned ins_number = ~0u;
    const char *ins_name = nullptr;
    operator bool() const noexcept { return ins_number != ~0u; }
};

struct Bank
{
    unsigned bank_msb = ~0u;
    unsigned bank_lsb = ~0u;
    const char *bank_name = nullptr;
    Instrument bank_ins[128];
    operator bool() const noexcept { return bank_msb != ~0u && bank_lsb != ~0u; }
};
