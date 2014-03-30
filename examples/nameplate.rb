require 'rcad'

base_thickness = 4.mm
text_thickness = 4.mm
margin = 2.5.mm

t2d = text("Joe", font_name: "Times New Roman", font_size: 18)

base = ~box(t2d.xsize + margin * 2, t2d.ysize + margin * 2, base_thickness)

~t2d
    .extrude(text_thickness)
    .align(:xcenter, :ycenter, :bottom, base.xcenter * base.ycenter * base.top)
