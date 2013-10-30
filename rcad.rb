require_relative '_rcad'


class Numeric
  def mm
    self
  end

  def cm
    self * 10.0
  end

  def um
    self / 1000.0
  end

  def in
    self * 25.4
  end

  def deg_to_rad
    self * Math::PI / 180
  end

  def rad_to_deg
    self * 180 / Math::PI
  end
end


def to_polar(r, a)
  return [r * Math::cos(a), r * Math::sin(a)]
end


TOLERANCE = 50.um


class Shape
  # if @shape isn't defined in a Shape's initialize() method, then render()
  # should be overridden to create and return it on-the-fly.
  def render
    @shape
  end

  def +(right)
    Union.new(self, right)
  end

  def -(right)
    Difference.new(self, right)
  end

  def *(right)
    Intersection.new(self, right)
  end

  def ~@
    if $shape_mode == :hull
      $shape << self
    else
      $shape = ($shape == nil) ? self : $shape.send($shape_mode, self)
    end

    self
  end

  def move_x(delta)
    move(delta, 0, 0)
  end

  def move_y(delta)
    move(0, delta, 0)
  end

  def move_z(delta)
    move(0, 0, delta)
  end

  def rot_x(angle)
    rotate(angle, [1, 0, 0])
  end

  def rot_y(angle)
    rotate(angle, [0, 1, 0])
  end

  def rot_z(angle)
    rotate(angle, [0, 0, 1])
  end

  def scale_x(factor)
    scale(factor, 1, 1)
  end

  def scale_y(factor)
    scale(1, factor, 1)
  end

  def scale_z(factor)
    scale(1, 1, factor)
  end

  def extrude(height, twist=0)
    LinearExtrusion.new(self, height, twist)
  end

  def revolve(angle=360.deg_to_rad)
    Revolution.new(self, angle)
  end

  def bbox
    # TODO
  end

  def min_x
    bbox[0].x
  end

  def min_y
    bbox[0].y
  end

  def min_z
    bbox[0].z
  end

  def max_x
    bbox[1].x
  end

  def max_y
    bbox[1].y
  end

  def max_z
    bbox[1].z
  end

  def x_size
    max_x - min_x
  end

  def y_size
    max_y - min_y
  end

  def z_size
    max_z - min_z
  end
end

$shape_stack = []
$shape = nil
$shape_mode = :+

def _shape_mode_block(mode, &block)
  $shape_stack.push([$shape, $shape_mode])
  $shape = nil
  $shape_mode = mode

  block.call
  res = $shape

  $shape, $shape_mode = $shape_stack.pop
  res
end

def add(&block)
  _shape_mode_block(:+, &block)
end

def sub(&block)
  _shape_mode_block(:-, &block)
end

def mul(&block)
  _shape_mode_block(:*, &block)
end

def hull(&block)
  $shape_stack.push([$shape, $shape_mode])
  $shape = []
  $shape_mode = :hull

  block.call
  res = _hull($shape)

  $shape, $shape_mode = $shape_stack.pop
  res
end

def write_stl(*args)
  $shape != nil or raise
  $shape.write_stl(*args)
end

def clear_shape
  $shape = nil
end

at_exit do
  if $shape && ($! == nil)
    output_file = File.basename($0, ".*") + ".stl"
    printf("Writing '%s'\n", output_file)
    write_stl(output_file)
  end
end


class Polygon
  attr_reader :points, :paths

  def initialize(points, paths=nil)
    @points = points
    @paths = paths || [(0...points.size).to_a]
  end
end


class RegularPolygon < Polygon
  attr_reader :sides, :radius

  def initialize(sides, radius)
    @sides = sides
    @radius = radius

    angles = (1..sides).map { |i| i * 2 * Math::PI / sides }
    points = angles.map { |a| to_polar(radius, a) }
    super(points)
  end
end


class Square < Polygon
  attr_reader :size

  def initialize(size)
    @size = size

    super([[0,0], [size,0], [size,size], [0,size]])
  end
end


class Circle < Shape
  attr_accessor :dia

  def initialize(dia)
    @dia = dia
  end
end


class Text < Shape
    # TODO
end


class Box < Shape
  attr_accessor :xsize, :ysize, :zsize

  def initialize(xsize, ysize, zsize)
    @xsize = xsize
    @ysize = ysize
    @zsize = zsize
  end
end

class Cube < Box
  def initialize(size)
    super(size, size, size)
  end
end


class Cylinder < Shape
  attr_accessor :height, :dia

  def initialize(height, dia)
    @height = height
    @dia = dia
  end

  def radius
    dia / 2.0
  end
end


class Sphere < Shape
  attr_accessor :dia

  def initialize(dia)
    @dia = dia
  end

  def radius
    dia / 2.0
  end
end


class Polyhedron < Shape
  attr_accessor :points, :faces

  def initialize(points, faces)
    @points = points
    @faces = faces
  end
end


class Torus < Shape
  attr_accessor :inner_dia, :outer_dia, :angle

  def initialize(inner_dia, outer_dia, angle=nil)
    @inner_dia = inner_dia
    @outer_dia = outer_dia
    @angle = angle
  end

  def inner_radius
    inner_dia / 2.0
  end

  def outer_radius
    outer_dia / 2.0
  end
end


class LinearExtrusion < Shape
  attr_reader :profile, :height

  def initialize(profile, height, twist=0)
    @profile = profile
    @height = height
    @twist = twist
  end
end


class Revolution < Shape
  attr_reader :profile, :angle

  def initialize(profile, angle=nil)
    @profile = profile
    @angle = angle
  end
end


class RegularPrism < LinearExtrusion
  attr_reader :sides, :radius

  def initialize(sides, radius, height)
    @sides = sides
    @radius = radius

    poly = RegularPolygon.new(sides, radius)
    super(poly, height)
  end
end


def make_maker(name, klass)
  Object.send(:define_method, name, &klass.method(:new))
end

make_maker :polygon, Polygon
make_maker :reg_poly, RegularPolygon
make_maker :square, Square
make_maker :circle, Circle
make_maker :text, Text

make_maker :box, Box
make_maker :cube, Cube
make_maker :cylinder, Cylinder
make_maker :sphere, Sphere
make_maker :polyhedron, Polyhedron
make_maker :torus, Torus
make_maker :reg_prism, RegularPrism
