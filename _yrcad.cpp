#include <BRepPrimAPI_MakeBox.hxx>
#include <StlAPI_Writer.hxx>
#include <rice/Class.hpp>

using namespace Rice;


void shape_write_stl(Object self, String path)
{
    Data_Object<TopoDS_Shape> shape = self.call("render");

    StlAPI_Writer writer;
    writer.ASCIIMode() = false;
    writer.RelativeMode() = false;
    writer.SetDeflection(0.05);     // TODO: deflection param
    writer.Write(*shape, path.c_str());
}


void box_initialize(Object self, double xsize, double ysize, double zsize)
{
    self.iv_set("@xsize", xsize);
    self.iv_set("@ysize", ysize);
    self.iv_set("@zsize", zsize);

    self.iv_set("@shape",
        BRepPrimAPI_MakeBox(xsize, ysize, zsize).Shape());
}


void union_initialize(Object self, Object a, Object b)
{
    self.iv_set("@a", a);
    self.iv_set("@b", b);
}


extern "C"
void Init__yrcad()
{
    Data_Type<TopoDS_Shape> rb_cRenderedShape =
        define_class<TopoDS_Shape>("RenderedShape");

    Class rb_cShape = define_class("Shape")
        .define_method("write_stl", &shape_write_stl);

    Class rb_cBox = define_class("Box", rb_cShape)
        .define_method("initialize", &box_initialize);

    Class rb_cUnion = define_class("Union", rb_cShape)
        .define_method("initialize", &union_initialize);
}
