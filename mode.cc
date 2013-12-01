#include "vga/mode.h"

namespace vga {

Mode::~Mode() = default;

void Mode::deactivate() {}

void Mode::top_of_frame() {}

}  // namespace vga
