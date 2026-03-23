rm ../*.spv
glslc shader.vert -o ../vert.spv
glslc shader.frag -o ../frag.spv

glslc sky.vert -o ../sky.vert.spv
glslc sky.frag -o ../sky.frag.spv

glslc ui.frag -o ../ui.frag.spv
glslc shadow.vert -o ../shadow.vert.spv
glslc skinned.vert -o ../skinned.vert.spv
glslc shadow_skinned.vert -o ../shadow_skinned.vert.spv

rm -rf ../../../out/build/Release/assets*
rm -rf ../../../out/build/Debug/assets*
