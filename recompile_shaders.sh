#!/bin/sh

rm -rf shaders
cp -r ../shaders ./shaders
cd shaders
parallel "zsh -c 'glslangValidator -V {} -o {}.spv'" ::: *.rgen *.rchit *.rmiss
find . -type f ! -name '*.spv' -delete
find . -type f -name '.*' -delete
