require 'rcad/version'
require 'rcad/_rcad'


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


# global tolerance value used by C++ extension when rendering shapes
$tol = 50.um


def to_polar(r, a)
  return [r * Math::cos(a), r * Math::sin(a)]
end


class Transform
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

  def mirror_x
    mirror(1, 0, 0)
  end

  def mirror_y
    mirror(0, 1, 0)
  end

  def mirror_z
    mirror(0, 0, 1)
  end
end


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

    p self
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

  def mirror_x
    mirror(1, 0, 0)
  end

  def mirror_y
    mirror(0, 1, 0)
  end

  def mirror_z
    mirror(0, 0, 1)
  end

  def extrude(height, twist=0)
    LinearExtrusion.new(self, height, twist)
  end

  def revolve(angle=360.deg_to_rad)
    Revolution.new(self, angle)
  end

  def bbox
    if @bbox == nil
      @bbox = _bbox
    end

    @bbox
  end

  def minx
    bbox[0][0]
  end

  def miny
    bbox[0][1]
  end

  def minz
    bbox[0][2]
  end

  def maxx
    bbox[1][0]
  end

  def maxy
    bbox[1][1]
  end

  def maxz
    bbox[1][2]
  end

  def xsize
    maxx - minx
  end

  def ysize
    maxy - miny
  end

  def zsize
    maxz - minz
  end

  def cx
    (minx + maxx) / 2.0
  end

  def cy
    (miny + maxy) / 2.0
  end

  def cz
    (minz + maxz) / 2.0
  end

  def left
    I.move_x(minx)
  end

  def right
    I.move_x(maxx)
  end

  def front
    I.move_y(miny)
  end

  def back
    I.move_y(maxy)
  end

  def top
    I.move_z(maxz)
  end

  def bottom
    I.move_z(minz)
  end

  def xcenter
    I.move_x(cx)
  end

  def ycenter
    I.move_y(cy)
  end

  def zcenter
    I.move_z(cz)
  end

  def center
    I.move(cx, cy, cz)
  end

  def align(a, b=nil)
    if b == nil
      a, b = I, a
    end

    if a.is_a? Symbol
      a = self.send(a)
    elsif a.is_a? Array
      a = a.map { |s| self.send(s) }.reduce :*
    end

    a = (yield self) * a if block_given?

    self.transform(b * a.inverse)
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
    printf("Rendering '%s'\n", output_file)
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


class Rectangle < Polygon
  # These override Shape.xsize etc.
  attr_reader :xsize, :ysize

  def initialize(xsize, ysize)
    @xsize = xsize
    @ysize = ysize

    super([[0,0], [xsize,0], [xsize,ysize], [0,ysize]])
  end
end


class Square < Rectangle
  attr_reader :size

  def initialize(size)
    @size = size

    super(size, size)
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
  # These override Shape.xsize etc.
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


class Cone < Shape
  attr_accessor :height, :bottom_dia, :top_dia

  def initialize(height, bottom_dia, top_dia=0)
    @height = height
    @bottom_dia = bottom_dia
    @top_dia = top_dia
  end

  def bottom_radius
    bottom_dia / 2.0
  end

  def top_radius
    top_dia / 2.0
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
make_maker :rectangle, Rectangle
make_maker :square, Square
make_maker :circle, Circle
make_maker :text, Text

make_maker :box, Box
make_maker :cube, Cube
make_maker :cone, Cone
make_maker :cylinder, Cylinder
make_maker :sphere, Sphere
make_maker :polyhedron, Polyhedron
make_maker :torus, Torus
make_maker :reg_prism, RegularPrism
