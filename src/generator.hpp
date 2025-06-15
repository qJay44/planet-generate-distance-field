#pragma once

#include "utils/types.hpp"

namespace generator {

void genMask(const fspath& path0, const fspath& path1);
void genDistanceField(const fspath& path0, const fspath& path1, const GLenum& internalFormat);

} // namespace generator
