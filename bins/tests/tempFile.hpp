#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <unistd.h>

// RAII temp file: creates a uniquely-named file with the given content under
// the system temp directory and removes it on destruction.
// Guarantees cleanup even when the test body throws.
struct TempFile {
    std::filesystem::path path;

    explicit TempFile(const std::string& content) {
        auto tmpl = (std::filesystem::temp_directory_path() / "test_XXXXXX").string();
        // mkstemp mutates the template buffer in place to produce the unique name.
        const int fd = mkstemp(tmpl.data());
        if (fd == -1) {
            throw std::runtime_error("mkstemp failed");
        }
        close(fd);
        path = tmpl;
        std::ofstream out(path);
        out << content;
    }

    ~TempFile() noexcept {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&&) = delete;
    TempFile& operator=(TempFile&&) = delete;
};
