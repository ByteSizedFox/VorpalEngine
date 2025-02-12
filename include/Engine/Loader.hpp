#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h> // Output data structure
#include "miniz.h"
#include <string>
#include <memory>
#include <vector>

// Forward declarations
class MinizIOSystem;

// Custom IOStream implementation for reading from zip archives
class MinizIOStream : public Assimp::IOStream {
    friend class MinizIOSystem;

protected:
    const mz_zip_archive* zip;
    size_t fileIndex;
    std::vector<uint8_t> fileData;
    size_t currentPos;

    MinizIOStream(const mz_zip_archive* zipArchive, size_t index) : zip(zipArchive), fileIndex(index), currentPos(0) {
        // Extract file data upfront since miniz doesn't support streaming
        mz_zip_archive_file_stat stat;
        mz_zip_reader_file_stat(const_cast<mz_zip_archive*>(zip), fileIndex, &stat);
        fileData.resize(stat.m_uncomp_size);
        mz_zip_reader_extract_to_mem(const_cast<mz_zip_archive*>(zip), fileIndex, fileData.data(), fileData.size(), 0);
    }

public:
    ~MinizIOStream() {
        // Cleanup is handled by MinizIOSystem
    }

    size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
        size_t bytesToRead = pSize * pCount;
        size_t bytesAvailable = fileData.size() - currentPos;
        size_t bytesToActuallyRead = std::min(bytesToRead, bytesAvailable);

        if (bytesToActuallyRead == 0) return 0;

        memcpy(pvBuffer, fileData.data() + currentPos, bytesToActuallyRead);
        currentPos += bytesToActuallyRead;
        return bytesToActuallyRead / pSize;
    }

    size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override {
        return 0; // Write operations not supported
    }

    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
        size_t newPos = 0;
        switch (pOrigin) {
            case aiOrigin_SET: newPos = pOffset; break;
            case aiOrigin_CUR: newPos = currentPos + pOffset; break;
            case aiOrigin_END: newPos = fileData.size() + pOffset; break;
            default: return aiReturn_FAILURE;
        }

        if (newPos > fileData.size()) {
            return aiReturn_FAILURE;
        }

        currentPos = newPos;
        return aiReturn_SUCCESS;
    }

    size_t Tell() const override {
        return currentPos;
    }

    size_t FileSize() const override {
        return fileData.size();
    }

    void Flush() override {}
};

// Custom IOSystem implementation for zip archives using miniz
class MinizIOSystem : public Assimp::IOSystem {
private:
    mz_zip_archive zip;
    std::vector<uint8_t> zipBuffer;

    // Helper function to locate file index
    int locateFile(const char* pFile) const {
        // Need to const_cast because miniz API isn't const-correct
        return mz_zip_reader_locate_file(const_cast<mz_zip_archive*>(&zip), 
                                       pFile, 
                                       nullptr, 
                                       0);
    }

public:
    MinizIOSystem(const uint8_t* buffer, size_t bufferSize) {
        // Initialize miniz structure
        memset(&zip, 0, sizeof(zip));
        
        // Copy buffer to ensure it stays valid
        zipBuffer.assign(buffer, buffer + bufferSize);

        // Open the archive from memory
        if (!mz_zip_reader_init_mem(&zip, zipBuffer.data(), zipBuffer.size(), 0)) {
            throw std::runtime_error("Failed to initialize zip archive");
        }
    }

    ~MinizIOSystem() {
        mz_zip_reader_end(&zip);
    }

    bool Exists(const char* pFile) const override {
        return locateFile(pFile) >= 0;
    }

    char getOsSeparator() const override {
        return '/';
    }

    Assimp::IOStream* Open(const char* pFile, const char* pMode) override {
        if (strchr(pMode, 'w') != nullptr) {
            return nullptr;
        }

        int fileIndex = locateFile(pFile);
        if (fileIndex < 0) {
            return nullptr;
        }

        return new MinizIOStream(&zip, fileIndex);
    }

    void Close(Assimp::IOStream* pFile) override {
        delete pFile;
    }
};