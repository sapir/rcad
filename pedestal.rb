require 'rcad'


base = box(10.cm, 10.cm, 3.cm)
    .move(-5.cm, -5.cm, 0)

pedestal = add do
  ~base
  ~cylinder(8.cm, 7.cm).move_z(3.cm)
  ~base.move_z(3.cm + 7.cm)
end

~pedestal.rot_z(45)
