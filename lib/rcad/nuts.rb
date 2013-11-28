# calculations are (mostly) from OpenSCAD MCAD library

require 'rcad'


module NutSizesMixin
  module_function

  def nut_height
    0.8 * bolt_dia
  end

  def nut_dia_across_flats
    1.8 * bolt_dia
  end

  def nut_dia_across_corners
    nut_dia_across_flats / Math.cos(30.deg_to_rad)
  end
end


class HexNut < Shape
  include NutSizesMixin

  attr_accessor :bolt_dia, :xytol, :ztol, :full

  def initialize(bolt_dia, opts = {})
    @bolt_dia = bolt_dia
    @xytol = opts[:xytol] || 0
    @ztol = opts[:ztol] || 0
    @full = opts.key?(:full) ? opts[:full] : false
  end

  def render
    d = nut_dia_across_corners + @xytol
    h = nut_height + @ztol

    nut = reg_prism(6, d / 2.0, h)
    
    if full
      return nut
    else
      return sub do
          ~nut
          ~cylinder(h + 0.02, bolt_dia + @xytol)
            .move_z(-0.01)
        end
    end
  end
end


class Bolt < Shape
  include NutSizesMixin

  attr_accessor :bolt_dia, :bolt_len, :xytol, :ztol
  
  def initialize(bolt_dia, bolt_len, xytol=0, ztol=0)
    @bolt_dia = bolt_dia
    @bolt_len = bolt_len
    @xytol = xytol
    @ztol = ztol
  end

  def head_height
    0.7 * bolt_dia
  end

  def total_len
    bolt_len + head_height
  end

  def render
    d = nut_dia_across_corners + @xytol
    h = head_height + @ztol
    head = reg_prism(6, d / 2.0, h)

    add do
        ~head
        ~cylinder(total_len + @ztol, bolt_dia + @xytol)
          .move_z(-head_height)
      end
  end
end
