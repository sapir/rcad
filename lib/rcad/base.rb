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
  return [r * Math.cos(a), r * Math.sin(a)]
end


module TransformableMixin
  # TODO: consider removing move_foo and scale_foo methods

  def move_x(delta)
    move(x: delta)
  end

  def move_y(delta)
    move(y: delta)
  end

  def move_z(delta)
    move(z: delta)
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
    scale(x: factor)
  end

  def scale_y(factor)
    scale(y: factor)
  end

  def scale_z(factor)
    scale(z: factor)
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


class Transform
  include TransformableMixin

  # TODO: low-level transform methods should be private

  def move(*args)
    _move(*Transform.magic_transform_params(args, 0))
  end

  def rotate(angle, axis)
    _rotate(angle, axis)
  end

  def scale(*args)
    if args.size == 1 and args[0].is_a? Numeric
      factor = args[0]
      _scale(factor, factor, factor)
    else
      _scale(*Transform.magic_transform_params(args, 1))
    end
  end

  def mirror(x, y, z)
    _mirror(x, y, z)
  end

  class << self
    private

    def self.magic_transform_params(args, default)
      if args.size == 1 and args[0].is_a? Array
        fail ArgumentError, "please pass coordinates separately, not in an array"
      elsif (args.size == 3 or args.size == 2) and args.all? { |n| n.is_a? Numeric }
        args.push(default) if args.size < 3
        args
      elsif args.size == 1 and args[0].is_a? Hash
        opts, = args
        [opts[:x] || default, opts[:y] || default, opts[:z] || default]
      else
        fail ArgumentError, "bad params for transform method"
      end
    end
  end
end


class Shape
  include TransformableMixin

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

  def transform(trsf)
    TransformedShape.new(self, trsf)
  end

  def move(*args)
    TransformedShape.new(self, I.move(*args))
  end

  def rotate(*args)
    TransformedShape.new(self, I.rotate(*args))
  end

  def scale(*args)
    TransformedShape.new(self, I.scale(*args))
  end

  def mirror(*args)
    TransformedShape.new(self, I.mirror(*args))
  end

  def bbox
    @bbox ||= _bbox
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

  def align(*align_pts)
    at = align_pts.pop

    if align_pts.empty?
      return self.transform(at)
    end

    combined = align_pts
      .map { |apt| (apt.is_a? Transform) ? apt : self.send(apt) }
      .reduce :*

    combined = (yield self) * combined if block_given?

    self.transform(at * combined.inverse)
  end

  class << self
    protected

    def magic_shape_params(args, *expected)
      opts = (args[-1].is_a? Hash) ? args.pop : {}
      defaults = (expected[-1].is_a? Hash) ? expected.pop : {}

      expected.map do |name|
        name_s = name.to_s
        is_dia = name_s.start_with?("d")

        if is_dia
          rname = ("r" + name_s[1..-1]).to_sym
          fail_msg = "please specify either #{name} or #{rname}"
        else
          fail_msg = "please specify #{name}"
        end


        if not args.empty?
          args.shift

        elsif opts.key?(name)
          opts[name]

        elsif is_dia and opts.key?(rname)
          opts[rname] * 2

        elsif defaults.key?(name)
          defaults[name]

        else
          fail ArgumentError, fail_msg
        end
      end
    end
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


class TransformedShape < Shape
  attr_reader :shape, :trsf

  def initialize(shape, trsf)
    @shape = shape
    @trsf = trsf
  end

  def to_s
    sprintf("%s*%s", @trsf, @shape)
  end

  def transform(trsf)
    TransformedShape.new(@shape, trsf * @trsf)
  end

  def move(*args)
    TransformedShape.new(@shape, @trsf.move(*args))
  end

  def rotate(*args)
    TransformedShape.new(@shape, @trsf.rotate(*args))
  end

  def scale(*args)
    TransformedShape.new(@shape, @trsf.scale(*args))
  end

  def mirror(*args)
    TransformedShape.new(@shape, @trsf.mirror(*args))
  end
end


def make_maker(name, klass)
  Object.send(:define_method, name, &klass.method(:new))
end
