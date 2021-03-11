#!/bin/sh

rm -rf shaders
cp -r ../shaders ./shaders
cd shaders
for i in *.rgen *.rchit *.rmiss *.comp
do
  glslangValidator --target-env vulkan1.2 -V $i -o $i.spv
done
cd extra
for i in *.rchit
do
  glslangValidator --target-env vulkan1.2 -V $i -o $i.spv
done
cd ..
find . -type f ! -name '*.spv' -delete
find . -type f -name '.*' -delete
