#include "Engine/Engine.hpp"
#include "miniz.h"

mz_zip_archive zip;
std::vector<uint8_t> zipBuffer;
std::vector<char> fileData;

std::vector<char> Utils::readFileZip(const std::string& filename) {
    int fileIndex = mz_zip_reader_locate_file(const_cast<mz_zip_archive*>(&zip), filename.c_str(), nullptr, 0);
    if (fileIndex < 0) {
        printf("Error: File not found: %s\n", filename.c_str());
        return {};
    }

    // Extract file data upfront since miniz doesn't support streaming
    mz_zip_archive_file_stat stat;
    if (!mz_zip_reader_file_stat(const_cast<mz_zip_archive*>(&zip), fileIndex, &stat)) {
        printf("Error: Failed to get file stats for index %i\n", fileIndex);
        throw std::runtime_error("Failed to get file stats");
    }

    fileData.resize(stat.m_uncomp_size);
    
    if (!mz_zip_reader_extract_to_mem(const_cast<mz_zip_archive*>(&zip), fileIndex, fileData.data(), fileData.size(), 0)) {
        printf("Error: Failed to extract file data for index %i\n", fileIndex);
        throw std::runtime_error("Failed to extract file data");
    }
    
    return fileData;
}
bool Utils::fileExistsZip(const std::string& filename) {
    bool exists = mz_zip_reader_locate_file(const_cast<mz_zip_archive*>(&zip), filename.c_str(), nullptr, 0) >= 0;
    return exists;
}
void Utils::initIOSystem(const char * data, size_t size) {
    memset(&zip, 0, sizeof(zip));
    zipBuffer.assign(data, data + size);

    // Open the archive from memory
    if (!mz_zip_reader_init_mem(&zip, zipBuffer.data(), zipBuffer.size(), 0)) {
        mz_zip_error err_code = mz_zip_get_last_error(&zip);
        printf("Error: Failed to initialize zip archive. Error code: %d\n", (int)err_code);
        printf("Error description: %s\n", mz_zip_get_error_string(err_code));
        throw std::runtime_error("Failed to initialize zip archive");
    }
}