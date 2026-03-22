rm ../*.spv
glslc shader.vert -o ../vert.spv
glslc shader.frag -o ../frag.spv

glslc sky.vert -o ../sky.vert.spv
glslc sky.frag -o ../sky.frag.spv

glslc ui.frag -o ../ui.frag.spv
glslc shadow.vert -o ../shadow.vert.spv

rm -rf ../../../out/build/Release/assets*
rm -rf ../../../out/build/Debug/assets*
