require 'rcad'

pedetal = add do
  box = box(10.cm, 10.cm, 3.cm)
    .align(:xcenter, :ycenter, I)


  base = ~box

  c = ~cylinder(d: 7.cm, h: 8.cm)
    .align(:bottom, base.top)

  ~box.align(:bottom, c.top)
end


~pedestal.rot_z(45.deg_to_rad)
