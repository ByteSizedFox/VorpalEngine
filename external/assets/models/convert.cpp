#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    // Check if an input file was provided
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_model_file>" << std::endl;
        return 1;
    }

    // Get the input file path
    std::string inputFile = argv[1];
    
    // Create output file path (same name but with .assbin extension)
    fs::path inputPath(inputFile);
    fs::path outputPath = inputPath.parent_path() / (inputPath.stem().string() + ".assbin");
    std::string outputFile = outputPath.string();

    std::cout << "Converting " << inputFile << " to " << outputFile << std::endl;

    // Create the importer
    Assimp::Importer importer;
    
    // Configure import settings
    // For faster loading at the expense of some accuracy:
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);

    // Only use essential post-processing steps
    unsigned int ppFlags =
        aiProcess_Triangulate |         // Ensure all faces are triangles
        aiProcess_JoinIdenticalVertices;// Merge duplicate vertices

    // Load the model
    std::cout << "Loading model file..." << std::endl;
    const aiScene* scene = importer.ReadFile(inputFile, ppFlags);
    
    // Check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error loading model: " << importer.GetErrorString() << std::endl;
        return 2;
    }
    
    std::cout << "Model loaded successfully. Converting to assbin format..." << std::endl;
    
    // Create the exporter
    Assimp::Exporter exporter;
    
    // Set binary optimization level (experimental)
    // 0 = no optimization, 1-4 = increasing levels
    //exporter.SetPropertyInteger("binaryOptimizerLevel", 3);
    
    // Export to assbin format
    aiReturn result = exporter.Export(scene, "assbin", outputFile);
    
    if (result != aiReturn_SUCCESS) {
        std::cerr << "Error exporting to assbin: " << exporter.GetErrorString() << std::endl;
        return 3;
    }
    
    std::cout << "Conversion successful! Saved to: " << outputFile << std::endl;
    
    // Optional: print basic scene info
    std::cout << "\nModel information:" << std::endl;
    std::cout << "- Meshes: " << scene->mNumMeshes << std::endl;
    std::cout << "- Materials: " << scene->mNumMaterials << std::endl;
    std::cout << "- Textures: " << scene->mNumTextures << std::endl;
    
    return 0;
}
