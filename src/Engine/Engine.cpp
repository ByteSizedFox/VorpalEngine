#include "Engine/Engine.hpp"
#include "Engine/Loader.hpp"

std::vector<char> Utils::readFileZip(const std::string& filename) {
    // wip trying to load individual file
    MinizIOStream *stream = (MinizIOStream *) ioSystem->Open(filename.c_str(), "r");
    size_t fileSize = stream->FileSize();
    
    std::vector<char> buffer(fileSize);
    //char buffer[fileSize];
    //stream->Seek(0, aiOrigin_SET);
    stream->Read(buffer.data(), 1, fileSize);
    
    return buffer;
}
bool Utils::fileExistsZip(const std::string& filename) {
    return ioSystem->Exists(filename.c_str());
}
void Utils::initIOSystem(const char * data, size_t size) {
    ioSystem = new MinizIOSystem( reinterpret_cast<const uint8_t*>(data), size );
    importer.SetIOHandler(ioSystem);
}