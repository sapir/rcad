rcad
====

Solid CAD programming with Ruby

Example:

```ruby
require 'rcad'
require 'gears'

# overloaded ~ operator adds stuff to the shape
# you're working on
~sub do
  gear = ~SpurGear.new(1.cm, 4.8.cm)

  # make a hole for an M3 screw
  ~cylinder(d: 3.mm, h: gear.zsize)
end

# STL is written automatically on exit
```
