#include "bank.h"
#include "../simpleini/SimpleIni.h"
#include <string>
#include <stdint.h>
#include <string.h>

static CSimpleIniA the_ini;
static std::map<uint32_t, Bank> the_melodic_banks;
static std::map<uint32_t, Bank> the_percussive_banks;

static void process_melodic_bank(Mode mode, unsigned bankno, const char *bank_name)
{
    const CSimpleIniA &ini = ::the_ini;
    std::map<uint32_t, Bank> &bank_set = ::the_melodic_banks;

    unsigned bank_lsb = bankno % 128;
    unsigned bank_msb = bankno / 128;
    if (bank_msb >= 128 || bank_lsb >= 128)
        throw std::runtime_error("Invalid bank number");

    const CSimpleIniA::TKeyVal *patchsec = ini.GetSection(bank_name);
    if (!patchsec)
        throw std::runtime_error("Patch section not found");

    for (auto i = patchsec->begin(), e = patchsec->end(); i != e; ++i) {
        const char *item = i->first.pItem;
        const char *patch_name = i->second;
        unsigned patchno;
        unsigned count;
        if (sscanf(item, "%u%n", &patchno, &count) == 1 && count == strlen(item)) {
            if (patchno >= 128)
                throw std::runtime_error("Invalid patch number");
            Bank &bank = bank_set[bankno];
            bank.bank_msb = bank_msb;
            bank.bank_lsb = bank_lsb;
            bank.bank_name = bank_name;
            bank.bank_ins[patchno].ins_number = patchno;
            bank.bank_ins[patchno].ins_name = patch_name;
        }
    }
}

static void process_melodics(Mode mode, const char *set_name)
{
    const CSimpleIniA &ini = ::the_ini;

    const CSimpleIniA::TKeyVal *melosec = ini.GetSection(set_name);
    if (!melosec)
        throw std::runtime_error("Melodic section not found");

    for (auto i = melosec->begin(), e = melosec->end(); i != e; ++i) {
        const char *item = i->first.pItem;
        const char *bank_name = i->second;
        unsigned bankno;
        unsigned count;
        if (sscanf(item, "Patch[%u]%n", &bankno, &count) == 1 && count == strlen(item))
            process_melodic_bank(mode, bankno, bank_name);
    }
}

static void process_drum_bank(Mode mode, unsigned bankno1, unsigned bankno2, const char *bank_name)
{
    const CSimpleIniA &ini = ::the_ini;
    std::map<uint32_t, Bank> &bank_set = ::the_percussive_banks;

    unsigned bank_msb = bankno1;
    unsigned bank_pgm = bankno2;

    if (mode == Mode::MU1000 || mode == Mode::SonarXG) {
        if (bankno1 % 128 != 0)
            throw std::runtime_error("Drum bank number invalid");
        bank_msb = bankno1 / 128;
    }

    if (bank_msb >= 128 || bank_pgm >= 128)
        throw std::runtime_error("Drum bank number invalid");

    const CSimpleIniA::TKeyVal *patchsec = ini.GetSection(bank_name);
    if (!patchsec)
        throw std::runtime_error("Patch section not found");

    for (auto i = patchsec->begin(), e = patchsec->end(); i != e; ++i) {
        const char *item = i->first.pItem;
        const char *patch_name = i->second;
        unsigned patchno;
        unsigned count;
        if (sscanf(item, "%u%n", &patchno, &count) == 1 && count == strlen(item)) {
            if (patchno >= 128)
                throw std::runtime_error("Invalid patch number");
            Bank &bank = bank_set[bank_msb * 128 + bank_pgm];
            bank.bank_msb = bank_msb;
            bank.bank_lsb = bank_pgm;
            bank.bank_name = bank_name;
            bank.bank_ins[patchno].ins_number = patchno;
            bank.bank_ins[patchno].ins_name = patch_name;
        }
    }
}

static void process_drums(Mode mode, const char *set_name)
{
    const CSimpleIniA &ini = ::the_ini;

    const CSimpleIniA::TKeyVal *drumsec = ini.GetSection(set_name);
    if (!drumsec)
        throw std::runtime_error("Drum section not found");

    for (auto i = drumsec->begin(), e = drumsec->end(); i != e; ++i) {
        const char *item = i->first.pItem;
        const char *bank_name = i->second;
        unsigned bankno1;
        unsigned bankno2;
        unsigned count;
        if (sscanf(item, "Key[%u,%u]%n", &bankno1, &bankno2, &count) == 2 && count == strlen(item))
            process_drum_bank(mode, bankno1, bankno2, bank_name);
    }
}

static std::string cpp_escape_string(const std::string &str)
{
    std::string result;
    result.reserve(str.size() * 2);
    for (char c: str) {
        if (c == '"')
            result.push_back('\\');
        result.push_back(c);
    }
    return result;
}

