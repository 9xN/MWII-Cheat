#pragma once
#include "sdk.h"

// If the sigs return nothing, try removing a few bytes and searching again. sometimes things break.

namespace offsets {
    static uintptr_t refdef = 0x130AD430; // Sig: 33 05 ?? ?? ?? ?? 89 44 24 24 48 83 7C 24 20 00
    static uintptr_t name_array = 0x130CC9A0; // Sig: 40 53 48 83 EC 20 8B D9 E8 ? ? ? ? 48 63 D0 48 8D 0D ? ? ? ? 48 8B 0C D1 8B D3 48 8B 01 FF 90 ? ? ? ? 8B 40 70 C1 E8 08 25 ? ? ? ? 48 83 C4 20 5B C3
    static uintptr_t name_array_pos = 0x5E70;
    static uintptr_t loot_ptr = 0xB39FE8C; // For loot esp, you can add it if you want 
    static uintptr_t name_size = 0xD0;
    static uintptr_t camera_base = 0x136CCE40;
    static uintptr_t camera_pos = 0x1F8;
    static uintptr_t local_index = 0xF3F50;
    static uintptr_t local_index_pos = 0x2D0;
    static uintptr_t recoil = 0xED7F0;
    static uintptr_t game_mode = 0xF8DCF58; // Sig: 3B 0D ?? ?? ?? ?? 0F 47 0D ?? ?? ?? ?? 41 89 4D 24 or 48 8d 0d ? ? ? ? c6 05 ? ? ? ? ? e8 ? ? ? ? 48 8d 15 + 0x198
    static uintptr_t weapon_definitions = 0x1302C500; // Can be used to make nospread, I don't have the sig for it.
    static uintptr_t distribute = 0xB3687B8;
    static uintptr_t visible_offset = 0xA80;
    static uintptr_t visible = 0x233B700;
    namespace player {
        static uintptr_t size = 0x6948;
        static uintptr_t valid = 0x949;
        static uintptr_t pos = 0x1138;
        static uintptr_t team = 0xC71; // Sig: 3A 88 ? ? ? ? 0F 84 ? ? ? ? 80 3D ? ? ? ? ?
        static uintptr_t weapon_index = 0x1394;
        static uintptr_t dead_1 = 0x91D;
        static uintptr_t dead_2 = 0x40;
    }
    namespace bone {
        static uintptr_t base_pos = 0x3CCA8; // Sig: C5 FB 10 86 ? ? ? ? C5 FB 11 85 ? ? ? ? 8B 86 ? ? ? ? 89 85 ? ? ? ?
        static uintptr_t index_struct_size = 0x180;
    }
}