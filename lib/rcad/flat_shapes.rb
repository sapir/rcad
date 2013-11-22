require 'rcad/_rcad'
require 'rcad/base'


class Polygon < Shape
  attr_reader :points, :paths

  def initialize(points, paths=nil)
    @points = points
    @paths = paths || [(0...points.size).to_a]
  end
end


class RegularPolygon < Polygon
  attr_reader :sides, :radius

  def initialize(*args)
    @sides, @radius = Shape.magic_shape_params(args, :sides, :r)

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

  def initialize(*args)
    @dia, = Shape.magic_shape_params(args, :d)
  end
end


class Text < Shape
    # TODO
end


make_maker :polygon, Polygon
make_maker :reg_poly, RegularPolygon
make_maker :rectangle, Rectangle
make_maker :square, Square
make_maker :circle, Circle
make_maker :text, Text
