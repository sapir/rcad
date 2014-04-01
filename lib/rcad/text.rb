require 'set'
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
    path = make_path
    wap = Text.path_to_wires_and_pts(path)
    return RenderedShape._new_compound(Text.group_wires_into_faces(wap))
  end

  def slant
    Cairo::FONT_SLANT_NORMAL
  end

  def weight
    Cairo::FONT_WEIGHT_NORMAL
  end


  private

  def make_path
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

  def Text.path_to_wires_and_pts(path)
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
        if cur_pt != nil
          cur_wire_edges << RenderedShape._new_line2D(cur_pt, p)
        end

        cur_pt = p

      elsif instr.curve_to?
        cur_wire_edges << RenderedShape._new_curve2D([cur_pt] + points)
        cur_pt = points[-1]

      elsif instr.close_path?
        if start_pt != nil and cur_pt != nil and start_pt != cur_pt
          cur_wire_edges << RenderedShape._new_line2D(cur_pt, start_pt)
          cur_pt = start_pt
        end

        if not cur_wire_edges.empty?
          wire = RenderedShape._new_wire(cur_wire_edges)
          wires_and_pts << [wire, start_pt]

          # get ready for next wire
          cur_wire_edges = []
        end

        start_pt = nil
      end
    end

    wires_and_pts
  end

  # wires must be non-intersecting (I expect cairo text paths to be fine)
  def Text.group_wires_into_faces(wires_and_pts)
    # for each wire, set of wires it contains
    graph_a_contains_bs = Hash[
      wires_and_pts.map do |w,_|
        f = RenderedShape._new_face([w])

        contained = wires_and_pts
          .select {|_,p| _is_pnt2D_in_face(p, f)}
          .map {|w,p| w}

        [w, Set.new(contained)]
      end]

    # build direct graph and set of root wires not contained in any other wre
    graph_a_directly_contains_bs = {}
    root_wires = Set.new(wires_and_pts.map {|w,_| w})

    graph_a_contains_bs.each_pair do |a,bs|
        direct_bs = bs.dup
        bs.each {|b| direct_bs.subtract(graph_a_contains_bs[b])}
        graph_a_directly_contains_bs[a] = direct_bs if !direct_bs.empty?

        root_wires.subtract(direct_bs)
      end

    root_wires.map do |root|
      RenderedShape._new_face(Text.oriented_wires(
        root, graph_a_directly_contains_bs))
    end
  end

  # gets all children of root, with wire orientation alternating between
  # parents and childs
  def Text.oriented_wires(root, graph, reverse=false)
    ret = [reverse ? root._reversed  : root]

    children = graph[root]
    if children
      children.each do |child_wire|
        ret.concat(Text.oriented_wires(child_wire, graph, !reverse))
      end
    end

    ret
  end
end


make_maker :text, Text
