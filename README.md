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
  ~SpurGear.new(1.cm, 4.8.cm)

  # hole for an M3 screw
  ~cylinder(3.mm)
end

# STL is written automatically on exit
```
