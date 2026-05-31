#include "EdidParser.h"
#include <cstring>

namespace
{
    std::wstring ExtractAsciiDescriptor(const BYTE* desc)
    {
        // bytes [5..17] are ASCII text terminated by 0x0A or 0x00
        wchar_t wbuf[14] = {};
        for (int j = 0; j < 13; ++j)
        {
            const BYTE ch = desc[5 + j];
            if (ch == 0x0A || ch == 0x00)
            {
                break;
            }

            if (ch < 0x20 || ch > 0x7E)
            {
                wbuf[j] = L'?';
                continue;
            }

            wbuf[j] = static_cast<wchar_t>(ch);
        }

        return Edid::Trim(std::wstring(wbuf));
    }

    std::wstring ParseSerialFromBlock(const BYTE* edid128)
    {
        if (!edid128)
        {
            return L"";
        }

        constexpr size_t kDescriptorCount = 4u;

        for (size_t i = 0; i < kDescriptorCount; ++i)
        {
            constexpr size_t base = 0x36;
            constexpr size_t kDescriptorSize = 18u;

            const BYTE* desc = edid128 + base + i * kDescriptorSize;
            if (desc[0] == 0x00 && desc[1] == 0x00 && desc[2] == 0x00)
            {
                // 0xFF: Monitor Serial Number; 0xFE: ASCII String (sometimes used by vendors)
                if (desc[3] == 0xFF || desc[3] == 0xFE)
                {
                    auto s = ExtractAsciiDescriptor(desc);
                    if (Edid::IsLikelyValidSerial(s))
                    {
                        return s;
                    }
                }
            }
        }

        return L"";
    }
}

namespace Edid
{
    std::wstring Trim(const std::wstring& s)
    {
        const size_t start = s.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos)
        {
            return L"";
        }

        const size_t end = s.find_last_not_of(L" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    bool IsLikelyValidSerial(const std::wstring& s)
    {
        const auto t = Trim(s);
        if (t.empty())
        {
            return false;
        }

        if (t == L"00000000" || t == L"FFFFFFFF")
        {
            return false;
        }

        // Reject strings with no alphanumeric characters
        bool hasAlnum = false;
        for (const wchar_t c : t)
        {
            if (iswalnum(c))
            {
                hasAlnum = true;
                break;
            }
        }

        return hasAlnum;
    }

    std::wstring ParseSerial(const BYTE* edid, const DWORD size)
    {
        if (!edid || size < 128)
        {
            return L"";
        }

        // Try base block descriptors
        auto s = ParseSerialFromBlock(edid);
        if (IsLikelyValidSerial(s))
        {
            return s;
        }

        // Try extension blocks — only base-EDID-style blocks (tag 0x00) have
        // descriptors at 0x36. CEA-861 (0x02) and others have different layouts.
        const BYTE extCount = edid[0x7E];
        for (int i = 0; i < extCount; ++i)
        {
            const size_t off = 128ull * (i + 1);
            if (off + 128 > size)
            {
                continue;
            }

            if (edid[off] != 0x00)
            {
                continue;
            }

            auto se = ParseSerialFromBlock(edid + off);
            if (IsLikelyValidSerial(se))
            {
                return se;
            }
        }

        // Fallback: 4-byte serial at bytes 12..15 rendered as hex
        if (size >= 16)
        {
            DWORD ser = 0;
            memcpy(&ser, edid + 12, sizeof(DWORD));
            if (ser != 0 && ser != 0xFFFFFFFF)
            {
                wchar_t wbuf[16];
                (void)swprintf_s(wbuf, L"%08X", ser);
                return std::wstring{ wbuf };
            }
        }

        return L"";
    }
}
