#include "vga/rasterizer.h"

namespace vga {

Rasterizer::~Rasterizer() = default;

void Rasterizer::activate(Timing const &) {}

void Rasterizer::deactivate() {}

}  // namespace vga
