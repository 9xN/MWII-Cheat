#include "sdk.h"
#include "Offsets.hpp"
#include "ImGui/imgui.h"
#include "driver.h"
using namespace driver;


namespace sdk {
    HANDLE	  process_id = NULL;
    uintptr_t module_base = NULL;
    uintptr_t peb = NULL;
    HWND      hwnd = NULL;
    uintptr_t client_info = NULL;
    uintptr_t client_info_base = NULL;
    uint64_t bone_base = NULL;
    uint64_t bone_index = NULL;
    uintptr_t currentvisoffset = NULL;
    uintptr_t last_visible_offset = NULL;

    BOOL CALLBACK enum_windows(HWND hwnd, LPARAM param) {
        DWORD process_id;
        GetWindowThreadProcessId(hwnd, &process_id);
        if (process_id == param)
        {
            sdk::hwnd = hwnd;
            return false;
        }
        return true;
    }

    void mousemove(float tarx, float tary, float X, float Y, int smooth)
    {
        float ScreenCenterX = (X / 2);
        float ScreenCenterY = (Y / 2);
        float TargetX = 0;
        float TargetY = 0;

        if (tarx != 0)
        {
            if (tarx > ScreenCenterX)
            {
                TargetX = -(ScreenCenterX - tarx);
                TargetX /= smooth;
                if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
            }

            if (tarx < ScreenCenterX)
            {
                TargetX = tarx - ScreenCenterX;
                TargetX /= smooth;
                if (TargetX + ScreenCenterX < 0) TargetX = 0;
            }
        }

        if (tary != 0)
        {
            if (tary > ScreenCenterY)
            {
                TargetY = -(ScreenCenterY - tary);
                TargetY /= smooth;
                if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
            }

            if (tary < ScreenCenterY)
            {
                TargetY = tary - ScreenCenterY;
                TargetY /= smooth;
                if (TargetY + ScreenCenterY < 0) TargetY = 0;
            }
        }
        mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(TargetX), static_cast<DWORD>(TargetY), NULL, NULL);
    }

    void set_game_hwnd() {
        EnumWindows(enum_windows, (LPARAM)sdk::process_id);
    }

    bool in_game() {
        return driver::read<int>(sdk::module_base + offsets::game_mode) > 1;
    }

    int player_count() {
        return driver::read<int>(sdk::module_base + offsets::game_mode);
    }

    int local_index() {
        auto local_index = driver::read<uintptr_t>(sdk::client_info + offsets::local_index);
        return driver::read<int>(local_index + offsets::local_index_pos); // 0x1fc
    }

#define BYTEn(x, n)   (*((BYTE*)&(x)+n))
#define BYTE1(x)   BYTEn(x,  1)

    uint64_t get_visible_base()
    {
        for (int32_t j{}; j <= 0x1770; ++j)
        {
            uint64_t vis_base_ptr = driver::read<uint64_t>(sdk::module_base + offsets::distribute) + (j * 0x190);
            uint64_t cmp_function = driver::read<uint64_t>(vis_base_ptr + 0x38);

            if (!cmp_function)
                continue;

            uint64_t about_visible = sdk::module_base + offsets::visible;

            if (cmp_function == about_visible)
            {
                sdk::currentvisoffset = vis_base_ptr;
                return sdk::currentvisoffset;
            }

        }
        return NULL;
    }

    bool is_visible(int entityNum) {

        if (!sdk::currentvisoffset)
            return false;

        uint64_t VisibleList = driver::read<uint64_t>(sdk::last_visible_offset + 0x80);
        if (!VisibleList)
            return false;
        uint64_t v421 = VisibleList + (entityNum * 9 + 0x152) * 8;
        if (!v421)
            return false;
        DWORD VisibleFlags = (v421 + 0x10) ^ driver::read<DWORD>(v421 + 0x14);
        if (!VisibleFlags)
            return false;
        DWORD v1630 = VisibleFlags * (VisibleFlags + 2);
        if (!v1630)
            return false;
        BYTE VisibleFlags1 = driver::read<DWORD>(v421 + 0x10) ^ v1630 ^ BYTE1(v1630);
        if (VisibleFlags1 == 3) {
            return true;
        }
        return false;
    }

    uint32_t player_t::getIndex()
    {
        return (address - sdk::module_base) / offsets::player::size;
    }

    NameEntry player_t::GetNameEntry(uint32_t index) {
        return driver::read<NameEntry>(sdk::GetNameList() + (index * offsets::name_size));
    }

    bool player_t::is_valid() {
        return driver::read<bool>(address + offsets::player::valid);
    }

    bool player_t::dead() {
        auto dead1 = driver::read<bool>(address + offsets::player::dead_1);
        auto dead2 = driver::read<bool>(address + offsets::player::dead_2);
        return dead1 || dead2;
    }

    int player_t::team_id() {
        return driver::read<int>(address + offsets::player::team);
    }

    vec3_t player_t::get_pos() {
        auto local_pos = driver::read<uintptr_t>(address + offsets::player::pos);
        return driver::read<vec3_t>(local_pos + 0x48);
    }    

    uintptr_t player_t::get_bone_ptr(uint64_t bone_base, uint64_t bone_index)
    {
        return driver::read<uintptr_t>(bone_base + (bone_index * offsets::bone::index_struct_size) + 0xD8);
    }

    vec3_t get_camera_position() {
        auto camera = driver::read<uintptr_t>(sdk::module_base + offsets::camera_base);
        if (!camera)
            return {};

        return driver::read<vec3_t>(camera + offsets::camera_pos);
    }

    vec2_t get_camera_angles() {
        auto camera = *(uintptr_t*)(sdk::module_base + offsets::camera_base);
        if (!camera)
            return {};
        return driver::read<vec2_t>(camera + offsets::camera_pos + 0xC);
    }

    uint64_t GetNameList() {
        auto ptr = driver::read<uint64_t>(sdk::module_base + offsets::name_array);
        return ptr + offsets::name_array_pos;
    }

    bool world_to_screen(vec3_t world_location, vec2_t& out, vec3_t camera_pos, int screen_width, int screen_height, vec2_t fov, vec3_t matricies[3]) {
        auto local = world_location - camera_pos;
        auto trans = vec3_t{
            local.dot(matricies[1]),
            local.dot(matricies[2]),
            local.dot(matricies[0])
        };

        if (trans.z < 0.01f) {
            return false;
        }

        out.x = ((float)screen_width / 2.0) * (1.0 - (trans.x / fov.x / trans.z));
        out.y = ((float)screen_height / 2.0) * (1.0 - (trans.y / fov.y / trans.z));

        if (out.x < 1 || out.y < 1 || (out.x > sdk::ref_def.width) || (out.y > sdk::ref_def.height)) {
            return false;
        }

        return true;
    }

    bool w2s(vec3_t world_position, vec2_t& screen_position) {
        return world_to_screen(world_position, screen_position, get_camera_position(), ref_def.width, ref_def.height, ref_def.view.tan_half_fov, ref_def.view.axis);
    }

    float units_to_m(float units) {
        return units * 0.0254;
    }

    ref_def_t ref_def;
}


float Distance3D(vec3_t point1, vec3_t point2)
{
    float distance = sqrt((point1.x - point2.x) * (point1.x - point2.x) +
        (point1.y - point2.y) * (point1.y - point2.y) +
        (point1.z - point2.z) * (point1.z - point2.z));
    return distance;
}

void DrawBone(vec3_t from, vec3_t to, vec3_t m_location, ImU32 col)
{
    if (Distance3D(m_location, from) > 118)
        return;

    if (Distance3D(m_location, to) > 118)
        return;

    if (Distance3D(from, to) > 39)
        return;

    vec2_t W2S_from;
    if (!sdk::w2s(from, W2S_from))
        return;

    vec2_t W2S_to;
    if (!sdk::w2s(to, W2S_to))
        return;

    auto draw = ImGui::GetForegroundDrawList();
    draw->AddLine(ImVec2(W2S_from.x, W2S_from.y), ImVec2(W2S_to.x, W2S_to.y), col, 1.0f);

}

namespace decryption {
    uintptr_t get_client_info()
    {
        if (vars::is_steam)
        {
            return STEAMdecrypt_client_info(sdk::module_base, sdk::peb);
        }
        else
        {
			return BNETdecrypt_client_info(sdk::module_base, sdk::peb);
        }
    }

    uintptr_t get_client_info_base() {
        if (vars::is_steam)
        {
            return STEAMdecrypt_client_base(sdk::client_info, sdk::module_base, sdk::peb);
        }
        else
        {
            return BNETdecrypt_client_base(sdk::client_info, sdk::module_base, sdk::peb);
        }
    }

    uint64_t get_bone_decrypted_base() {
        if (vars::is_steam)
        {
            return STEAMdecrypt_bone_base(sdk::client_info, sdk::peb);
        }
        else
        {
            return BNETdecrypt_bone_base(sdk::client_info, sdk::peb);
        }
    }    
    
    uint64_t platform_get_bone_index(int index) {
        if (vars::is_steam)
        {
            return decryption::STEAMget_bone_index(index, sdk::module_base);
        }
        else
        {
            return decryption::BNETget_bone_index(index, sdk::module_base);
        }
    }

    struct ref_def_key {
        int ref0;
        int ref1;
        int ref2;
    };

    uintptr_t get_ref_def() {
        ref_def_key crypt = driver::read<ref_def_key>(sdk::module_base + offsets::refdef);
        uint64_t baseAddr = sdk::module_base;

        DWORD lower = crypt.ref0 ^ (crypt.ref2 ^ (uint64_t)(baseAddr + offsets::refdef)) * ((crypt.ref2 ^ (uint64_t)(baseAddr + offsets::refdef)) + 2);
        DWORD upper = crypt.ref1 ^ (crypt.ref2 ^ (uint64_t)(baseAddr + offsets::refdef + 0x4)) * ((crypt.ref2 ^ (uint64_t)(baseAddr + offsets::refdef + 0x4)) + 2);

        return (uint64_t)upper << 32 | lower;
    }

    vec3_t get_bone_position(const uintptr_t bone_ptr, const vec3_t& base_pos, const int bone)
    {
        vec3_t pos = driver::read<vec3_t>(bone_ptr + ((uint64_t)bone * 0x20) + 0x10);
        pos.x += base_pos.x;
        pos.y += base_pos.y;
        pos.z += base_pos.z;
        return pos;
    }

    vec3_t get_bone_base_pos(const uintptr_t client_info)
    {
        return driver::read<vec3_t>(client_info + offsets::bone::base_pos);
    }

    // UPDATED
    extern "C" auto BNETdecrypt_client_info(uint64_t baseModuleAddr, uint64_t Peb) -> uint64_t
    {
        const uint64_t mb = baseModuleAddr;
        uint64_t rax = mb, rbx = mb, rcx = mb, rdx = mb, rdi = mb, rsi = mb, r8 = mb, r9 = mb, r10 = mb, r11 = mb, r12 = mb, r13 = mb, r14 = mb, r15 = mb;
        rbx = read<uintptr_t>(baseModuleAddr + 0x130AD1E8);
        if (!rbx)
            return rbx;
        rdx = ~Peb;              //mov rdx, gs:[rax]
        rax = rbx;              //mov rax, rbx
        rax >>= 0x22;           //shr rax, 0x22
        rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
        rbx ^= rax;             //xor rbx, rax
        rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
        rcx ^= read<uintptr_t>(baseModuleAddr + 0xA1840E3);             //xor rcx, [0x00000000081983B0]
        rax = baseModuleAddr + 0x1343C359;              //lea rax, [0x000000001145061F]
        rbx += rdx;             //add rbx, rdx
        rcx = ~rcx;             //not rcx
        rbx += rax;             //add rbx, rax
        rax = 0xD63E4A83CB9A620B;               //mov rax, 0xD63E4A83CB9A620B
        rbx *= read<uintptr_t>(rcx + 0x11);             //imul rbx, [rcx+0x11]
        rbx -= rdx;             //sub rbx, rdx
        rbx *= rax;             //imul rbx, rax
        rax = 0x57242547CAD98C71;               //mov rax, 0x57242547CAD98C71
        rbx -= rax;             //sub rbx, rax
        return rbx;
    }

