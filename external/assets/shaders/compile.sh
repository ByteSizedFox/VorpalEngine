rm *.spv
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv

rm -rf ../../out/build/Release/assets*
