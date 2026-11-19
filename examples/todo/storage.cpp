#include "todo/storage.hpp"

#include <fstream>
#include <iterator>
#include <stdexcept>

namespace lamina::examples::todo {

std::optional<std::string> read_file(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return std::nullopt;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open " + path.string());
    }
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (in.bad()) {
        throw std::runtime_error("cannot read " + path.string());
    }
    return text;
}

void write_file_replacing(const std::filesystem::path& path, std::string_view contents) {
    std::filesystem::path temporary = path;
    temporary += ".new";
    {
        std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
        out.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        out.flush();
        if (!out) {
            throw std::runtime_error("cannot write " + temporary.string());
        }
    }
    std::filesystem::rename(temporary, path);
}

}
