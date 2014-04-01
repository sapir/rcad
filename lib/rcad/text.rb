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
    return Shape._new_face(wap.map { |w, p| w })
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
    wires_and_pts = []
    start_pt = nil
    cur_pt = nil

    path.each do |instr|
      points = instr.points.map { |p| [p.x, p.y] }

      if instr.move_to?
        cur_pt = points[0]
        start_pt = cur_pt   # move_to begins a new sub-path

      elsif instr.line_to?
        p = points[0]
        cur_wire_edges << Shape._new_line2D(cur_pt, p) unless cur_pt == nil
        cur_pt = p

      elsif instr.curve_to?
        cur_wire_edges << Shape._new_curve2D([cur_pt] + points)
        cur_pt = points[-1]

      elsif instr.close_path?
        if start_pt != nil and cur_pt != nil and start_pt != cur_pt
          cur_wire_edges << Shape._new_line2D(cur_pt, start_pt)
          cur_pt = start_pt
        end

        if not cur_wire_edges.empty?
          wire = Shape._new_wire(cur_wire_edges)
          wires_and_pts << [wire, start_pt]

          # get ready for next wire
          cur_wire_edges = []
        end

        start_pt = nil
      end
    end

    wires_and_pts
  end
end


make_maker :text, Text
