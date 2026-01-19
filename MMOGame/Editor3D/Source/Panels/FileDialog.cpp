#include "FileDialog.h"

#include <nfd.hpp>
#include <algorithm>
#include <iostream>

namespace MMO {

void FileDialog::Init() {
    NFD::Init();
}

void FileDialog::Shutdown() {
    NFD::Quit();
}

std::string FileDialog::OpenFile(const std::initializer_list<FileDialogFilter>& filters) {
    NFD::UniquePath outPath;

    // NFD uses nfdfilteritem_t which has the same layout as FileDialogFilter
    nfdresult_t result = NFD::OpenDialog(
        outPath,
        reinterpret_cast<const nfdfilteritem_t*>(filters.begin()),
        static_cast<nfdfiltersize_t>(filters.size())
    );

    switch (result) {
        case NFD_OKAY: {
            std::string path = outPath.get();
            // Normalize to forward slashes
            std::replace(path.begin(), path.end(), '\\', '/');
            return path;
        }
        case NFD_CANCEL:
            return "";
        case NFD_ERROR:
            std::cerr << "NFD Error: " << NFD::GetError() << std::endl;
            return "";
    }

    return "";
}

std::string FileDialog::OpenFolder(const char* initialDir) {
    NFD::UniquePath outPath;

    nfdresult_t result = NFD::PickFolder(outPath, initialDir);

    switch (result) {
        case NFD_OKAY: {
            std::string path = outPath.get();
            std::replace(path.begin(), path.end(), '\\', '/');
            return path;
        }
        case NFD_CANCEL:
            return "";
        case NFD_ERROR:
            std::cerr << "NFD Error: " << NFD::GetError() << std::endl;
            return "";
    }

    return "";
}

std::string FileDialog::SaveFile(const std::initializer_list<FileDialogFilter>& filters,
                                  const char* defaultName) {
    NFD::UniquePath outPath;

    nfdresult_t result = NFD::SaveDialog(
        outPath,
        reinterpret_cast<const nfdfilteritem_t*>(filters.begin()),
        static_cast<nfdfiltersize_t>(filters.size()),
        nullptr,  // default path
        defaultName
    );

    switch (result) {
        case NFD_OKAY: {
            std::string path = outPath.get();
            std::replace(path.begin(), path.end(), '\\', '/');
            return path;
        }
        case NFD_CANCEL:
            return "";
        case NFD_ERROR:
            std::cerr << "NFD Error: " << NFD::GetError() << std::endl;
            return "";
    }

    return "";
}

} // namespace MMO
