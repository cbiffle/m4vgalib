#!/usr/bin/ruby

puts "GIMP Palette"
puts "Name: PixelPusher64"
puts "Columns: 8"

(0..255).step(85) { |r|
  (0..255).step(85) { |g|
    (0..255).step(85) { |b|
      puts [r, g, b].join(' ')
    }
  }
}
