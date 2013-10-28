require_relative "yrcad"


base = box(10.cm, 10.cm, 3.cm)
    .move(-5.cm, -5.cm, 0)

pedestal = base
    .add(cylinder(8.cm, 7.cm).move_z(3.cm))
    .add(base.move_z(3.cm + 7.cm))

pedestal
  .rotate_z(45)
  .write_stl("pedestal.stl")
