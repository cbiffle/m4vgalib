#ifndef VGA_VGA_H
#define VGA_VGA_H

#include <cstdint>

namespace vga {

// Forward declarations of types used below.
struct Timing;      // see: timing.h
class Rasterizer;   // see: rasterizer.h


/*******************************************************************************
 * Types
 */

/*
 * Common shorthand for a pixel, which we store in an 8-bit byte.
 */
using Pixel = std::uint8_t;

/*
 * Description of a group of scanlines handled by a particular Rasterizer.
 * Bands make up a singly-linked list that describes the whole screen.
 */
struct Band {
  Rasterizer *rasterizer;   // Rasterizer that handles this band.
  unsigned line_count;      // Number of scanlines included.
  Band const *next;         // Where to go from here.
};


/*******************************************************************************
 * Public functions
 *
 * Note: in general, you want to call the functions below in this sequence:
 *
 * vga::init
 * vga::configure_band_list
 * vga::configure_timing
 * vga::video_on
 */

/*
 * Sets up state used by the driver.  Must be called before any functions below.
 */
void init();

/*
 * Provides the driver with a singly-linked list of Bands describing how to
 * produce pixels for an entire screen.  Once provided, this list will be reused
 * until replaced or cleared.
 *
 * It's safe to alter the Bands during the vertical blanking interval.  Changes
 * made during active video may or may not be reflected.
 *
 * Remember to take the Band pointer back (typically using clear_band_list)
 * before deallocating the Band or Rasterizers!
 */
void configure_band_list(Band const *head);

/*
 * Switches the driver band list for an empty list and synchronizes with the
 * driver to ensure that the change has been made.  This is useful to disconnect
 * a rasterizer from the driver before destruction.
 */
void clear_band_list();

/*
 * Configures vertical and horizontal timing according to the parameters
 * contained in the given Timing struct.  Note that this will also change the
 * CPU frequency.
 *
 * The driver makes a copy of the Timing struct for its own reference, so it's
 * safe to deallocate or rewrite it after configure_timing returns -- with the
 * understanding that such changes will not be reflected.
 *
 * This is safe to do repeatedly, even after video has started, but will
 * (necessarily) introduce a glitch in the output.  To minimize the external
 * appearance of the glitch, configure_timing calls video_off and sync_off
 * before beginning.
 *
 * configure_timing leaves sync on at return.
 */
void configure_timing(Timing const &);

/*
 * Idles the CPU until the driver is in vertical blank.  If called *during*
 * vertical blank, returns immediately.
 *
 * To synchronize to the *start* of vblank, e.g. to do some expensive
 * operation, use sync_to_vblank instead.
 */
void wait_for_vblank();

/*
 * Idles the CPU until the driver reaches the first line of vertical blank.  If
 * called *during* vertical blank, this will wait out the current blank and
 * resume at the next.
 *
 * To synchronize to *any* part of vblank, e.g. to avoid tearing in a simple
 * animation loop, use wait_for_vblank instead.
 */
void sync_to_vblank();

/*
 * Simply checks whether the driver is in vertical blank.
 */
bool in_vblank();

/*
 * Switches on the parallel output leading to the video DAC.  It's best to do
 * this during vertical blank, once you're ready to produce a frame.
 */
void video_on();

/*
 * Switches off the parallel output leading to the video DAC, driving the lines
 * high-impedance.  This effectively blanks the display and can be used to hide
 * mode changes.
 */
void video_off();

/*
 * Switches off sync signal generation.  This is exposed in case you need it,
 * but it's rarely useful.
 */
void sync_off();

/*
 * Switches on sync signal generation.  Use this to reverse the effect of
 * sync_off.  Under normal operation, configure_timer turns sync on for you.
 */
void sync_on();

}  // namespace vga

/*
 * Applications can implement this function to receive a callback during hblank.
 */
extern void vga_hblank_interrupt();

#endif  // VGA_VGA_H
