require 'rcad/_rcad'
require 'rcad/base'
require 'cairo'


class Text < Shape
  attr_reader :text, :font_name, :font_size

  def initialize(text, opts={})
    @text = text
    @font_name = opts.key?(:font_name) ? opts[:font_name] : "Arial";
    @font_size = opts.key?(:font_size) ? opts[:font_size] : 12;
  end

  def render
    path = _make_path
    wap = Text._path_to_wires_and_pts(path)
    # wap.each { |w,p| puts w.to_s }
    # return cairoPathToOccShape(path)
    puts "yum"
  end

  def slant
    Cairo::FONT_SLANT_NORMAL
  end

  def weight
    Cairo::FONT_WEIGHT_NORMAL
  end

  def _make_path
    # actually, we want an in-memory surface, but it seems ruby-cairo
    # doesn't support that right now
    surf = Cairo::SVGSurface.new("tmp.svg", 1024, 1024)
    
    ctx = Cairo::Context.new(surf)
    ctx.select_font_face(font_name, slant, weight)
    ctx.set_font_size(font_size)

    ctx.new_path()
    ctx.text_path(text)
    # invert y direction, to match coordinates used for 3D
    ctx.scale(1, -1)

    ctx.copy_path()
  end

  def Text._path_to_wires_and_pts(path)
    cur_wire_edges = []
    wires = []
    start_pt = nil
    cur_pt = nil

    path.each do |instr|
      if instr.move_to?
        cur_pt = instr.points[0]
        start_pt = cur_pt   # move_to begins a new sub-path

      elsif instr.line_to?
        p = instr.points[0]
        cur_wire_edges << [cur_pt, p] unless cur_pt == nil
        cur_pt = p

      elsif instr.curve_to?
        p1, p2, p3 = instr.points
        curve = [cur_pt, p1, p2, p3]
        cur_wire_edges << curve
        cur_pt = p3

      elsif instr.close_path?
        if start_pt != nil and cur_pt != nil and start_pt != cur_pt
          cur_wire_edges << [cur_pt, start_pt]
          cur_pt = start_pt
        end

        if not cur_wire_edges.empty?
          wires << [cur_wire_edges, start_pt]

          # get ready for next wire
          cur_wire_edges = []
        end

        start_pt = nil
      end
    end

    wires
  end
end


make_maker :text, Text
