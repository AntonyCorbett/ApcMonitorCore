#include "pch.h"
#include "EdidParser.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace ApcMonitorCoreTests
{
    // -----------------------------------------------------------------------
    // Edid::Trim
    // -----------------------------------------------------------------------
    TEST_CLASS(TrimTests)
    {
    public:
        TEST_METHOD(EmptyString_ReturnsEmpty)
        {
            Assert::AreEqual(std::wstring{}, Edid::Trim(L""));
        }

        TEST_METHOD(WhitespaceOnly_ReturnsEmpty)
        {
            Assert::AreEqual(std::wstring{}, Edid::Trim(L"   \t\r\n"));
        }

        TEST_METHOD(LeadingSpaces_Trimmed)
        {
            Assert::AreEqual(std::wstring{ L"hello" }, Edid::Trim(L"   hello"));
        }

        TEST_METHOD(TrailingSpaces_Trimmed)
        {
            Assert::AreEqual(std::wstring{ L"hello" }, Edid::Trim(L"hello   "));
        }

        TEST_METHOD(BothEnds_Trimmed)
        {
            Assert::AreEqual(std::wstring{ L"hello world" }, Edid::Trim(L"  hello world  "));
        }

        TEST_METHOD(NoWhitespace_Unchanged)
        {
            Assert::AreEqual(std::wstring{ L"ABC123" }, Edid::Trim(L"ABC123"));
        }
    };

    // -----------------------------------------------------------------------
    // Edid::IsLikelyValidSerial
    // -----------------------------------------------------------------------
    TEST_CLASS(IsLikelyValidSerialTests)
    {
    public:
        TEST_METHOD(EmptyString_ReturnsFalse)
        {
            Assert::IsFalse(Edid::IsLikelyValidSerial(L""));
        }

        TEST_METHOD(WhitespaceOnly_ReturnsFalse)
        {
            Assert::IsFalse(Edid::IsLikelyValidSerial(L"   "));
        }

        TEST_METHOD(KnownBadSerial_AllZeros_ReturnsFalse)
        {
            Assert::IsFalse(Edid::IsLikelyValidSerial(L"00000000"));
        }

        TEST_METHOD(KnownBadSerial_AllFFs_ReturnsFalse)
        {
            Assert::IsFalse(Edid::IsLikelyValidSerial(L"FFFFFFFF"));
        }

        TEST_METHOD(PunctuationOnly_ReturnsFalse)
        {
            Assert::IsFalse(Edid::IsLikelyValidSerial(L"!@#$%^&*"));
        }

        TEST_METHOD(SingleDigit_ReturnsTrue)
        {
            Assert::IsTrue(Edid::IsLikelyValidSerial(L"1"));
        }

        TEST_METHOD(TypicalSerial_ReturnsTrue)
        {
            Assert::IsTrue(Edid::IsLikelyValidSerial(L"335014EEC4201"));
        }

        TEST_METHOD(SerialWithLeadingWhitespace_ReturnsTrue)
        {
            Assert::IsTrue(Edid::IsLikelyValidSerial(L"  ABC123  "));
        }
    };

    // -----------------------------------------------------------------------
    // Edid::ParseSerial
    // -----------------------------------------------------------------------

    // Builds a zeroed 128-byte base EDID block.
    static std::vector<BYTE> MakeBaseEdid()
    {
        return std::vector<BYTE>(128, 0x00);
    }

    // Writes an ASCII monitor-descriptor at descriptor slot [slot] (0-3).
    // type: 0xFF = Serial Number, 0xFE = ASCII String
    static void WriteDescriptor(std::vector<BYTE>& edid, int slot, BYTE type, const char* text)
    {
        const size_t base = 0x36 + static_cast<size_t>(slot) * 18;
        edid[base + 0] = 0x00;
        edid[base + 1] = 0x00;
        edid[base + 2] = 0x00;
        edid[base + 3] = type;
        edid[base + 4] = 0x00;
        size_t j = 0;
        while (text[j] && j < 13)
        {
            edid[base + 5 + j] = static_cast<BYTE>(text[j]);
            ++j;
        }
        if (j < 13) edid[base + 5 + j] = 0x0A; // newline terminator
    }

    TEST_CLASS(ParseSerialTests)
    {
    public:
        TEST_METHOD(NullPointer_ReturnsEmpty)
        {
            Assert::AreEqual(std::wstring{}, Edid::ParseSerial(nullptr, 128));
        }

        TEST_METHOD(SizeLessThan128_ReturnsEmpty)
        {
            auto edid = MakeBaseEdid();
            Assert::AreEqual(std::wstring{}, Edid::ParseSerial(edid.data(), 127));
        }

        TEST_METHOD(NoDescriptors_ReturnsEmpty)
        {
            auto edid = MakeBaseEdid();
            // All descriptor slots are zero (timing descriptors) → no serial, hex fallback bytes 12-15 also 0
            Assert::AreEqual(std::wstring{}, Edid::ParseSerial(edid.data(), 128));
        }

        TEST_METHOD(SerialDescriptor_0xFF_ExtractsSerial)
        {
            auto edid = MakeBaseEdid();
            WriteDescriptor(edid, 0, 0xFF, "TEST123");
            Assert::AreEqual(std::wstring{ L"TEST123" }, Edid::ParseSerial(edid.data(), 128));
        }

        TEST_METHOD(AsciiStringDescriptor_0xFE_ExtractsSerial)
        {
            auto edid = MakeBaseEdid();
            WriteDescriptor(edid, 0, 0xFE, "VND12345");
            Assert::AreEqual(std::wstring{ L"VND12345" }, Edid::ParseSerial(edid.data(), 128));
        }

        TEST_METHOD(SerialInSecondSlot_ExtractsSerial)
        {
            auto edid = MakeBaseEdid();
            WriteDescriptor(edid, 1, 0xFF, "SN-SLOT1");
            Assert::AreEqual(std::wstring{ L"SN-SLOT1" }, Edid::ParseSerial(edid.data(), 128));
        }

        TEST_METHOD(HexFallback_NonZeroBytes12to15_ReturnsHex)
        {
            auto edid = MakeBaseEdid();
            // Bytes 12-15: little-endian 0x12345678
            edid[12] = 0x78; edid[13] = 0x56; edid[14] = 0x34; edid[15] = 0x12;
            Assert::AreEqual(std::wstring{ L"12345678" }, Edid::ParseSerial(edid.data(), 128));
        }

        TEST_METHOD(HexFallback_ZeroBytes_ReturnsEmpty)
        {
            auto edid = MakeBaseEdid();
            // bytes 12-15 are already 0
            Assert::AreEqual(std::wstring{}, Edid::ParseSerial(edid.data(), 128));
        }

        TEST_METHOD(HexFallback_AllFFBytes_ReturnsEmpty)
        {
            auto edid = MakeBaseEdid();
            edid[12] = 0xFF; edid[13] = 0xFF; edid[14] = 0xFF; edid[15] = 0xFF;
            Assert::AreEqual(std::wstring{}, Edid::ParseSerial(edid.data(), 128));
        }

        // Fix 4 regression: CEA-861 extension block must be skipped.
        // If the extension block tag is 0x02 (CEA), ParseSerial must not
        // misinterpret its data as base-EDID descriptors.
        TEST_METHOD(CeaExtensionBlock_IsSkipped_ReturnsEmpty)
        {
            std::vector<BYTE> edid(256, 0x00);
            edid[0x7E] = 1; // one extension block

            // Extension block at byte 128: tag 0x02 = CEA-861
            edid[128] = 0x02;

            // Plant a fake "serial descriptor" at 0x36 within the extension block.
            // Without Fix 4 this would be misread as a serial.
            const size_t extBase = 128 + 0x36;
            edid[extBase + 0] = 0x00;
            edid[extBase + 1] = 0x00;
            edid[extBase + 2] = 0x00;
            edid[extBase + 3] = 0xFF;
            edid[extBase + 5] = 'F'; edid[extBase + 6] = 'A';
            edid[extBase + 7] = 'K'; edid[extBase + 8] = 'E';
            edid[extBase + 9] = '1'; edid[extBase + 10] = 0x0A;

            // With Fix 4: CEA block skipped → no serial found → bytes 12-15 are 0 → empty
            Assert::AreEqual(std::wstring{}, Edid::ParseSerial(edid.data(), 256));
        }

        // Contrast: a tag-0x00 extension block IS parsed (same layout as base EDID).
        TEST_METHOD(Tag0x00ExtensionBlock_IsParsed_ExtractsSerial)
        {
            std::vector<BYTE> edid(256, 0x00);
            edid[0x7E] = 1;

            // Extension block at byte 128: tag 0x00 = base-EDID style
            edid[128] = 0x00;

            WriteDescriptor(edid, 0, 0xFF, ""); // base block has no serial
            const size_t extBase = 128;
            // Write descriptor into slot 0 of the extension block
            edid[extBase + 0x36 + 0] = 0x00;
            edid[extBase + 0x36 + 1] = 0x00;
            edid[extBase + 0x36 + 2] = 0x00;
            edid[extBase + 0x36 + 3] = 0xFF;
            edid[extBase + 0x36 + 5] = 'E'; edid[extBase + 0x36 + 6] = 'X';
            edid[extBase + 0x36 + 7] = 'T'; edid[extBase + 0x36 + 8] = 'S';
            edid[extBase + 0x36 + 9] = 'N'; edid[extBase + 0x36 + 10] = 0x0A;

            Assert::AreEqual(std::wstring{ L"EXTSN" }, Edid::ParseSerial(edid.data(), 256));
        }

        TEST_METHOD(ExtensionBlockBeyondBuffer_IsSkipped)
        {
            std::vector<BYTE> edid(128, 0x00); // only 128 bytes
            edid[0x7E] = 1; // claims 1 extension, but buffer is too short
            // Should not read out of bounds
            Assert::AreEqual(std::wstring{}, Edid::ParseSerial(edid.data(), 128));
        }
    };
}
