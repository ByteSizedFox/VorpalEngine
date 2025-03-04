rm *.spv
glslc shader.vert -o vert.spv
glslc shader.frag -o frag.spv

glslc 2d.vert -o vert2D.spv
glslc 2d.frag -o frag2D.spv

rm -rf ../../out/build/clang/assets*