static void print_instruments(char indicator, const std::map<uint32_t, Bank> &banks, Mode mode)
{
    for (auto i = banks.begin(), e = banks.end(); i != e; ++i) {
        uint32_t bankno = i->first;
        const Bank &bank = i->second;

        uint32_t bank_msb = bankno / 128;
        uint32_t bank_lsb = bankno % 128;

        if ((mode == Mode::MU1000 || mode == Mode::SonarXG) && indicator == 'P' /* && bankno != 0 */) {
            bank_msb = 127 - bank_msb;
        }

        for (unsigned n = 0; n < 128; ++n) {
            if (!bank.bank_ins[n])
                continue;

            if (mode != Mode::GM && bankno == 0)
            {
                if (indicator == 'M')
                    continue;
                if (indicator == 'P' && (n >= 35 || n <= 81))
                    continue;
            }

            std::string bank_name = bank.bank_name;
            if (mode == Mode::MU1000) {
                static const std::string &prefix = "YAMAHA MU1000/MU2000 ";
                if (bank_name.size() >= prefix.size() && !memcmp(bank_name.data(), prefix.data(), prefix.size()))
                    bank_name = "XG " + bank_name.substr(prefix.size());
            }
            else if (mode == Mode::MSGS) {
                static const std::string &prefix = "Microsoft GS ";
                if (bank_name.size() >= prefix.size() && !memcmp(bank_name.data(), prefix.data(), prefix.size()))
                    bank_name = "GS " + bank_name.substr(prefix.size());
            }
            else if (mode == Mode::GM) {
                static const std::string &prefix = "General MIDI Level 2 ";
                if (bank_name.size() >= prefix.size() && !memcmp(bank_name.data(), prefix.data(), prefix.size()))
                    bank_name = "GM2 " + bank_name.substr(prefix.size());
            }
            else if (mode == Mode::SC88) {
                static const std::string &prefix = "Roland SC-";
                if (bank_name.size() >= prefix.size() && !memcmp(bank_name.data(), prefix.data(), prefix.size()))
                    bank_name = "SC-" + bank_name.substr(prefix.size());
            }

            printf("    {'%c', %3d, %3d, %3d, \"%s\", \"%s\"},\n",
                   indicator,
                   bank_msb, bank_lsb, n,
                   cpp_escape_string(bank_name).c_str(),
                   cpp_escape_string(bank.bank_ins[n].ins_name).c_str());
        }
    }
}

int main(int argc, char *argv[])
{
    CSimpleIniA &ini = ::the_ini;

    if (argc != 2) {
        fprintf(stderr, "Bad arguments\n");
        return 1;
    }

    Mode mode = (Mode)-1;

    if (!strcmp(argv[1], "gm"))
        mode = Mode::GM;
    else if (!strcmp(argv[1], "xg"))
        mode = Mode::MU1000;
    else if (!strcmp(argv[1], "gs"))
        mode = Mode::MSGS;
    else if (!strcmp(argv[1], "sc"))
        mode = Mode::SC88;
    else if (!strcmp(argv[1], "sonar-xg"))
        mode = Mode::SonarXG;
    else if (!strcmp(argv[1], "sonar-gs"))
        mode = Mode::SonarGS;
    else {
        fprintf(stderr, "Bad mode name\n");
        return 1;
    }

    if (mode == Mode::GM) {
        if (ini.LoadFile("instrument/GM1_GM2.ins") != SI_OK)
            return 1;
        process_melodics(mode, "General MIDI Level 2");
        process_drums(mode, "General MIDI Level 2 Drumsets");
    }
    else if (mode == Mode::MU1000) {
        if (ini.LoadFile("instrument/YAMAHA_MU1000_MU2000.ins") != SI_OK)
            return 1;
        process_melodics(mode, "YAMAHA MU1000/MU2000");
        process_drums(mode, "YAMAHA MU1000/MU2000 Drumsets");
    }
    else if (mode == Mode::MSGS) {
        if (ini.LoadFile("instrument/Microsoft_GS_Wavetable_Synth.ins") != SI_OK)
            return 1;
        process_melodics(mode, "Microsoft GS Wavetable Synth");
        process_drums(mode, "Microsoft GS Wavetable Synth Drumsets");
    }
    else if (mode == Mode::SC88) {
        if (ini.LoadFile("instrument/Roland_SC-8850.ins") != SI_OK)
            return 1;
        process_melodics(mode, "Roland SC-8850");
        process_drums(mode, "Roland SC-8850 Drumsets");
    }
    else if (mode == Mode::SonarXG) {
        if (ini.LoadFile("instrument/Sonar.ins") != SI_OK)
            return 1;
        process_melodics(mode, "Yamaha XG");
        process_drums(mode, "Yamaha XG Drum Kits");
    }
    else if (mode == Mode::SonarGS) {
        if (ini.LoadFile("instrument/Sonar.ins") != SI_OK)
            return 1;
        process_melodics(mode, "Roland GS");
        process_drums(mode, "Roland GS Drumsets");
    }

    print_instruments('M', ::the_melodic_banks, mode);
    print_instruments('P', ::the_percussive_banks, mode);

    return 0;
}
