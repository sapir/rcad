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
end


def to_polar(r, a)
  return [r * Math::cos(a), r * Math::sin(a)]
end


TOLERANCE = 50.um


class Shape
  # if @shape isn't defined in a Shape's initialize() method, then render()
  # should be overridden to create it on-the-fly
  def render
    @shape
  end

  def add(shape)
    Union.new(self, shape)
  end

  def sub(shape)
    Difference.new(self, shape)
  end

  def mul(shape)
    Intersection.new(self, shape)
  end

  def move(x, y, z)
    Shape.new   # TODO
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

  def rotate(angle, axis)
    Shape.new   # TODO
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

  def scale(x, y, z)
    Shape.new   # TODO
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

  def revolve(angle=360)
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

  def write_stl(path)
    # TODO
  end
end


class Polygon < Shape
  def initialize(points, paths=nil)
    @shape = nil    # TODO
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
  def initialize(size)
    @shape = nil    # TODO
  end
end


class Circle < Shape
  def initialize(dia)
    @shape = nil    # TODO
  end
end


class Text < Shape
end


class Box < Shape
  def initialize(xsize, ysize, zsize)
    @shape = nil    # TODO
  end
end

class Cube < Box
  def initialize(size)
    super(size, size, size)
  end
end


class Cylinder < Shape
  def initialize(height, dia)
    @shape = nil    # TODO
  end
end


class Sphere < Shape
  def initialize(dia)
    @shape = nil    # TODO
  end
end


class Polyhedron < Shape
  def initialize(points, triangles)
    @shape = nil    # TODO
  end
end


class Torus < Shape
  def initialize(dia1, dia2)
    @shape = nil    # TODO
  end
end


class Union < Shape
  def initialize(a, b)
    @shape = nil    # TODO
  end
end


class Difference < Shape
  def initialize(a, b)
    @shape = nil    # TODO
  end
end


class Intersection < Shape
  def initialize(a, b)
    @shape = nil    # TODO
  end
end


class LinearExtrusion < Shape
  attr_reader :profile, :height

  def initialize(profile, height, twist=0)
    @profile = profile
    @height = height
    @twist = twist

    @shape = nil    # TODO
  end
end


class Revolution < Shape
  attr_reader :profile, :angle

  def initialize(profile, angle=360)
    @profile = profile
    @angle = angle

    @shape = nil    # TODO
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


def read_stl(path)
  nil   # TODO
end


polygon = Polygon.method(:new)
reg_poly = RegularPolygon.method(:new)
square = Square.method(:new)
circle = Circle.method(:new)
text = Text.method(:new)

box = Box.method(:new)
cube = Cube.method(:new)
cylinder = Cylinder.method(:new)
sphere = Sphere.method(:new)
polyhedron = Polyhedron.method(:new)
torus = Torus.method(:new)
reg_prism = RegularPrism.method(:new)
