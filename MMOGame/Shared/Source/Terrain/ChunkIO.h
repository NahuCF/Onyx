#pragma once

#include "ChunkFormat.h"
#include <fstream>
#include <string>

namespace MMO {

// ---- String IO helpers ----
inline void WriteString(std::ofstream& f, const std::string& s) {
    uint16_t len = static_cast<uint16_t>(s.size());
    f.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) f.write(s.data(), len);
}

inline std::string ReadString(std::ifstream& f) {
    uint16_t len = 0;
    f.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len == 0 || len > MAX_STRING_LENGTH) return {};
    std::string s(len, '\0');
    f.read(s.data(), len);
    return s;
}

// ---- RAII section writer (tag + size placeholder + auto-patch) ----
class SectionWriter {
public:
    SectionWriter(std::ofstream& f, uint32_t tag)
        : m_File(f)
    {
        f.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
        m_SizePos = f.tellp();
        uint32_t placeholder = 0;
        f.write(reinterpret_cast<const char*>(&placeholder), sizeof(placeholder));
        m_DataStart = f.tellp();
    }

    ~SectionWriter() { if (!m_Finalized) Finalize(); }

    void Finalize() {
        if (m_Finalized) return;
        m_Finalized = true;
        auto dataEnd = m_File.tellp();
        uint32_t size = static_cast<uint32_t>(dataEnd - m_DataStart);
        m_File.seekp(m_SizePos);
        m_File.write(reinterpret_cast<const char*>(&size), sizeof(size));
        m_File.seekp(dataEnd);
    }

private:
    std::ofstream& m_File;
    std::streampos m_SizePos;
    std::streampos m_DataStart;
    bool m_Finalized = false;
};

} // namespace MMO
