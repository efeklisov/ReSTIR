#!/bin/sh

rm -rf shaders
cp -r ../shaders ./shaders
cd shaders
parallel "bash -c 'glslangValidator --target-env vulkan1.2 -V {} -o {}.spv'" ::: *.rgen *.rchit *.rmiss *.comp
cd extra
parallel "bash -c 'glslangValidator --target-env vulkan1.2 -V {} -o {}.spv'" ::: *.rchit
cd ..
find . -type f ! -name '*.spv' -delete
find . -type f -name '.*' -delete
