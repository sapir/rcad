require 'rcad/_rcad'
require 'rcad/base'


class Shape
  def extrude(height, twist=0)
    LinearExtrusion.new(self, height, twist)
  end

  def revolve(angle=nil)
    Revolution.new(self, angle)
  end
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

  def initialize(*args)
    # TODO: maybe make positional order be (:d0, [:dh], :h)
    @height, @bottom_dia, @top_dia = magic_shape_params(
      args, :h, :d0, :dh, dh: 0)
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

  def initialize(*args)
    @dia, @height = magic_shape_params(args, :d, :h)
  end

  def radius
    dia / 2.0
  end
end


class Sphere < Shape
  attr_accessor :dia

  def initialize(*args)
    @dia, = magic_shape_params(args, :d)
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

  def initialize(*args)
    @inner_dia, @outer_dia, @angle = magic_shape_params(
      args, :id, :od, :angle, angle: nil)
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

  def initialize(*args)
    @sides, @radius, height = Shape.magic_shape_params(args,
      :sides, :r, :h)

    poly = RegularPolygon.new(sides, radius)
    super(poly, height)
  end
end


make_maker :box, Box
make_maker :cube, Cube
make_maker :cone, Cone
make_maker :cylinder, Cylinder
make_maker :sphere, Sphere
make_maker :polyhedron, Polyhedron
make_maker :torus, Torus
make_maker :reg_prism, RegularPrism
