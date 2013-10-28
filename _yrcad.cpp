#include <BRepBuilderAPI_MakeShape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <rice/Class.hpp>

using namespace Rice;


static Data_Object<TopoDS_Shape> wrapShape(const TopoDS_Shape &shape)
{
    return Data_Object<TopoDS_Shape>(new TopoDS_Shape(shape));
}

static Data_Object<TopoDS_Shape> wrapShapeFromMaker(
    const BRepBuilderAPI_MakeShape &maker)
{
    return wrapShape(maker.Shape());
}


void box_initialize(Object self, double xsize, double ysize, double zsize)
{
    self.iv_set("@xsize", xsize);
    self.iv_set("@ysize", ysize);
    self.iv_set("@zsize", zsize);

    self.iv_set("@shape",
        wrapShapeFromMaker(BRepPrimAPI_MakeBox(xsize, ysize, zsize)));
}

void union_initialize(Object self, Object a, Object b)
{
    self.iv_set("@a", a);
    self.iv_set("@b", b);
}

extern "C"
void Init__yrcad()
{
    Class rb_cShape = define_class("Shape");

    Class rb_cBox = define_class("Box")
        .define_method("initialize", &box_initialize);

    Class rb_cUnion = define_class("Union")
        .define_method("initialize", &union_initialize);
}