    // UPDATED
    extern "C" auto BNETdecrypt_client_base(uint64_t client_info, uint64_t baseModuleAddr, uint64_t Peb) -> uint64_t
    {
        const uint64_t mb = baseModuleAddr;
        uint64_t rax = mb, rbx = mb, rcx = mb, rdx = mb, rdi = mb, rsi = mb, r8 = mb, r9 = mb, r10 = mb, r11 = mb, r12 = mb, r13 = mb, r14 = mb, r15 = mb;
        rdx = read<uintptr_t>(client_info + 0x10e5a0);
        if (!rdx)
            return rdx;
        r11 = ~Peb;              //mov r11, gs:[rax]
        rax = r11;              //mov rax, r11
        rax = _rotl64(rax, 0x23);               //rol rax, 0x23
        rax &= 0xF;
        switch (rax) {
        case 0:
        {
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE036AED]
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081BABAC]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x7;            //shr rax, 0x07
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xE;            //shr rax, 0x0E
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1C;           //shr rax, 0x1C
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x38;           //shr rax, 0x38
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xA;            //shr rax, 0x0A
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x14;           //shr rax, 0x14
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x28;           //shr rax, 0x28
            rdx ^= rax;             //xor rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rdx += rbx;             //add rdx, rbx
            rax = 0x6A51BC9BC4AA6767;               //mov rax, 0x6A51BC9BC4AA6767
            rdx *= rax;             //imul rdx, rax
            rax = 0x5447EBF1221B83E6;               //mov rax, 0x5447EBF1221B83E6
            rdx -= rax;             //sub rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rdx ^= rbx;             //xor rdx, rbx
            return rdx;
        }
        case 1:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081BA775]
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE036645]
            rdx -= r11;             //sub rdx, r11
            rax = baseModuleAddr + 0x6B3C0100;              //lea rax, [0x00000000693F6656]
            rax = ~rax;             //not rax
            rdx ^= rax;             //xor rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rdx ^= rbx;             //xor rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x24;           //shr rax, 0x24
            rdx ^= rax;             //xor rdx, rax
            rax = 0x4A2A83616AD92661;               //mov rax, 0x4A2A83616AD92661
            rdx *= rax;             //imul rdx, rax
            rax = 0xECFC5B4C57C54F28;               //mov rax, 0xECFC5B4C57C54F28
            rdx += rax;             //add rdx, rax
            rax = 0xF0FDCE631F7BA29F;               //mov rax, 0xF0FDCE631F7BA29F
            rdx ^= rax;             //xor rdx, rax
            return rdx;
        }
        case 2:
        {
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE0361CB]
            rcx = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov rcx, [0x00000000081BA275]
            rax = 0xD511FD9CF85D2C07;               //mov rax, 0xD511FD9CF85D2C07
            rdx *= rax;             //imul rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rax = r11;              //mov rax, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFCF;
            RSP_0xFFFFFFFFFFFFFFCF = baseModuleAddr + 0x2E433015;           //lea rax, [0x000000002C469199] : RBP+0xFFFFFFFFFFFFFFCF
            rax *= RSP_0xFFFFFFFFFFFFFFCF;          //imul rax, [rbp-0x31]
            rdx += rax;             //add rdx, rax
            rdx -= rbx;             //sub rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x12;           //shr rax, 0x12
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x24;           //shr rax, 0x24
            rdx ^= rax;             //xor rdx, rax
            rax = 0x3EDAD65FDC1034FF;               //mov rax, 0x3EDAD65FDC1034FF
            rdx *= rax;             //imul rdx, rax
            rax = 0x2AE3002A8E8BF08B;               //mov rax, 0x2AE3002A8E8BF08B
            rdx -= rax;             //sub rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= rcx;             //xor rax, rcx
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            return rdx;
        }
        case 3:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081B9ED9]
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE035DA9]
            rax = rbx + 0x1771cb1b;                 //lea rax, [rbx+0x1771CB1B]
            rax += r11;             //add rax, r11
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xB;            //shr rax, 0x0B
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x16;           //shr rax, 0x16
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x2C;           //shr rax, 0x2C
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = 0xFD500870540625B;                //mov rax, 0xFD500870540625B
            rdx *= rax;             //imul rdx, rax
            rax = 0x1BC06434489E44B5;               //mov rax, 0x1BC06434489E44B5
            rdx += rax;             //add rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1F;           //shr rax, 0x1F
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x3E;           //shr rax, 0x3E
            rdx ^= rax;             //xor rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rax = 0x600D6B3C699E6524;               //mov rax, 0x600D6B3C699E6524
            rdx += rax;             //add rdx, rax
            return rdx;
        }
        case 4:
        {
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE0357D8]
            r9 = read<uintptr_t>(baseModuleAddr + 0xA184129);               //mov r9, [0x00000000081B9890]
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rax = 0x44DF33AE79D34CE7;               //mov rax, 0x44DF33AE79D34CE7
            rdx *= rax;             //imul rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = r11;              //mov rax, r11
            rax = ~rax;             //not rax
            rdx ^= rax;             //xor rdx, rax
            rax = baseModuleAddr + 0x3B36;          //lea rax, [0xFFFFFFFFFE038E0F]
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x27;           //shr rax, 0x27
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = r11;              //mov rax, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFA7;
            RSP_0xFFFFFFFFFFFFFFA7 = baseModuleAddr + 0x27B9;               //lea rax, [0xFFFFFFFFFE037F4F] : RBP+0xFFFFFFFFFFFFFFA7
            rax *= RSP_0xFFFFFFFFFFFFFFA7;          //imul rax, [rbp-0x59]
            rdx += rax;             //add rdx, rax
            rax = r11;              //mov rax, r11
            rax -= rbx;             //sub rax, rbx
            rax += 0xFFFFFFFFD6AD7A46;              //add rax, 0xFFFFFFFFD6AD7A46
            rdx += rax;             //add rdx, rax
            return rdx;
        }
        case 5:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081B93D9]
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE0352A9]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x17;           //shr rax, 0x17
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x2E;           //shr rax, 0x2E
            rdx ^= rax;             //xor rdx, rax
            rax = rbx + 0xb7ef;             //lea rax, [rbx+0xB7EF]
            rax += r11;             //add rax, r11
            rdx += rax;             //add rdx, rax
            rdx -= r11;             //sub rdx, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFBF;
            RSP_0xFFFFFFFFFFFFFFBF = 0x20E3F69C982B8265;            //mov rax, 0x20E3F69C982B8265 : RBP+0xFFFFFFFFFFFFFFBF
            rdx ^= RSP_0xFFFFFFFFFFFFFFBF;          //xor rdx, [rbp-0x41]
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rax = 0xAEE1A029315E3D4F;               //mov rax, 0xAEE1A029315E3D4F
            rdx *= rax;             //imul rdx, rax
            rax = rbx + 0x618b;             //lea rax, [rbx+0x618B]
            rax += r11;             //add rax, r11
            rdx += rax;             //add rdx, rax
            rax = 0x4F576A9DC4CD39EE;               //mov rax, 0x4F576A9DC4CD39EE
            rdx += rax;             //add rdx, rax
            return rdx;
        }
        case 6:
        {
            r9 = read<uintptr_t>(baseModuleAddr + 0xA184129);               //mov r9, [0x00000000081B8F58]
            rdx -= r11;             //sub rdx, r11
            rax = rdx;              //mov rax, rdx
            rax >>= 0x12;           //shr rax, 0x12
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x24;           //shr rax, 0x24
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x21;           //shr rax, 0x21
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1A;           //shr rax, 0x1A
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x34;           //shr rax, 0x34
            rax ^= rdx;             //xor rax, rdx
            rdx = 0x17CF0497F2D22203;               //mov rdx, 0x17CF0497F2D22203
            rax *= rdx;             //imul rax, rdx
            rdx = rax;              //mov rdx, rax
            rdx >>= 0x25;           //shr rdx, 0x25
            rdx ^= rax;             //xor rdx, rax
            rax = 0xBE5CC72B0AEE64FD;               //mov rax, 0xBE5CC72B0AEE64FD
            rdx ^= rax;             //xor rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            return rdx;
        }
        case 7:
        {
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE034952]
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081B89F4]
            rcx = r11;              //mov rcx, r11
            rax = baseModuleAddr + 0xDEF0;          //lea rax, [0xFFFFFFFFFE042553]
            rcx ^= rax;             //xor rcx, rax
            rax = 0xF875422C3B24C08F;               //mov rax, 0xF875422C3B24C08F
            rax -= rcx;             //sub rax, rcx
            rdx += rax;             //add rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x16;           //shr rax, 0x16
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x2C;           //shr rax, 0x2C
            rdx ^= rax;             //xor rdx, rax
            rdx ^= rbx;             //xor rdx, rbx
            rax = 0x65FDE940447DEE2B;               //mov rax, 0x65FDE940447DEE2B
            rdx *= rax;             //imul rdx, rax
            rax = 0x39A26EAD2B76265B;               //mov rax, 0x39A26EAD2B76265B
            rdx += rax;             //add rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1B;           //shr rax, 0x1B
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x36;           //shr rax, 0x36
            rdx ^= rax;             //xor rdx, rax
            return rdx;
        }
        case 8:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081B8551]
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE034421]
            rax = 0xFFFFFFFFFFFF597D;               //mov rax, 0xFFFFFFFFFFFF597D
            rax -= r11;             //sub rax, r11
            rax -= rbx;             //sub rax, rbx
            rdx += rax;             //add rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rax = 0xD0F09E7A8C7613B3;               //mov rax, 0xD0F09E7A8C7613B3
            rdx *= rax;             //imul rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1F;           //shr rax, 0x1F
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x3E;           //shr rax, 0x3E
            rdx ^= rax;             //xor rdx, rax
            rax = 0x256B3436B62B89E5;               //mov rax, 0x256B3436B62B89E5
            rdx -= rax;             //sub rdx, rax
            rdx += rbx;             //add rdx, rbx
            rcx = r11;              //mov rcx, r11
            rax = baseModuleAddr + 0x7B64B958;              //lea rax, [0x000000007967FAA4]
            rax = ~rax;             //not rax
            rcx = ~rcx;             //not rcx
            rcx += rax;             //add rcx, rax
            rdx ^= rcx;             //xor rdx, rcx
            return rdx;
        }
        case 9:
        {
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE033FC5]
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081B8072]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x4;            //shr rax, 0x04
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x8;            //shr rax, 0x08
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x10;           //shr rax, 0x10
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x20;           //shr rax, 0x20
            rdx ^= rax;             //xor rdx, rax
            rax = 0x54E648D07B6D0B80;               //mov rax, 0x54E648D07B6D0B80
            rdx += rax;             //add rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rdx ^= rbx;             //xor rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x16;           //shr rax, 0x16
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x2C;           //shr rax, 0x2C
            rax ^= r11;             //xor rax, r11
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = 0xB0386AF6C89E01ED;               //mov rax, 0xB0386AF6C89E01ED
            rdx *= rax;             //imul rdx, rax
            return rdx;
        }
        case 10:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081B7B9F]
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE033A6F]
            rax = 0x9332D19135BB918F;               //mov rax, 0x9332D19135BB918F
            rdx *= rax;             //imul rdx, rax
            rax = 0xFFFFFFFF8DA4B362;               //mov rax, 0xFFFFFFFF8DA4B362
            rax -= r11;             //sub rax, r11
            rax -= rbx;             //sub rax, rbx
            rdx += rax;             //add rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rdx ^= r11;             //xor rdx, r11
            rax = rdx;              //mov rax, rdx
            rax >>= 0x8;            //shr rax, 0x08
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x10;           //shr rax, 0x10
            rdx ^= rax;             //xor rdx, rax
            rcx = r11;              //mov rcx, r11
            rcx = ~rcx;             //not rcx
            rax = baseModuleAddr + 0xD1ED;          //lea rax, [0xFFFFFFFFFE040AB6]
            rcx *= rax;             //imul rcx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x20;           //shr rax, 0x20
            rcx ^= rax;             //xor rcx, rax
            rdx ^= rcx;             //xor rdx, rcx
            rcx = r11;              //mov rcx, r11
            rcx = ~rcx;             //not rcx
            rax = baseModuleAddr + 0x868;           //lea rax, [0xFFFFFFFFFE0341A1]
            rdx += rax;             //add rdx, rax
            rdx += rcx;             //add rdx, rcx
            rax = r11;              //mov rax, r11
            rax = ~rax;             //not rax
            rax -= rbx;             //sub rax, rbx
            rax -= 0x7CCC6306;              //sub rax, 0x7CCC6306
            rdx ^= rax;             //xor rdx, rax
            return rdx;
        }
        case 11:
        {
            r9 = read<uintptr_t>(baseModuleAddr + 0xA184129);               //mov r9, [0x00000000081B764F]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x12;           //shr rax, 0x12
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x24;           //shr rax, 0x24
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xA;            //shr rax, 0x0A
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x14;           //shr rax, 0x14
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x28;           //shr rax, 0x28
            rdx ^= rax;             //xor rdx, rax
            rax = 0x8CABBD467C0219D3;               //mov rax, 0x8CABBD467C0219D3
            rdx *= rax;             //imul rdx, rax
            rax = 0xAB98E88DE9C18818;               //mov rax, 0xAB98E88DE9C18818
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = r11;              //mov rax, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFEF;
            RSP_0xFFFFFFFFFFFFFFEF = baseModuleAddr + 0x8516;               //lea rax, [0xFFFFFFFFFE03BAD0] : RBP+0xFFFFFFFFFFFFFFEF
            rax *= RSP_0xFFFFFFFFFFFFFFEF;          //imul rax, [rbp-0x11]
            rdx += rax;             //add rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rax = 0x596E42B1953FE5C1;               //mov rax, 0x596E42B1953FE5C1
            rdx += rax;             //add rdx, rax
            return rdx;
        }
        case 12:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081B710E]
            rdx ^= r11;             //xor rdx, r11
            rax = baseModuleAddr + 0x1BAF;          //lea rax, [0xFFFFFFFFFE034819]
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFF9F;
            RSP_0xFFFFFFFFFFFFFF9F = baseModuleAddr + 0x259F56F6;           //lea rax, [0x0000000023A286D4] : RBP+0xFFFFFFFFFFFFFF9F
            rdx += RSP_0xFFFFFFFFFFFFFF9F;          //add rdx, [rbp-0x61]
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rax = 0x294D76F27D0F6D85;               //mov rax, 0x294D76F27D0F6D85
            rdx -= rax;             //sub rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x8;            //shr rax, 0x08
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x10;           //shr rax, 0x10
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x20;           //shr rax, 0x20
            rdx ^= rax;             //xor rdx, rax
            rax = 0x40AE2552A77DAFE6;               //mov rax, 0x40AE2552A77DAFE6
            rdx -= r11;             //sub rdx, r11
            rdx ^= rax;             //xor rdx, rax
            rax = 0x6425FC1CEAFDBD3B;               //mov rax, 0x6425FC1CEAFDBD3B
            rdx *= rax;             //imul rdx, rax
            return rdx;
        }
        case 13:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081B6D16]
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE032BE6]
            rdx += rbx;             //add rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x10;           //shr rax, 0x10
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x20;           //shr rax, 0x20
            rdx ^= rax;             //xor rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rax = r11;              //mov rax, r11
            rax -= rbx;             //sub rax, rbx
            rdx += rax;             //add rdx, rax
            rax = 0x7AAF0F372FD53CD5;               //mov rax, 0x7AAF0F372FD53CD5
            rdx *= rax;             //imul rdx, rax
            rax = 0x4BE188FD7D45B824;               //mov rax, 0x4BE188FD7D45B824
            rdx -= rax;             //sub rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rax = rdx;              //mov rax, rdx
            rax >>= 0x25;           //shr rax, 0x25
            rdx ^= rax;             //xor rdx, rax
            rax = 0x2F94247E3E6CDFF6;               //mov rax, 0x2F94247E3E6CDFF6
            rdx -= rax;             //sub rdx, rax
            return rdx;
        }
        case 14:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184129);              //mov r10, [0x00000000081B6889]
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE03274E]
            rax = 0x59FC5D34C1D95075;               //mov rax, 0x59FC5D34C1D95075
            rdx += rax;             //add rdx, rax
            rax = 0x113F93E895C764EB;               //mov rax, 0x113F93E895C764EB
            rdx *= rax;             //imul rdx, rax
            rax = baseModuleAddr + 0x3BFCF952;              //lea rax, [0x000000003A001DA5]
            rax = ~rax;             //not rax
            rax ^= r11;             //xor rax, r11
            rdx -= rax;             //sub rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rdx -= rbx;             //sub rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x23;           //shr rax, 0x23
            rdx ^= rax;             //xor rdx, rax
            uintptr_t RSP_0xFFFFFFFFFFFFFF9F;
            RSP_0xFFFFFFFFFFFFFF9F = 0x3D4F5BB3C70BE95B;            //mov rax, 0x3D4F5BB3C70BE95B : RBP+0xFFFFFFFFFFFFFF9F
            rdx *= RSP_0xFFFFFFFFFFFFFF9F;          //imul rdx, [rbp-0x61]
            rax = r11;              //mov rax, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFA7;
            RSP_0xFFFFFFFFFFFFFFA7 = baseModuleAddr + 0x9DA7;               //lea rax, [0xFFFFFFFFFE03C500] : RBP+0xFFFFFFFFFFFFFFA7
            rax ^= RSP_0xFFFFFFFFFFFFFFA7;          //xor rax, [rbp-0x59]
            rdx -= rax;             //sub rdx, rax
            return rdx;
        }
        case 15:
        {
            rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFE0322D0]
            r9 = read<uintptr_t>(baseModuleAddr + 0xA184129);               //mov r9, [0x00000000081B6389]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x13;           //shr rax, 0x13
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x26;           //shr rax, 0x26
            rdx ^= rax;             //xor rdx, rax
            rax = 0x63FD32E967945525;               //mov rax, 0x63FD32E967945525
            rdx -= rax;             //sub rdx, rax
            rdx += r11;             //add rdx, r11
            rax = 0xB85215B9839B7D9;                //mov rax, 0xB85215B9839B7D9
            rdx += rax;             //add rdx, rax
            rdx ^= rbx;             //xor rdx, rbx
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= read<uintptr_t>(rax + 0x15);             //imul rdx, [rax+0x15]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x5;            //shr rax, 0x05
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xA;            //shr rax, 0x0A
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x14;           //shr rax, 0x14
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x28;           //shr rax, 0x28
            rdx ^= rax;             //xor rdx, rax
            rax = 0xE52EBF353AE32CDB;               //mov rax, 0xE52EBF353AE32CDB
            rdx *= rax;             //imul rdx, rax
            return rdx;
        }
        }
    }

    // UPDATED
    extern "C" auto BNETdecrypt_bone_base(uint64_t baseModuleAddr, uint64_t Peb) -> uint64_t
    {
        const uint64_t mb = baseModuleAddr;
        uint64_t rax = mb, rbx = mb, rcx = mb, rdx = mb, rdi = mb, rsi = mb, r8 = mb, r9 = mb, r10 = mb, r11 = mb, r12 = mb, r13 = mb, r14 = mb, r15 = mb;
        rax = read<uintptr_t>(baseModuleAddr + 0xDA89AC8);
        if (!rax)
            return rax;
        rbx = Peb;              //mov rbx, gs:[rcx]
        rcx = rbx;              //mov rcx, rbx
        rcx >>= 0x1C;           //shr rcx, 0x1C
        rcx &= 0xF;
        switch (rcx) {
        case 0:
        {
            r9 = read<uintptr_t>(baseModuleAddr + 0xA184221);               //mov r9, [0x0000000007F81643]
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r9;              //xor rcx, r9
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rax += rbx;             //add rax, rbx
            rcx = rax;              //mov rcx, rax
            rax >>= 0x13;           //shr rax, 0x13
            rcx ^= rax;             //xor rcx, rax
            rax = rcx;              //mov rax, rcx
            rax >>= 0x26;           //shr rax, 0x26
            rax ^= rcx;             //xor rax, rcx
            rax -= rbx;             //sub rax, rbx
            rcx = 0xD5A6F9222EC0CD8B;               //mov rcx, 0xD5A6F9222EC0CD8B
            rax *= rcx;             //imul rax, rcx
            rcx = 0xBB2862E8C0DD851B;               //mov rcx, 0xBB2862E8C0DD851B
            rax *= rcx;             //imul rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x4;            //shr rcx, 0x04
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x8;            //shr rcx, 0x08
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x10;           //shr rcx, 0x10
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x20;           //shr rcx, 0x20
            rax ^= rcx;             //xor rax, rcx
            return rax;
        }
        case 1:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F811F4]
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r10;             //xor rcx, r10
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x1E;           //shr rcx, 0x1E
            rax ^= rcx;             //xor rax, rcx
            rdx = baseModuleAddr + 0x6179D5AB;              //lea rdx, [0x000000005F59A35C]
            rdx -= rbx;             //sub rdx, rbx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x3C;           //shr rcx, 0x3C
            rdx ^= rcx;             //xor rdx, rcx
            rax ^= rdx;             //xor rax, rdx
            rax += rbx;             //add rax, rbx
            rcx = 0xC430FCF5AB246D6;                //mov rcx, 0xC430FCF5AB246D6
            rax += rcx;             //add rax, rcx
            rcx = 0x8A220291A10CAF87;               //mov rcx, 0x8A220291A10CAF87
            rax *= rcx;             //imul rax, rcx
            rcx = 0x7FAA38A95F85A6FD;               //mov rcx, 0x7FAA38A95F85A6FD
            rax ^= rcx;             //xor rax, rcx
            rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFDDFCF55]
            rax -= rcx;             //sub rax, rcx
            return rax;
        }
        case 2:
        {
            r14 = baseModuleAddr + 0x2281;          //lea r14, [0xFFFFFFFFFDDFEE40]
            r13 = baseModuleAddr + 0x66A0AFC0;              //lea r13, [0x0000000064807B70]
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F80D85]
            rdx = r13;              //mov rdx, r13
            rdx = ~rdx;             //not rdx
            rdx *= rbx;             //imul rdx, rbx
            rcx = rbx;              //mov rcx, rbx
            rcx = ~rcx;             //not rcx
            rdx += rcx;             //add rdx, rcx
            rcx = baseModuleAddr + 0xBA28;          //lea rcx, [0xFFFFFFFFFDE082E6]
            rax += rcx;             //add rax, rcx
            rax += rdx;             //add rax, rdx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x13;           //shr rcx, 0x13
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x26;           //shr rcx, 0x26
            rax ^= rcx;             //xor rax, rcx
            rcx = r14;              //mov rcx, r14
            rcx ^= rbx;             //xor rcx, rbx
            rax += rcx;             //add rax, rcx
            rcx = 0x975CC895B7E831F1;               //mov rcx, 0x975CC895B7E831F1
            rax ^= rcx;             //xor rax, rcx
            rcx = 0x3B97C5DC626E056F;               //mov rcx, 0x3B97C5DC626E056F
            rax *= rcx;             //imul rax, rcx
            rcx = 0x5534067E232C6632;               //mov rcx, 0x5534067E232C6632
            rax += rcx;             //add rax, rcx
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r10;             //xor rcx, r10
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            return rax;
        }
        case 3:
        {
            r13 = baseModuleAddr + 0x50F8B6F5;              //lea r13, [0x000000004ED87D3D]
            r9 = read<uintptr_t>(baseModuleAddr + 0xA184221);               //mov r9, [0x0000000007F8079F]
            rcx = r13;              //mov rcx, r13
            rcx = ~rcx;             //not rcx
            rcx ^= rbx;             //xor rcx, rbx
            rax += rcx;             //add rax, rcx
            rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFDDFC195]
            rax += rcx;             //add rax, rcx
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r9;              //xor rcx, r9
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rcx = 0x60F6A79B0C8456B1;               //mov rcx, 0x60F6A79B0C8456B1
            rax += rcx;             //add rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x1B;           //shr rcx, 0x1B
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x36;           //shr rcx, 0x36
            rax ^= rcx;             //xor rax, rcx
            rcx = 0x648FCA6FE7D44377;               //mov rcx, 0x648FCA6FE7D44377
            rax -= rcx;             //sub rax, rcx
            rcx = 0xC462FCF18E2C2995;               //mov rcx, 0xC462FCF18E2C2995
            rax *= rcx;             //imul rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x16;           //shr rcx, 0x16
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x2C;           //shr rcx, 0x2C
            rax ^= rcx;             //xor rax, rcx
            return rax;
        }
        case 4:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F80224]
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r10;             //xor rcx, r10
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rax -= rbx;             //sub rax, rbx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x18;           //shr rcx, 0x18
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x30;           //shr rcx, 0x30
            rax ^= rcx;             //xor rax, rcx
            rcx = 0xB1A93FB4C084CAB9;               //mov rcx, 0xB1A93FB4C084CAB9
            rax *= rcx;             //imul rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x24;           //shr rcx, 0x24
            rcx ^= rax;             //xor rcx, rax
            rax = 0x352F8A796F79706B;               //mov rax, 0x352F8A796F79706B
            rcx ^= rax;             //xor rcx, rax
            rax = baseModuleAddr;           //lea rax, [0xFFFFFFFFFDDFBC59]
            rcx -= rax;             //sub rcx, rax
            rax = rbx + 0xffffffffa5917e54;                 //lea rax, [rbx-0x5A6E81AC]
            rax += rcx;             //add rax, rcx
            rcx = 0xA920BAB7A21DDE47;               //mov rcx, 0xA920BAB7A21DDE47
            rax *= rcx;             //imul rax, rcx
            return rax;
        }
        case 5:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F7FDA4]
            rdx = baseModuleAddr + 0x661;           //lea rdx, [0xFFFFFFFFFDDFC174]
            uintptr_t RSP_0x78;
            RSP_0x78 = 0x19A86082B9386E61;          //mov rcx, 0x19A86082B9386E61 : RSP+0x78
            rax ^= RSP_0x78;                //xor rax, [rsp+0x78]
            rcx = rbx;              //mov rcx, rbx
            rcx = ~rcx;             //not rcx
            uintptr_t RSP_0x30;
            RSP_0x30 = baseModuleAddr + 0x89B8;             //lea rcx, [0xFFFFFFFFFDE04502] : RSP+0x30
            rcx ^= RSP_0x30;                //xor rcx, [rsp+0x30]
            rax -= rcx;             //sub rax, rcx
            rcx = 0x6CF5D40C805C3929;               //mov rcx, 0x6CF5D40C805C3929
            rax *= rcx;             //imul rax, rcx
            rax -= rbx;             //sub rax, rbx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x23;           //shr rcx, 0x23
            rax ^= rcx;             //xor rax, rcx
            rcx = 0xEA2BDCA216FA84E;                //mov rcx, 0xEA2BDCA216FA84E
            rax += rcx;             //add rax, rcx
            rcx = rdx;              //mov rcx, rdx
            rcx = ~rcx;             //not rcx
            rcx -= rbx;             //sub rcx, rbx
            rax += rcx;             //add rax, rcx
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r10;             //xor rcx, r10
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            return rax;
        }
        case 6:
        {
            r9 = read<uintptr_t>(baseModuleAddr + 0xA184221);               //mov r9, [0x0000000007F7F885]
            rcx = 0x143119596E0AB6F4;               //mov rcx, 0x143119596E0AB6F4
            rcx -= rbx;             //sub rcx, rbx
            rax += rcx;             //add rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x14;           //shr rcx, 0x14
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x28;           //shr rcx, 0x28
            rax ^= rcx;             //xor rax, rcx
            rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFDDFB4B1]
            rax -= rcx;             //sub rax, rcx
            rcx = 0xD0FF53657C7A437;                //mov rcx, 0xD0FF53657C7A437
            rax += rcx;             //add rax, rcx
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r9;              //xor rcx, r9
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rcx = 0x1946435536018835;               //mov rcx, 0x1946435536018835
            rax *= rcx;             //imul rax, rcx
            rax -= rbx;             //sub rax, rbx
            return rax;
        }
        case 7:
        {
            r11 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r11, [0x0000000007F7F474]
            rdx = baseModuleAddr + 0x32D6EFEE;              //lea rdx, [0x0000000030B6A1E7]
            r8 = 0;                 //and r8, 0xFFFFFFFFC0000000
            r8 = _rotl64(r8, 0x10);                 //rol r8, 0x10
            r8 ^= r11;              //xor r8, r11
            rcx = rbx;              //mov rcx, rbx
            rcx = ~rcx;             //not rcx
            r8 = ~r8;               //not r8
            rax += rcx;             //add rax, rcx
            rax += rdx;             //add rax, rdx
            rax ^= rbx;             //xor rax, rbx
            rax *= read<uintptr_t>(r8 + 0x13);              //imul rax, [r8+0x13]
            rcx = 0x6B8832A948DD0921;               //mov rcx, 0x6B8832A948DD0921
            rax += rcx;             //add rax, rcx
            rcx = 0x9D382E284DCFD7C7;               //mov rcx, 0x9D382E284DCFD7C7
            rax *= rcx;             //imul rax, rcx
            rcx = 0x4A2F2EC6D9595386;               //mov rcx, 0x4A2F2EC6D9595386
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x7;            //shr rcx, 0x07
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0xE;            //shr rcx, 0x0E
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x1C;           //shr rcx, 0x1C
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x38;           //shr rcx, 0x38
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x15;           //shr rcx, 0x15
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x2A;           //shr rcx, 0x2A
            rax ^= rcx;             //xor rax, rcx
            return rax;
        }
        case 8:
        {
            r14 = baseModuleAddr + 0x98C7;          //lea r14, [0xFFFFFFFFFDE04557]
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F7EE53]
            rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFDDFA8A0]
            rax ^= rcx;             //xor rax, rcx
            rcx = 0xB9B101CE6C8E2F91;               //mov rcx, 0xB9B101CE6C8E2F91
            rax *= rcx;             //imul rax, rcx
            rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFDDFAB97]
            rax -= rcx;             //sub rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0xC;            //shr rcx, 0x0C
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x18;           //shr rcx, 0x18
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x30;           //shr rcx, 0x30
            rax ^= rcx;             //xor rax, rcx
            rax -= rbx;             //sub rax, rbx
            rcx = 0x4F54898D891371A4;               //mov rcx, 0x4F54898D891371A4
            rax += rcx;             //add rax, rcx
            rdx = 0;                //and rdx, 0xFFFFFFFFC0000000
            rcx = r14;              //mov rcx, r14
            rdx = _rotl64(rdx, 0x10);               //rol rdx, 0x10
            rcx ^= rbx;             //xor rcx, rbx
            rax -= rcx;             //sub rax, rcx
            rdx ^= r10;             //xor rdx, r10
            rdx = ~rdx;             //not rdx
            rax *= read<uintptr_t>(rdx + 0x13);             //imul rax, [rdx+0x13]
            return rax;
        }
        case 9:
        {
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F7E9F7]
            rcx = 0x5C495DB1FF8A0C7D;               //mov rcx, 0x5C495DB1FF8A0C7D
            rax *= rcx;             //imul rax, rcx
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r10;             //xor rcx, r10
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFDDFA752]
            rax -= rcx;             //sub rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x13;           //shr rcx, 0x13
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x26;           //shr rcx, 0x26
            rax ^= rcx;             //xor rax, rcx
            rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFDDFA511]
            rax += rcx;             //add rax, rcx
            rdx = baseModuleAddr;           //lea rdx, [0xFFFFFFFFFDDFA4F8]
            rdx += rbx;             //add rdx, rbx
            rcx = 0x931F45DADBA6534A;               //mov rcx, 0x931F45DADBA6534A
            rax += rcx;             //add rax, rcx
            rax += rdx;             //add rax, rdx
            rcx = 0x7254D9C4F5E0407F;               //mov rcx, 0x7254D9C4F5E0407F
            rax *= rcx;             //imul rax, rcx
            return rax;
        }
        case 10:
        {
            r9 = read<uintptr_t>(baseModuleAddr + 0xA184221);               //mov r9, [0x0000000007F7E578]
            rcx = 0x16092956D42CB466;               //mov rcx, 0x16092956D42CB466
            rax += rcx;             //add rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x28;           //shr rcx, 0x28
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x22;           //shr rcx, 0x22
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0xB;            //shr rcx, 0x0B
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x16;           //shr rcx, 0x16
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x2C;           //shr rcx, 0x2C
            rax ^= rcx;             //xor rax, rcx
            rcx = 0xC2E2E61ED49F5991;               //mov rcx, 0xC2E2E61ED49F5991
            rax *= rcx;             //imul rax, rcx
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r9;              //xor rcx, r9
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x4;            //shr rcx, 0x04
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x8;            //shr rcx, 0x08
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x10;           //shr rcx, 0x10
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x20;           //shr rcx, 0x20
            rax ^= rcx;             //xor rax, rcx
            rax -= rbx;             //sub rax, rbx
            return rax;
        }
        case 11:
        {
            r13 = baseModuleAddr + 0x3E6FD0B3;              //lea r13, [0x000000003C4F6EF1]
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F7DFAC]
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x1B;           //shr rcx, 0x1B
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x36;           //shr rcx, 0x36
            rax ^= rcx;             //xor rax, rcx
            r14 = baseModuleAddr;           //lea r14, [0xFFFFFFFFFDDF9881]
            rax += r14;             //add rax, r14
            r14 = baseModuleAddr + 0x27799030;              //lea r14, [0x000000002559289A]
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r10;             //xor rcx, r10
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rcx = 0x6E5FECB626B1C472;               //mov rcx, 0x6E5FECB626B1C472
            rax += rcx;             //add rax, rcx
            rdx = r13;              //mov rdx, r13
            rdx = ~rdx;             //not rdx
            rdx ^= rbx;             //xor rdx, rbx
            rcx = 0xF5121CBF37E46BBB;               //mov rcx, 0xF5121CBF37E46BBB
            rax += rcx;             //add rax, rcx
            rax += rdx;             //add rax, rdx
            rcx = r14;              //mov rcx, r14
            rcx ^= rbx;             //xor rcx, rbx
            rax -= rcx;             //sub rax, rcx
            rcx = 0xD2D6E8735A76DE2D;               //mov rcx, 0xD2D6E8735A76DE2D
            rax *= rcx;             //imul rax, rcx
            return rax;
        }
        case 12:
        {
            rdx = baseModuleAddr + 0xF737;          //lea rdx, [0xFFFFFFFFFDE08F62]
            r13 = baseModuleAddr + 0x1124573F;              //lea r13, [0x000000000F03EF4C]
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F7D98E]
            rax += rbx;             //add rax, rbx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x10;           //shr rcx, 0x10
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x20;           //shr rcx, 0x20
            rax ^= rcx;             //xor rax, rcx
            uintptr_t RSP_0x50;
            RSP_0x50 = 0x636BE495B0FA383E;          //mov rcx, 0x636BE495B0FA383E : RSP+0x50
            rax ^= RSP_0x50;                //xor rax, [rsp+0x50]
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r10;             //xor rcx, r10
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rcx = r13;              //mov rcx, r13
            rcx ^= rbx;             //xor rcx, rbx
            rax ^= rcx;             //xor rax, rcx
            rcx = 0x8812EF99851F0715;               //mov rcx, 0x8812EF99851F0715
            rax *= rcx;             //imul rax, rcx
            rcx = rbx;              //mov rcx, rbx
            rcx = ~rcx;             //not rcx
            rcx += rdx;             //add rcx, rdx
            rax ^= rcx;             //xor rax, rcx
            rcx = baseModuleAddr + 0xB69;           //lea rcx, [0xFFFFFFFFFDDFA1C3]
            rcx -= rbx;             //sub rcx, rbx
            rax += rcx;             //add rax, rcx
            return rax;
        }
        case 13:
        {
            r13 = baseModuleAddr + 0x5EF7;          //lea r13, [0xFFFFFFFFFDDFF21B]
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F7D4EB]
            rcx = rax;              //mov rcx, rax
            rcx >>= 0xA;            //shr rcx, 0x0A
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x14;           //shr rcx, 0x14
            rax ^= rcx;             //xor rax, rcx
            rdx = rbx;              //mov rdx, rbx
            rcx = rax;              //mov rcx, rax
            rdx = ~rdx;             //not rdx
            rcx >>= 0x28;           //shr rcx, 0x28
            rdx ^= r13;             //xor rdx, r13
            rax ^= rcx;             //xor rax, rcx
            rax += rdx;             //add rax, rdx
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r10;             //xor rcx, r10
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            rcx = 0x736E085CD239F4CB;               //mov rcx, 0x736E085CD239F4CB
            rax *= rcx;             //imul rax, rcx
            rax -= rbx;             //sub rax, rbx
            rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFDDF8E32]
            rax ^= rcx;             //xor rax, rcx
            rcx = 0x7DA51A97E3053243;               //mov rcx, 0x7DA51A97E3053243
            rax *= rcx;             //imul rax, rcx
            rcx = 0x785CF31817D043AC;               //mov rcx, 0x785CF31817D043AC
            rax ^= rcx;             //xor rax, rcx
            return rax;
        }
        case 14:
        {
            r13 = baseModuleAddr + 0x16A6;          //lea r13, [0xFFFFFFFFFDDFA3F5]
            r14 = baseModuleAddr + 0xF57E;          //lea r14, [0xFFFFFFFFFDE082BE]
            r9 = read<uintptr_t>(baseModuleAddr + 0xA184221);               //mov r9, [0x0000000007F7CF10]
            rcx = rbx;              //mov rcx, rbx
            rcx = ~rcx;             //not rcx
            rcx *= r14;             //imul rcx, r14
            rax += rcx;             //add rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x1A;           //shr rcx, 0x1A
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x34;           //shr rcx, 0x34
            rax ^= rcx;             //xor rax, rcx
            r11 = 0xBE40C084BA769FA;                //mov r11, 0xBE40C084BA769FA
            rcx = r13;              //mov rcx, r13
            rcx *= rbx;             //imul rcx, rbx
            rcx += r11;             //add rcx, r11
            rax += rcx;             //add rax, rcx
            rcx = 0xB92AAB45027C43E2;               //mov rcx, 0xB92AAB45027C43E2
            rax ^= rcx;             //xor rax, rcx
            rcx = baseModuleAddr + 0x4FED906B;              //lea rcx, [0x000000004DCD1AB1]
            rcx += rbx;             //add rcx, rbx
            rax += rcx;             //add rax, rcx
            rcx = 0xBCCB1C832C79BF0B;               //mov rcx, 0xBCCB1C832C79BF0B
            rax *= rcx;             //imul rax, rcx
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
            rcx ^= r9;              //xor rcx, r9
            rcx = ~rcx;             //not rcx
            rax *= read<uintptr_t>(rcx + 0x13);             //imul rax, [rcx+0x13]
            return rax;
        }
        case 15:
        {
            r14 = baseModuleAddr + 0x31081C40;              //lea r14, [0x000000002EE7A45C]
            r10 = read<uintptr_t>(baseModuleAddr + 0xA184221);              //mov r10, [0x0000000007F7C9DF]
            rdx = 0;                //and rdx, 0xFFFFFFFFC0000000
            rdx = _rotl64(rdx, 0x10);               //rol rdx, 0x10
            rcx = rax;              //mov rcx, rax
            rdx ^= r10;             //xor rdx, r10
            rcx >>= 0x20;           //shr rcx, 0x20
            rdx = ~rdx;             //not rdx
            rax ^= rcx;             //xor rax, rcx
            rcx = r14;              //mov rcx, r14
            rcx ^= rbx;             //xor rcx, rbx
            rax *= read<uintptr_t>(rdx + 0x13);             //imul rax, [rdx+0x13]
            rax -= rcx;             //sub rax, rcx
            rcx = 0xFC32828FC4E7EFD1;               //mov rcx, 0xFC32828FC4E7EFD1
            rax *= rcx;             //imul rax, rcx
            rax -= rbx;             //sub rax, rbx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x3;            //shr rcx, 0x03
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x6;            //shr rcx, 0x06
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0xC;            //shr rcx, 0x0C
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x18;           //shr rcx, 0x18
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x30;           //shr rcx, 0x30
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x8;            //shr rcx, 0x08
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x10;           //shr rcx, 0x10
            rax ^= rcx;             //xor rax, rcx
            rcx = rax;              //mov rcx, rax
            rcx >>= 0x20;           //shr rcx, 0x20
            rax ^= rcx;             //xor rax, rcx
            rcx = 0x8D793715ED015397;               //mov rcx, 0x8D793715ED015397
            rax *= rcx;             //imul rax, rcx
            return rax;
        }
        }
    }

    // UPDATED
    extern "C" auto BNETget_bone_index(uint32_t bone_index, uint64_t baseModuleAddr) -> uint64_t
    {
        const uint64_t mb = baseModuleAddr;
        uint64_t rax = mb, rbx = mb, rcx = mb, rdx = mb, rdi = mb, rsi = mb, r8 = mb, r9 = mb, r10 = mb, r11 = mb, r12 = mb, r13 = mb, r14 = mb, r15 = mb;
        rdi = bone_index;
        rcx = rdi * 0x13C8;
        rax = 0x1B5C5E9652FDACE7;               //mov rax, 0x1B5C5E9652FDACE7
        rax = _umul128(rax, rcx, (uintptr_t*)&rdx);             //mul rcx
        r11 = baseModuleAddr;           //lea r11, [0xFFFFFFFFFD82C0B8]
        r10 = 0x19E9C4E0C9861BBD;               //mov r10, 0x19E9C4E0C9861BBD
        rdx >>= 0xA;            //shr rdx, 0x0A
        rax = rdx * 0x256D;             //imul rax, rdx, 0x256D
        rcx -= rax;             //sub rcx, rax
        rax = 0x4F9FF77A70376427;               //mov rax, 0x4F9FF77A70376427
        r8 = rcx * 0x256D;              //imul r8, rcx, 0x256D
        rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
        rax = r8;               //mov rax, r8
        rax -= rdx;             //sub rax, rdx
        rax >>= 0x1;            //shr rax, 0x01
        rax += rdx;             //add rax, rdx
        rax >>= 0xD;            //shr rax, 0x0D
        rax = rax * 0x30D1;             //imul rax, rax, 0x30D1
        r8 -= rax;              //sub r8, rax
        rax = 0x70381C0E070381C1;               //mov rax, 0x70381C0E070381C1
        rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
        rax = 0x624DD2F1A9FBE77;                //mov rax, 0x624DD2F1A9FBE77
        rdx >>= 0x6;            //shr rdx, 0x06
        rcx = rdx * 0x92;               //imul rcx, rdx, 0x92
        rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
        rax = r8;               //mov rax, r8
        rax -= rdx;             //sub rax, rdx
        rax >>= 0x1;            //shr rax, 0x01
        rax += rdx;             //add rax, rdx
        rax >>= 0x6;            //shr rax, 0x06
        rcx += rax;             //add rcx, rax
        rax = rcx * 0xFA;               //imul rax, rcx, 0xFA
        rcx = r8 * 0xFC;                //imul rcx, r8, 0xFC
        rcx -= rax;             //sub rcx, rax
        rax = read<uint16_t>(rcx + r11 * 1 + 0xA2418C0);                //movzx eax, word ptr [rcx+r11*1+0xA2418C0]
        r8 = rax * 0x13C8;              //imul r8, rax, 0x13C8
        rax = r10;              //mov rax, r10
        rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
        rcx = r8;               //mov rcx, r8
        rax = r10;              //mov rax, r10
        rcx -= rdx;             //sub rcx, rdx
        rcx >>= 0x1;            //shr rcx, 0x01
        rcx += rdx;             //add rcx, rdx
        rcx >>= 0xC;            //shr rcx, 0x0C
        rcx = rcx * 0x1D0F;             //imul rcx, rcx, 0x1D0F
        r8 -= rcx;              //sub r8, rcx
        r9 = r8 * 0x3981;               //imul r9, r8, 0x3981
        rax = _umul128(rax, r9, (uintptr_t*)&rdx);              //mul r9
        rax = r9;               //mov rax, r9
        rax -= rdx;             //sub rax, rdx
        rax >>= 0x1;            //shr rax, 0x01
        rax += rdx;             //add rax, rdx
        rax >>= 0xC;            //shr rax, 0x0C
        rax = rax * 0x1D0F;             //imul rax, rax, 0x1D0F
        r9 -= rax;              //sub r9, rax
        rax = 0xD79435E50D79435F;               //mov rax, 0xD79435E50D79435F
        rax = _umul128(rax, r9, (uintptr_t*)&rdx);              //mul r9
        rax = 0xA6810A6810A6811;                //mov rax, 0xA6810A6810A6811
        rdx >>= 0x6;            //shr rdx, 0x06
        rcx = rdx * 0x4C;               //imul rcx, rdx, 0x4C
        rax = _umul128(rax, r9, (uintptr_t*)&rdx);              //mul r9
        rax = r9;               //mov rax, r9
        rax -= rdx;             //sub rax, rdx
        rax >>= 0x1;            //shr rax, 0x01
        rax += rdx;             //add rax, rdx
        rax >>= 0x6;            //shr rax, 0x06
        rcx += rax;             //add rcx, rax
        rax = rcx * 0xF6;               //imul rax, rcx, 0xF6
        rcx = r9 * 0xF8;                //imul rcx, r9, 0xF8
        rcx -= rax;             //sub rcx, rax
        r15 = read<uint16_t>(rcx + r11 * 1 + 0xA24A760);                //movsx r15d, word ptr [rcx+r11*1+0xA24A760]
        return r15;
    }

    // OUTDATED
    extern "C" auto STEAMdecrypt_client_info(uint64_t baseModuleAddr, uint64_t Peb) -> uint64_t
    {
        uint64_t rax = baseModuleAddr, rbx = baseModuleAddr, rcx = baseModuleAddr, rdx = baseModuleAddr, rdi = baseModuleAddr, rsi = baseModuleAddr, r8 = baseModuleAddr, r9 = baseModuleAddr, r10 = baseModuleAddr, r11 = baseModuleAddr, r12 = baseModuleAddr, r13 = baseModuleAddr, r14 = baseModuleAddr, r15 = baseModuleAddr;
        rbx = driver::read<uintptr_t>(baseModuleAddr + 0x12DB8AB0);
        if (!rbx)
            return rbx;
        rdx = ~Peb;              //mov rdx, gs:[rax]
        rax = rbx;              //mov rax, rbx
        rax >>= 0x22;           //shr rax, 0x22
        rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
        rbx ^= rax;             //xor rbx, rax
        rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
        rcx ^= driver::read<uintptr_t>(baseModuleAddr + 0x9DE30E3);             //xor rcx, [0x00000000013AF086]
        rax = baseModuleAddr + 0x1343C359;              //lea rax, [0x000000000AA082F5]
        rbx += rdx;             //add rbx, rdx
        rcx = ~rcx;             //not rcx
        rbx += rax;             //add rbx, rax
        rax = 0xD63E4A83CB9A620B;               //mov rax, 0xD63E4A83CB9A620B
        rbx *= driver::read<uintptr_t>(rcx + 0x11);             //imul rbx, [rcx+0x11]
        rbx -= rdx;             //sub rbx, rdx
        rbx *= rax;             //imul rbx, rax
        rax = 0x57242547CAD98C71;               //mov rax, 0x57242547CAD98C71
        rbx -= rax;             //sub rbx, rax
        return rbx;
    }

    // OUTDATED
    extern "C" auto STEAMdecrypt_client_base(uint64_t client_info, uint64_t baseModuleAddr, uint64_t Peb) -> uint64_t
    {
        const uint64_t mb = sdk::module_base;
        uint64_t rax = mb, rbx = mb, rcx = mb, rdx = mb, rdi = mb, rsi = mb, r8 = mb, r9 = mb, r10 = mb, r11 = mb, r12 = mb, r13 = mb, r14 = mb, r15 = mb;
        rdx = driver::read<uintptr_t>(client_info + 0x10e240);
        if (!rdx)
            return rdx;
        r11 = ~Peb;                //mov r11, gs:[rax]
        rax = r11;              //mov rax, r11
        rax = _rotl64(rax, 0x23);               //rol rax, 0x23
        rax &= 0xF;
        switch (rax) {
        case 0:
        {
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE06AD3A]
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E4DDF9]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x7;            //shr rax, 0x07
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xE;            //shr rax, 0x0E
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1C;           //shr rax, 0x1C
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x38;           //shr rax, 0x38
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xA;            //shr rax, 0x0A
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x14;           //shr rax, 0x14
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x28;           //shr rax, 0x28
            rdx ^= rax;             //xor rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rdx += rbx;             //add rdx, rbx
            rax = 0x6A51BC9BC4AA6767;               //mov rax, 0x6A51BC9BC4AA6767
            rdx *= rax;             //imul rdx, rax
            rax = 0x5447EBF1221B83E6;               //mov rax, 0x5447EBF1221B83E6
            rdx -= rax;             //sub rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rdx ^= rbx;             //xor rdx, rbx
            return rdx;
        }
        case 1:
        {
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E4D9C5]
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE06A895]
            rdx -= r11;             //sub rdx, r11
            rax = baseModuleAddr + 0x6B3C0100;            //lea rax, [0x000000006942A8A6]
            rax = ~rax;             //not rax
            rdx ^= rax;             //xor rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rdx ^= rbx;             //xor rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x24;           //shr rax, 0x24
            rdx ^= rax;             //xor rdx, rax
            rax = 0x4A2A83616AD92661;               //mov rax, 0x4A2A83616AD92661
            rdx *= rax;             //imul rdx, rax
            rax = 0xECFC5B4C57C54F28;               //mov rax, 0xECFC5B4C57C54F28
            rdx += rax;             //add rdx, rax
            rax = 0xF0FDCE631F7BA29F;               //mov rax, 0xF0FDCE631F7BA29F
            rdx ^= rax;             //xor rdx, rax
            return rdx;
        }
        case 2:
        {
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE06A41B]
            rcx = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov rcx, [0x0000000007E4D4C5]
            rax = 0xD511FD9CF85D2C07;               //mov rax, 0xD511FD9CF85D2C07
            rdx *= rax;             //imul rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rax = r11;              //mov rax, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFCF;
            RSP_0xFFFFFFFFFFFFFFCF = baseModuleAddr + 0x2E433015;                 //lea rax, [0x000000002C49D3E9] : RBP+0xFFFFFFFFFFFFFFCF
            rax *= RSP_0xFFFFFFFFFFFFFFCF;          //imul rax, [rbp-0x31]
            rdx += rax;             //add rdx, rax
            rdx -= rbx;             //sub rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x12;           //shr rax, 0x12
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x24;           //shr rax, 0x24
            rdx ^= rax;             //xor rdx, rax
            rax = 0x3EDAD65FDC1034FF;               //mov rax, 0x3EDAD65FDC1034FF
            rdx *= rax;             //imul rdx, rax
            rax = 0x2AE3002A8E8BF08B;               //mov rax, 0x2AE3002A8E8BF08B
            rdx -= rax;             //sub rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= rcx;             //xor rax, rcx
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            return rdx;
        }
        case 3:
        {
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E4D129]
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE069FF9]
            rax = rbx + 0x1771cb1b;                 //lea rax, [rbx+0x1771CB1B]
            rax += r11;             //add rax, r11
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xB;            //shr rax, 0x0B
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x16;           //shr rax, 0x16
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x2C;           //shr rax, 0x2C
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = 0xFD500870540625B;                //mov rax, 0xFD500870540625B
            rdx *= rax;             //imul rdx, rax
            rax = 0x1BC06434489E44B5;               //mov rax, 0x1BC06434489E44B5
            rdx += rax;             //add rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1F;           //shr rax, 0x1F
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x3E;           //shr rax, 0x3E
            rdx ^= rax;             //xor rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rax = 0x600D6B3C699E6524;               //mov rax, 0x600D6B3C699E6524
            rdx += rax;             //add rdx, rax
            return rdx;
        }
        case 4:
        {
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE069A28]
            r9 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);              //mov r9, [0x0000000007E4CAE0]
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rax = 0x44DF33AE79D34CE7;               //mov rax, 0x44DF33AE79D34CE7
            rdx *= rax;             //imul rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = r11;              //mov rax, r11
            rax = ~rax;             //not rax
            rdx ^= rax;             //xor rdx, rax
            rax = baseModuleAddr + 0x3B36;                //lea rax, [0xFFFFFFFFFE06D05F]
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x27;           //shr rax, 0x27
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = r11;              //mov rax, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFA7;
            RSP_0xFFFFFFFFFFFFFFA7 = baseModuleAddr + 0x27B9;             //lea rax, [0xFFFFFFFFFE06C19F] : RBP+0xFFFFFFFFFFFFFFA7
            rax *= RSP_0xFFFFFFFFFFFFFFA7;          //imul rax, [rbp-0x59]
            rdx += rax;             //add rdx, rax
            rax = r11;              //mov rax, r11
            rax -= rbx;             //sub rax, rbx
            rax += 0xFFFFFFFFD6AD7A46;              //add rax, 0xFFFFFFFFD6AD7A46
            rdx += rax;             //add rdx, rax
            return rdx;
        }
        case 5:
        {
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E4C629]
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE0694F9]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x17;           //shr rax, 0x17
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x2E;           //shr rax, 0x2E
            rdx ^= rax;             //xor rdx, rax
            rax = rbx + 0xb7ef;             //lea rax, [rbx+0xB7EF]
            rax += r11;             //add rax, r11
            rdx += rax;             //add rdx, rax
            rdx -= r11;             //sub rdx, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFBF;
            RSP_0xFFFFFFFFFFFFFFBF = 0x20E3F69C982B8265;            //mov rax, 0x20E3F69C982B8265 : RBP+0xFFFFFFFFFFFFFFBF
            rdx ^= RSP_0xFFFFFFFFFFFFFFBF;          //xor rdx, [rbp-0x41]
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rax = 0xAEE1A029315E3D4F;               //mov rax, 0xAEE1A029315E3D4F
            rdx *= rax;             //imul rdx, rax
            rax = rbx + 0x618b;             //lea rax, [rbx+0x618B]
            rax += r11;             //add rax, r11
            rdx += rax;             //add rdx, rax
            rax = 0x4F576A9DC4CD39EE;               //mov rax, 0x4F576A9DC4CD39EE
            rdx += rax;             //add rdx, rax
            return rdx;
        }
        case 6:
        {
            r9 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);              //mov r9, [0x0000000007E4C1A8]
            rdx -= r11;             //sub rdx, r11
            rax = rdx;              //mov rax, rdx
            rax >>= 0x12;           //shr rax, 0x12
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x24;           //shr rax, 0x24
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x21;           //shr rax, 0x21
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1A;           //shr rax, 0x1A
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x34;           //shr rax, 0x34
            rax ^= rdx;             //xor rax, rdx
            rdx = 0x17CF0497F2D22203;               //mov rdx, 0x17CF0497F2D22203
            rax *= rdx;             //imul rax, rdx
            rdx = rax;              //mov rdx, rax
            rdx >>= 0x25;           //shr rdx, 0x25
            rdx ^= rax;             //xor rdx, rax
            rax = 0xBE5CC72B0AEE64FD;               //mov rax, 0xBE5CC72B0AEE64FD
            rdx ^= rax;             //xor rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            return rdx;
        }
        case 7:
        {
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE068BA2]
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E4BC44]
            rcx = r11;              //mov rcx, r11
            rax = baseModuleAddr + 0xDEF0;                //lea rax, [0xFFFFFFFFFE0767A3]
            rcx ^= rax;             //xor rcx, rax
            rax = 0xF875422C3B24C08F;               //mov rax, 0xF875422C3B24C08F
            rax -= rcx;             //sub rax, rcx
            rdx += rax;             //add rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x16;           //shr rax, 0x16
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x2C;           //shr rax, 0x2C
            rdx ^= rax;             //xor rdx, rax
            rdx ^= rbx;             //xor rdx, rbx
            rax = 0x65FDE940447DEE2B;               //mov rax, 0x65FDE940447DEE2B
            rdx *= rax;             //imul rdx, rax
            rax = 0x39A26EAD2B76265B;               //mov rax, 0x39A26EAD2B76265B
            rdx += rax;             //add rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1B;           //shr rax, 0x1B
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x36;           //shr rax, 0x36
            rdx ^= rax;             //xor rdx, rax
            return rdx;
        }
        case 8:
        {
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E4B7A1]
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE068671]
            rax = 0xFFFFFFFFFFFF597D;               //mov rax, 0xFFFFFFFFFFFF597D
            rax -= r11;             //sub rax, r11
            rax -= rbx;             //sub rax, rbx
            rdx += rax;             //add rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rax = 0xD0F09E7A8C7613B3;               //mov rax, 0xD0F09E7A8C7613B3
            rdx *= rax;             //imul rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x1F;           //shr rax, 0x1F
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x3E;           //shr rax, 0x3E
            rdx ^= rax;             //xor rdx, rax
            rax = 0x256B3436B62B89E5;               //mov rax, 0x256B3436B62B89E5
            rdx -= rax;             //sub rdx, rax
            rdx += rbx;             //add rdx, rbx
            rcx = r11;              //mov rcx, r11
            rax = baseModuleAddr + 0x7B64B958;            //lea rax, [0x00000000796B3CF4]
            rax = ~rax;             //not rax
            rcx = ~rcx;             //not rcx
            rcx += rax;             //add rcx, rax
            rdx ^= rcx;             //xor rdx, rcx
            return rdx;
        }
        case 9:
        {
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE068215]
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E4B2C2]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x4;            //shr rax, 0x04
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x8;            //shr rax, 0x08
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x10;           //shr rax, 0x10
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x20;           //shr rax, 0x20
            rdx ^= rax;             //xor rdx, rax
            rax = 0x54E648D07B6D0B80;               //mov rax, 0x54E648D07B6D0B80
            rdx += rax;             //add rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rdx ^= rbx;             //xor rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x16;           //shr rax, 0x16
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x2C;           //shr rax, 0x2C
            rax ^= r11;             //xor rax, r11
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = 0xB0386AF6C89E01ED;               //mov rax, 0xB0386AF6C89E01ED
            rdx *= rax;             //imul rdx, rax
            return rdx;
        }
        case 10:
        {
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E4ADEF]
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE067CBF]
            rax = 0x9332D19135BB918F;               //mov rax, 0x9332D19135BB918F
            rdx *= rax;             //imul rdx, rax
            rax = 0xFFFFFFFF8DA4B362;               //mov rax, 0xFFFFFFFF8DA4B362
            rax -= r11;             //sub rax, r11
            rax -= rbx;             //sub rax, rbx
            rdx += rax;             //add rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rdx ^= r11;             //xor rdx, r11
            rax = rdx;              //mov rax, rdx
            rax >>= 0x8;            //shr rax, 0x08
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x10;           //shr rax, 0x10
            rdx ^= rax;             //xor rdx, rax
            rcx = r11;              //mov rcx, r11
            rcx = ~rcx;             //not rcx
            rax = baseModuleAddr + 0xD1ED;                //lea rax, [0xFFFFFFFFFE074D06]
            rcx *= rax;             //imul rcx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x20;           //shr rax, 0x20
            rcx ^= rax;             //xor rcx, rax
            rdx ^= rcx;             //xor rdx, rcx
            rcx = r11;              //mov rcx, r11
            rcx = ~rcx;             //not rcx
            rax = baseModuleAddr + 0x868;                 //lea rax, [0xFFFFFFFFFE0683F1]
            rdx += rax;             //add rdx, rax
            rdx += rcx;             //add rdx, rcx
            rax = r11;              //mov rax, r11
            rax = ~rax;             //not rax
            rax -= rbx;             //sub rax, rbx
            rax -= 0x7CCC6306;              //sub rax, 0x7CCC6306
            rdx ^= rax;             //xor rdx, rax
            return rdx;
        }
        case 11:
        {
            r9 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);              //mov r9, [0x0000000007E4A89F]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x12;           //shr rax, 0x12
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x24;           //shr rax, 0x24
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xA;            //shr rax, 0x0A
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x14;           //shr rax, 0x14
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x28;           //shr rax, 0x28
            rdx ^= rax;             //xor rdx, rax
            rax = 0x8CABBD467C0219D3;               //mov rax, 0x8CABBD467C0219D3
            rdx *= rax;             //imul rdx, rax
            rax = 0xAB98E88DE9C18818;               //mov rax, 0xAB98E88DE9C18818
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            rax = r11;              //mov rax, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFEF;
            RSP_0xFFFFFFFFFFFFFFEF = baseModuleAddr + 0x8516;             //lea rax, [0xFFFFFFFFFE06FD20] : RBP+0xFFFFFFFFFFFFFFEF
            rax *= RSP_0xFFFFFFFFFFFFFFEF;          //imul rax, [rbp-0x11]
            rdx += rax;             //add rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rax = 0x596E42B1953FE5C1;               //mov rax, 0x596E42B1953FE5C1
            rdx += rax;             //add rdx, rax
            return rdx;
        }
        case 12:
        {
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E4A35E]
            rdx ^= r11;             //xor rdx, r11
            rax = baseModuleAddr + 0x1BAF;                //lea rax, [0xFFFFFFFFFE068A69]
            rdx ^= rax;             //xor rdx, rax
            rdx -= r11;             //sub rdx, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFF9F;
            RSP_0xFFFFFFFFFFFFFF9F = baseModuleAddr + 0x259F56F6;                 //lea rax, [0x0000000023A5C924] : RBP+0xFFFFFFFFFFFFFF9F
            rdx += RSP_0xFFFFFFFFFFFFFF9F;          //add rdx, [rbp-0x61]
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rax = 0x294D76F27D0F6D85;               //mov rax, 0x294D76F27D0F6D85
            rdx -= rax;             //sub rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x8;            //shr rax, 0x08
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x10;           //shr rax, 0x10
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x20;           //shr rax, 0x20
            rdx ^= rax;             //xor rdx, rax
            rax = 0x40AE2552A77DAFE6;               //mov rax, 0x40AE2552A77DAFE6
            rdx -= r11;             //sub rdx, r11
            rdx ^= rax;             //xor rdx, rax
            rax = 0x6425FC1CEAFDBD3B;               //mov rax, 0x6425FC1CEAFDBD3B
            rdx *= rax;             //imul rdx, rax
            return rdx;
        }
        case 13:
        {
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E49F66]
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE066E36]
            rdx += rbx;             //add rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x10;           //shr rax, 0x10
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x20;           //shr rax, 0x20
            rdx ^= rax;             //xor rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rax = r11;              //mov rax, r11
            rax -= rbx;             //sub rax, rbx
            rdx += rax;             //add rdx, rax
            rax = 0x7AAF0F372FD53CD5;               //mov rax, 0x7AAF0F372FD53CD5
            rdx *= rax;             //imul rdx, rax
            rax = 0x4BE188FD7D45B824;               //mov rax, 0x4BE188FD7D45B824
            rdx -= rax;             //sub rdx, rax
            rdx ^= r11;             //xor rdx, r11
            rax = rdx;              //mov rax, rdx
            rax >>= 0x25;           //shr rax, 0x25
            rdx ^= rax;             //xor rdx, rax
            rax = 0x2F94247E3E6CDFF6;               //mov rax, 0x2F94247E3E6CDFF6
            rdx -= rax;             //sub rdx, rax
            return rdx;
        }
        case 14:
        {
            r10 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);             //mov r10, [0x0000000007E49AD9]
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE06699E]
            rax = 0x59FC5D34C1D95075;               //mov rax, 0x59FC5D34C1D95075
            rdx += rax;             //add rdx, rax
            rax = 0x113F93E895C764EB;               //mov rax, 0x113F93E895C764EB
            rdx *= rax;             //imul rdx, rax
            rax = baseModuleAddr + 0x3BFCF952;            //lea rax, [0x000000003A035FF5]
            rax = ~rax;             //not rax
            rax ^= r11;             //xor rax, r11
            rdx -= rax;             //sub rdx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r10;             //xor rax, r10
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rdx -= rbx;             //sub rdx, rbx
            rax = rdx;              //mov rax, rdx
            rax >>= 0x23;           //shr rax, 0x23
            rdx ^= rax;             //xor rdx, rax
            uintptr_t RSP_0xFFFFFFFFFFFFFF9F;
            RSP_0xFFFFFFFFFFFFFF9F = 0x3D4F5BB3C70BE95B;            //mov rax, 0x3D4F5BB3C70BE95B : RBP+0xFFFFFFFFFFFFFF9F
            rdx *= RSP_0xFFFFFFFFFFFFFF9F;          //imul rdx, [rbp-0x61]
            rax = r11;              //mov rax, r11
            uintptr_t RSP_0xFFFFFFFFFFFFFFA7;
            RSP_0xFFFFFFFFFFFFFFA7 = baseModuleAddr + 0x9DA7;             //lea rax, [0xFFFFFFFFFE070750] : RBP+0xFFFFFFFFFFFFFFA7
            rax ^= RSP_0xFFFFFFFFFFFFFFA7;          //xor rax, [rbp-0x59]
            rdx -= rax;             //sub rdx, rax
            return rdx;
        }
        case 15:
        {
            rbx = baseModuleAddr;                 //lea rbx, [0xFFFFFFFFFE066520]
            r9 = driver::read<uintptr_t>(baseModuleAddr + 0x9DE3129);              //mov r9, [0x0000000007E495D9]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x13;           //shr rax, 0x13
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x26;           //shr rax, 0x26
            rdx ^= rax;             //xor rdx, rax
            rax = 0x63FD32E967945525;               //mov rax, 0x63FD32E967945525
            rdx -= rax;             //sub rdx, rax
            rdx += r11;             //add rdx, r11
            rax = 0xB85215B9839B7D9;                //mov rax, 0xB85215B9839B7D9
            rdx += rax;             //add rdx, rax
            rdx ^= rbx;             //xor rdx, rbx
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);               //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = _byteswap_uint64(rax);            //bswap rax
            rdx *= driver::read<uintptr_t>(rax + 0x15);              //imul rdx, [rax+0x15]
            rax = rdx;              //mov rax, rdx
            rax >>= 0x5;            //shr rax, 0x05
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0xA;            //shr rax, 0x0A
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x14;           //shr rax, 0x14
            rdx ^= rax;             //xor rdx, rax
            rax = rdx;              //mov rax, rdx
            rax >>= 0x28;           //shr rax, 0x28
            rdx ^= rax;             //xor rdx, rax
            rax = 0xE52EBF353AE32CDB;               //mov rax, 0xE52EBF353AE32CDB
            rdx *= rax;             //imul rdx, rax
            return rdx;
        }
        }
    }

    // OUTDATED
    extern "C" auto STEAMdecrypt_bone_base(uint64_t m_image_process, uint64_t Peb) -> uint64_t
    {
        uint64_t rax = m_image_process, rbx = m_image_process, rcx = m_image_process, rdx = m_image_process, rdi = m_image_process, rsi = m_image_process, r8 = m_image_process, r9 = m_image_process, r10 = m_image_process, r11 = m_image_process, r12 = m_image_process, r13 = m_image_process, r14 = m_image_process, r15 = m_image_process;
        rbx = read<uint64_t>(m_image_process + 0xf8d4e08);
        if (!rbx)
            return rbx;
        r10 = Peb;            //mov r10, gs:[rax]
        //failed to translate: mov [rsp+0xF8], rdi
        rax = r10;              //mov rax, r10
        rax = _rotr64(rax, 0x15);             //ror rax, 0x15
        //failed to translate: mov [rsp+0xC0], r12
        rax &= 0xF;
        switch (rax)
        {
        case 0:
        {
            r12 = m_image_process + 0x4333;                 //lea r12, [0xFFFFFFFFF8E78A5F]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF7821]
            rax = r12;              //mov rax, r12
            rax = ~rax;             //not rax
            rax ^= r10;             //xor rax, r10
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x6;            //shr rax, 0x06
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0xC;            //shr rax, 0x0C
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x18;           //shr rax, 0x18
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x30;           //shr rax, 0x30
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x6;            //shr rax, 0x06
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0xC;            //shr rax, 0x0C
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x18;           //shr rax, 0x18
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x30;           //shr rax, 0x30
            rbx ^= rax;             //xor rbx, rax
            rcx = r10;              //mov rcx, r10
            rcx = ~rcx;             //not rcx
            rax = m_image_process + 0x6582C5E6;             //lea rax, [0x000000005E6A0BEF]
            rax = ~rax;             //not rax
            rcx += rax;             //add rcx, rax
            rbx ^= rcx;             //xor rbx, rcx
            rbx -= r10;             //sub rbx, r10
            rax = rbx;              //mov rax, rbx
            rax >>= 0x19;           //shr rax, 0x19
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x32;           //shr rax, 0x32
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rbx ^= rax;             //xor rbx, rax
            rcx = _rotl64(rcx, 0x10);             //rol rcx, 0x10
            rcx ^= r9;              //xor rcx, r9
            rax = 0x7C19AF7C4499F107;               //mov rax, 0x7C19AF7C4499F107
            rcx = ~rcx;             //not rcx
            rbx *= read<uintptr_t>(rcx + 0x11);             //imul rbx, [rcx+0x11]
            rbx *= rax;             //imul rbx, rax
            return rbx;
        }
        case 1:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E741E5]
            r8 = read<uint64_t>(m_image_process + 0xA183161);             //mov r8, [0x0000000002FF729F]
            rax = r11 + 0x6d97106a;                 //lea rax, [r11+0x6D97106A]
            rax += r10;             //add rax, r10
            rbx += rax;             //add rbx, rax
            rbx ^= r10;             //xor rbx, r10
            rax = rbx;              //mov rax, rbx
            rax >>= 0x15;           //shr rax, 0x15
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x2A;           //shr rax, 0x2A
            rbx ^= rax;             //xor rbx, rax
            rax = 0xFBEB143C64668071;               //mov rax, 0xFBEB143C64668071
            rbx *= rax;             //imul rbx, rax
            rax = 0x20169B99A26E5274;               //mov rax, 0x20169B99A26E5274
            rbx ^= rax;             //xor rbx, rax
            rax = r10;              //mov rax, r10
            rax -= r11;             //sub rax, r11
            rax += 0xFFFFFFFFFFFF71B3;              //add rax, 0xFFFFFFFFFFFF71B3
            rbx += rax;             //add rbx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r8;              //xor rax, r8
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rax = rbx;              //mov rax, rbx
            rax >>= 0xC;            //shr rax, 0x0C
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x18;           //shr rax, 0x18
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x30;           //shr rax, 0x30
            rbx ^= rax;             //xor rbx, rax
            return rbx;
        }
        case 2:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E73CBE]
            r12 = m_image_process + 0x5E7C;                 //lea r12, [0xFFFFFFFFF8E79B2B]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF6D69]
            rbx ^= r11;             //xor rbx, r11
            rbx ^= r11;             //xor rbx, r11
            rax = 0x362F96B8B1C01EC9;               //mov rax, 0x362F96B8B1C01EC9
            rbx *= rax;             //imul rbx, rax
            rax = 0xEA488B91A27508BF;               //mov rax, 0xEA488B91A27508BF
            rbx ^= rax;             //xor rbx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rcx = r10;              //mov rcx, r10
            rcx = ~rcx;             //not rcx
            rax = r12;              //mov rax, r12
            rax = ~rax;             //not rax
            rcx *= rax;             //imul rcx, rax
            rbx += rcx;             //add rbx, rcx
            rax = rbx;              //mov rax, rbx
            rax >>= 0xB;            //shr rax, 0x0B
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x16;           //shr rax, 0x16
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x2C;           //shr rax, 0x2C
            rbx ^= rax;             //xor rbx, rax
            rax = 0xAD49091EBE946DCA;               //mov rax, 0xAD49091EBE946DCA
            rbx ^= rax;             //xor rbx, rax
            return rbx;
        }
        case 3:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E736A7]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF67A7]
            rax = rbx;              //mov rax, rbx
            rax >>= 0x21;           //shr rax, 0x21
            rbx ^= rax;             //xor rbx, rax
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);             //rol rcx, 0x10
            rax = 0x856C3BC5BF3A0D89;               //mov rax, 0x856C3BC5BF3A0D89
            rbx *= rax;             //imul rbx, rax
            rcx ^= r9;              //xor rcx, r9
            rax = 0x4A75F674E2A7C59D;               //mov rax, 0x4A75F674E2A7C59D
            rbx ^= r10;             //xor rbx, r10
            rcx = ~rcx;             //not rcx
            rbx *= read<uintptr_t>(rcx + 0x11);             //imul rbx, [rcx+0x11]
            rbx *= rax;             //imul rbx, rax
            rbx += r11;             //add rbx, r11
            rax = rbx;              //mov rax, rbx
            rax >>= 0x5;            //shr rax, 0x05
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0xA;            //shr rax, 0x0A
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x14;           //shr rax, 0x14
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x28;           //shr rax, 0x28
            rbx ^= rax;             //xor rbx, rax
            rax = 0x5DBFEDA0BB2BF024;               //mov rax, 0x5DBFEDA0BB2BF024
            rbx ^= rax;             //xor rbx, rax
            return rbx;
        }
        case 4:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E73247]
            r8 = read<uint64_t>(m_image_process + 0xA183161);             //mov r8, [0x0000000002FF6317]
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r8;              //xor rax, r8
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rbx += r11;             //add rbx, r11
            rax = rbx;              //mov rax, rbx
            rax >>= 0x22;           //shr rax, 0x22
            rax ^= r10;             //xor rax, r10
            rbx ^= rax;             //xor rbx, rax
            rax = 0xD82686167AF35547;               //mov rax, 0xD82686167AF35547
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x16;           //shr rax, 0x16
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x2C;           //shr rax, 0x2C
            rbx ^= rax;             //xor rbx, rax
            rax = 0x23F276B7C5E79E59;               //mov rax, 0x23F276B7C5E79E59
            rbx *= rax;             //imul rbx, rax
            return rbx;
        }
        case 5:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E72CC5]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF5DC9]
            rax = rbx;              //mov rax, rbx
            rax >>= 0x5;            //shr rax, 0x05
            rbx ^= rax;             //xor rbx, rax
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rax = rbx;              //mov rax, rbx
            rcx = _rotl64(rcx, 0x10);             //rol rcx, 0x10
            rax >>= 0xA;            //shr rax, 0x0A
            rcx ^= r9;              //xor rcx, r9
            rbx ^= rax;             //xor rbx, rax
            rcx = ~rcx;             //not rcx
            rax = rbx;              //mov rax, rbx
            rax >>= 0x14;           //shr rax, 0x14
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x28;           //shr rax, 0x28
            rbx ^= rax;             //xor rbx, rax
            rbx *= read<uintptr_t>(rcx + 0x11);             //imul rbx, [rcx+0x11]
            rax = rbx;              //mov rax, rbx
            rax >>= 0x11;           //shr rax, 0x11
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x22;           //shr rax, 0x22
            rbx ^= rax;             //xor rbx, rax
            rax = r11 + 0x6db3aefb;                 //lea rax, [r11+0x6DB3AEFB]
            rax += r10;             //add rax, r10
            rbx += rax;             //add rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x1F;           //shr rax, 0x1F
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x3E;           //shr rax, 0x3E
            rbx ^= rax;             //xor rbx, rax
            rax = 0x716323DA738B7D55;               //mov rax, 0x716323DA738B7D55
            rbx *= rax;             //imul rbx, rax
            rax = 0x5019B3C98E4C64B2;               //mov rax, 0x5019B3C98E4C64B2
            rbx += rax;             //add rbx, rax
            rbx += r10;             //add rbx, r10
            return rbx;
        }
        case 6:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E72772]
            r15 = m_image_process + 0x5FF9780B;             //lea r15, [0x0000000058E09F6E]
            r8 = read<uint64_t>(m_image_process + 0xA183161);             //mov r8, [0x0000000002FF5841]
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r8;              //xor rax, r8
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rax = rbx;              //mov rax, rbx
            rax >>= 0x1C;           //shr rax, 0x1C
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x38;           //shr rax, 0x38
            rbx ^= rax;             //xor rbx, rax
            rbx -= r11;             //sub rbx, r11
            rax = r10;              //mov rax, r10
            rax ^= r15;             //xor rax, r15
            rbx -= rax;             //sub rbx, rax
            rax = 0x17AF57BCC08DEEDE;               //mov rax, 0x17AF57BCC08DEEDE
            rbx ^= rax;             //xor rbx, rax
            rax = 0xC4AC99AD39D9A9C9;               //mov rax, 0xC4AC99AD39D9A9C9
            rbx *= rax;             //imul rbx, rax
            rax = 0xEDBE9B1BFC7A5AA0;               //mov rax, 0xEDBE9B1BFC7A5AA0
            rax -= r10;             //sub rax, r10
            rbx += rax;             //add rbx, rax
            return rbx;
        }
        case 7:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E72359]
            r8 = read<uint64_t>(m_image_process + 0xA183161);             //mov r8, [0x0000000002FF53E1]
            rbx -= r10;             //sub rbx, r10
            rbx += r10;             //add rbx, r10
            rax = 0xA5AB20AF9D2FC16D;               //mov rax, 0xA5AB20AF9D2FC16D
            rbx ^= rax;             //xor rbx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r8;              //xor rax, r8
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rbx -= r11;             //sub rbx, r11
            rax = 0xB35D61AF6BB15181;               //mov rax, 0xB35D61AF6BB15181
            rbx ^= rax;             //xor rbx, rax
            rax = 0x3518A4B0004228B1;               //mov rax, 0x3518A4B0004228B1
            rbx *= rax;             //imul rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x24;           //shr rax, 0x24
            rbx ^= rax;             //xor rbx, rax
            return rbx;
        }
        case 8:
        {
            r12 = m_image_process + 0x20BE5800;             //lea r12, [0x0000000019A57552]
            r13 = m_image_process + 0x9C7C;                 //lea r13, [0xFFFFFFFFF8E7B9BF]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF4E0F]
            rax = rbx;              //mov rax, rbx
            rax >>= 0xD;            //shr rax, 0x0D
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x1A;           //shr rax, 0x1A
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x34;           //shr rax, 0x34
            rbx ^= rax;             //xor rbx, rax
            rax = r12;              //mov rax, r12
            rax = ~rax;             //not rax
            rax -= r10;             //sub rax, r10
            rbx += rax;             //add rbx, rax
            rbx ^= r10;             //xor rbx, r10
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rcx = _rotl64(rcx, 0x10);             //rol rcx, 0x10
            rcx ^= r9;              //xor rcx, r9
            rax = 0x631CCDCAFAB77C20;               //mov rax, 0x631CCDCAFAB77C20
            rbx += rax;             //add rbx, rax
            rcx = ~rcx;             //not rcx
            rax = 0x711C7C2F7B657C16;               //mov rax, 0x711C7C2F7B657C16
            rbx ^= rax;             //xor rbx, rax
            rax = r13;              //mov rax, r13
            rax = ~rax;             //not rax
            rax *= r10;             //imul rax, r10
            rbx += rax;             //add rbx, rax
            rax = 0x7D7E483C1129BEAF;               //mov rax, 0x7D7E483C1129BEAF
            rbx *= read<uintptr_t>(rcx + 0x11);             //imul rbx, [rcx+0x11]
            rbx *= rax;             //imul rbx, rax
            return rbx;
        }
        case 9:
        {
            r12 = m_image_process + 0x59B42F6B;             //lea r12, [0x00000000529B4773]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF48DD]
            rbx ^= r10;             //xor rbx, r10
            rax = 0xEE50467079EBF139;               //mov rax, 0xEE50467079EBF139
            rbx *= rax;             //imul rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x23;           //shr rax, 0x23
            rbx ^= rax;             //xor rbx, rax
            rcx = r10 + 0x1;                //lea rcx, [r10+0x01]
            rcx *= r12;             //imul rcx, r12
            rax = m_image_process + 0x3D5C41D1;             //lea rax, [0x0000000036435774]
            rcx -= r10;             //sub rcx, r10
            rbx += rax;             //add rbx, rax
            rbx += rcx;             //add rbx, rcx
            rax = 0x94D397D8E6D9DB27;               //mov rax, 0x94D397D8E6D9DB27
            rbx *= rax;             //imul rbx, rax
            rax = 0x4EA04E63FA05D4D9;               //mov rax, 0x4EA04E63FA05D4D9
            rbx *= rax;             //imul rbx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            return rbx;
        }
        case 10:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E71325]
            r15 = m_image_process + 0x2D82BED2;             //lea r15, [0x000000002669D1E8]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF4428]
            rax = 0xFE00ED5AAA8FC77;                //mov rax, 0xFE00ED5AAA8FC77
            rax -= r10;             //sub rax, r10
            rbx += rax;             //add rbx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rax = rbx;              //mov rax, rbx
            rax >>= 0xE;            //shr rax, 0x0E
            rbx ^= rax;             //xor rbx, rax
            rcx = r11 + 0x230c3abe;                 //lea rcx, [r11+0x230C3ABE]
            rcx += r10;             //add rcx, r10
            rax = rbx;              //mov rax, rbx
            rax >>= 0x1C;           //shr rax, 0x1C
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x38;           //shr rax, 0x38
            rcx ^= rax;             //xor rcx, rax
            rax = r10;              //mov rax, r10
            rax *= r15;             //imul rax, r15
            rbx ^= rcx;             //xor rbx, rcx
            rbx -= rax;             //sub rbx, rax
            rbx += r11;             //add rbx, r11
            rax = 0x80A3409D5778D453;               //mov rax, 0x80A3409D5778D453
            rbx *= rax;             //imul rbx, rax
            return rbx;
        }
        case 11:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E70E91]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF3F99]
            rax = rbx;              //mov rax, rbx
            rax >>= 0x27;           //shr rax, 0x27
            rbx ^= rax;             //xor rbx, rax
            rbx -= r11;             //sub rbx, r11
            rax = rbx;              //mov rax, rbx
            rax >>= 0x16;           //shr rax, 0x16
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x2C;           //shr rax, 0x2C
            rbx ^= rax;             //xor rbx, rax
            rax = 0x1E19026D2976AB79;               //mov rax, 0x1E19026D2976AB79
            rbx *= rax;             //imul rbx, rax
            rax = 0xDAC84F581A21AE32;               //mov rax, 0xDAC84F581A21AE32
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0xB;            //shr rax, 0x0B
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x16;           //shr rax, 0x16
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x2C;           //shr rax, 0x2C
            rbx ^= rax;             //xor rbx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rax = 0xE81D2012BCFAB4F3;               //mov rax, 0xE81D2012BCFAB4F3
            rbx *= rax;             //imul rbx, rax
            return rbx;
        }
        case 12:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E709FC]
            r8 = read<uint64_t>(m_image_process + 0xA183161);             //mov r8, [0x0000000002FF3AAC]
            rbx -= r10;             //sub rbx, r10
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r8;              //xor rax, r8
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rax = r10;              //mov rax, r10
            rax -= r11;             //sub rax, r11
            rax -= 0x5CE7CB97;              //sub rax, 0x5CE7CB97
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x26;           //shr rax, 0x26
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x17;           //shr rax, 0x17
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x2E;           //shr rax, 0x2E
            rbx ^= rax;             //xor rbx, rax
            rax = 0x30C122163E777053;               //mov rax, 0x30C122163E777053
            rbx ^= rax;             //xor rbx, rax
            rax = 0xB0A45BF174307958;               //mov rax, 0xB0A45BF174307958
            rbx ^= rax;             //xor rbx, rax
            rax = 0xFD2A98F005A01345;               //mov rax, 0xFD2A98F005A01345
            rbx *= rax;             //imul rbx, rax
            return rbx;
        }
        case 13:
        {
            r12 = m_image_process + 0x4181;                 //lea r12, [0xFFFFFFFFF8E746E5]
            r13 = m_image_process + 0x1F0354E2;             //lea r13, [0x0000000017EA5A37]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF3620]
            rax = rbx;              //mov rax, rbx
            rax >>= 0x26;           //shr rax, 0x26
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x20;           //shr rax, 0x20
            rbx ^= rax;             //xor rbx, rax
            rax = r12;              //mov rax, r12
            rax = ~rax;             //not rax
            rax ^= r10;             //xor rax, r10
            rbx -= rax;             //sub rbx, rax
            rax = 0x6FCFE26D137B1CE7;               //mov rax, 0x6FCFE26D137B1CE7
            rbx *= rax;             //imul rbx, rax
            uint64_t RSP_0xFFFFFFFFFFFFFFAF;
            RSP_0xFFFFFFFFFFFFFFAF = 0xD736648F0A792C53;            //mov rax, 0xD736648F0A792C53 : RBP+0xFFFFFFFFFFFFFFAF
            rbx ^= RSP_0xFFFFFFFFFFFFFFAF;          //xor rbx, [rbp-0x51]
            rax = r13;              //mov rax, r13
            rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
            rax *= r10;             //imul rax, r10
            rcx = _rotl64(rcx, 0x10);             //rol rcx, 0x10
            rbx -= rax;             //sub rbx, rax
            rcx ^= r9;              //xor rcx, r9
            rcx = ~rcx;             //not rcx
            rax = 0x6759DD723B6E15F5;               //mov rax, 0x6759DD723B6E15F5
            rbx *= read<uintptr_t>(rcx + 0x11);             //imul rbx, [rcx+0x11]
            rbx *= rax;             //imul rbx, rax
            return rbx;
        }
        case 14:
        {
            r11 = m_image_process;          //lea r11, [0xFFFFFFFFF8E700B4]
            r9 = read<uint64_t>(m_image_process + 0xA183161);             //mov r9, [0x0000000002FF31BC]
            rax = r10;              //mov rax, r10
            rax = ~rax;             //not rax
            rax -= r11;             //sub rax, r11
            rax -= 0x30DC0A32;              //sub rax, 0x30DC0A32
            rbx ^= rax;             //xor rbx, rax
            rax = 0x5AB6023BC236FCFF;               //mov rax, 0x5AB6023BC236FCFF
            rbx += rax;             //add rbx, rax
            rax = 0xD41FB1FE844360CB;               //mov rax, 0xD41FB1FE844360CB
            rbx ^= rax;             //xor rbx, rax
            rbx -= r11;             //sub rbx, r11
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r9;              //xor rax, r9
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rax = rbx;              //mov rax, rbx
            rbx = rax;              //mov rbx, rax
            rbx >>= 0x20;           //shr rbx, 0x20
            rbx ^= rax;             //xor rbx, rax
            rax = 0x7FFE67E03E59CDF1;               //mov rax, 0x7FFE67E03E59CDF1
            rbx *= rax;             //imul rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x27;           //shr rax, 0x27
            rbx ^= rax;             //xor rbx, rax
            return rbx;
        }
        case 15:
        {
            r12 = m_image_process + 0x30AB4C3C;             //lea r12, [0x000000002992485B]
            r8 = read<uint64_t>(m_image_process + 0xA183161);             //mov r8, [0x0000000002FF2CC9]
            rax = rbx;              //mov rax, rbx
            rax >>= 0x26;           //shr rax, 0x26
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x18;           //shr rax, 0x18
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x30;           //shr rax, 0x30
            rbx ^= rax;             //xor rbx, rax
            rax = 0x3FF06DFD54AB90AC;               //mov rax, 0x3FF06DFD54AB90AC
            rbx += rax;             //add rbx, rax
            rax = 0x52504D6AB22C210B;               //mov rax, 0x52504D6AB22C210B
            rbx *= rax;             //imul rbx, rax
            rax = 0;                //and rax, 0xFFFFFFFFC0000000
            rax = _rotl64(rax, 0x10);             //rol rax, 0x10
            rax ^= r8;              //xor rax, r8
            rax = ~rax;             //not rax
            rbx *= read<uintptr_t>(rax + 0x11);             //imul rbx, [rax+0x11]
            rax = m_image_process + 0x7236938D;             //lea rax, [0x000000006B1D8CD3]
            rax += r10;             //add rax, r10
            rbx ^= rax;             //xor rbx, rax
            rax = rbx;              //mov rax, rbx
            rax >>= 0x23;           //shr rax, 0x23
            rbx ^= rax;             //xor rbx, rax
            rax = r12;              //mov rax, r12
            rax *= r10;             //imul rax, r10
            rbx ^= rax;             //xor rbx, rax
            return rbx;
        }
        }

        return 0;
    }

    // OUTDATED
    extern "C" auto STEAMget_bone_index(uint32_t bone_index, uint64_t baseModuleAddr) -> uint64_t
    {
        uint64_t rax = baseModuleAddr, rbx = baseModuleAddr, rcx = baseModuleAddr, rdx = baseModuleAddr, rdi = baseModuleAddr, rsi = baseModuleAddr, r8 = baseModuleAddr, r9 = baseModuleAddr, r10 = baseModuleAddr, r11 = baseModuleAddr, r12 = baseModuleAddr, r13 = baseModuleAddr, r14 = baseModuleAddr, r15 = baseModuleAddr;
        rdi = bone_index;
        rcx = rdi * 0x13C8;
        rax = 0x1B5C5E9652FDACE7;               //mov rax, 0x1B5C5E9652FDACE7
        rax = _umul128(rax, rcx, (uintptr_t*)&rdx);             //mul rcx
        r11 = baseModuleAddr;           //lea r11, [0xFFFFFFFFFD8943D8]
        r10 = 0x19E9C4E0C9861BBD;               //mov r10, 0x19E9C4E0C9861BBD
        rdx >>= 0xA;            //shr rdx, 0x0A
        rax = rdx * 0x256D;             //imul rax, rdx, 0x256D
        rcx -= rax;             //sub rcx, rax
        rax = 0x4F9FF77A70376427;               //mov rax, 0x4F9FF77A70376427
        r8 = rcx * 0x256D;              //imul r8, rcx, 0x256D
        rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
        rax = r8;               //mov rax, r8
        rax -= rdx;             //sub rax, rdx
        rax >>= 0x1;            //shr rax, 0x01
        rax += rdx;             //add rax, rdx
        rax >>= 0xD;            //shr rax, 0x0D
        rax = rax * 0x30D1;             //imul rax, rax, 0x30D1
        r8 -= rax;              //sub r8, rax
        rax = 0x70381C0E070381C1;               //mov rax, 0x70381C0E070381C1
        rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
        rax = 0x624DD2F1A9FBE77;                //mov rax, 0x624DD2F1A9FBE77
        rdx >>= 0x6;            //shr rdx, 0x06
        rcx = rdx * 0x92;               //imul rcx, rdx, 0x92
        rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
        rax = r8;               //mov rax, r8
        rax -= rdx;             //sub rax, rdx
        rax >>= 0x1;            //shr rax, 0x01
        rax += rdx;             //add rax, rdx
        rax >>= 0x6;            //shr rax, 0x06
        rcx += rax;             //add rcx, rax
        rax = rcx * 0xFA;               //imul rax, rcx, 0xFA
        rcx = r8 * 0xFC;                //imul rcx, r8, 0xFC
        rcx -= rax;             //sub rcx, rax
        rax = driver::read<uint16_t>(rcx + r11 * 1 + 0x9E9BBE0);                //movzx eax, word ptr [rcx+r11*1+0x9E9BBE0]
        r8 = rax * 0x13C8;              //imul r8, rax, 0x13C8
        rax = r10;              //mov rax, r10
        rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
        rcx = r8;               //mov rcx, r8
        rax = r10;              //mov rax, r10
        rcx -= rdx;             //sub rcx, rdx
        rcx >>= 0x1;            //shr rcx, 0x01
        rcx += rdx;             //add rcx, rdx
        rcx >>= 0xC;            //shr rcx, 0x0C
        rcx = rcx * 0x1D0F;             //imul rcx, rcx, 0x1D0F
        r8 -= rcx;              //sub r8, rcx
        r9 = r8 * 0x3981;               //imul r9, r8, 0x3981
        rax = _umul128(rax, r9, (uintptr_t*)&rdx);              //mul r9
        rax = r9;               //mov rax, r9
        rax -= rdx;             //sub rax, rdx
        rax >>= 0x1;            //shr rax, 0x01
        rax += rdx;             //add rax, rdx
        rax >>= 0xC;            //shr rax, 0x0C
        rax = rax * 0x1D0F;             //imul rax, rax, 0x1D0F
        r9 -= rax;              //sub r9, rax
        rax = 0xD79435E50D79435F;               //mov rax, 0xD79435E50D79435F
        rax = _umul128(rax, r9, (uintptr_t*)&rdx);              //mul r9
        rax = 0xA6810A6810A6811;                //mov rax, 0xA6810A6810A6811
        rdx >>= 0x6;            //shr rdx, 0x06
        rcx = rdx * 0x4C;               //imul rcx, rdx, 0x4C
        rax = _umul128(rax, r9, (uintptr_t*)&rdx);              //mul r9
        rax = r9;               //mov rax, r9
        rax -= rdx;             //sub rax, rdx
        rax >>= 0x1;            //shr rax, 0x01
        rax += rdx;             //add rax, rdx
        rax >>= 0x6;            //shr rax, 0x06
        rcx += rax;             //add rcx, rax
        rax = rcx * 0xF6;               //imul rax, rcx, 0xF6
        rcx = r9 * 0xF8;                //imul rcx, r9, 0xF8
        rcx -= rax;             //sub rcx, rax
        r15 = driver::read<uint16_t>(rcx + r11 * 1 + 0x9EA4A80);                //movsx r15d, word ptr [rcx+r11*1+0x9EA4A80]
        return r15;
    }
}