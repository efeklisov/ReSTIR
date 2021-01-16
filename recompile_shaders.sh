#!/bin/sh

rm -rf shaders
cp -r ../shaders ./shaders
cd shaders
parallel "zsh -c 'glslangValidator --target-env vulkan1.2 -V {} -o {}.spv'" ::: *.rgen *.rchit *.rmiss
find . -type f ! -name '*.spv' -delete
find . -type f -name '.*' -delete
