#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <StlAPI_Writer.hxx>
#include <Standard_Failure.hxx>
#include <rice/Class.hpp>
#include <rice/Exception.hpp>

using namespace Rice;


Data_Type<Standard_Failure> rb_cOCEError;

void translate_oce_exception(const Standard_Failure &e)
{
    //Data_Object<Standard_Failure> e_obj(
    //    new Standard_Failure(e), rb_cOCEError);
    throw Exception(rb_cOCEError, "%s", e.GetMessageString());
}


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


void combination_initialize(Object self, Object a, Object b)
{
    self.iv_set("@a", a);
    self.iv_set("@b", b);
}


Object union_render(Object self)
{
    Data_Object<TopoDS_Shape> shape_a = self.iv_get("@a").call("render");
    Data_Object<TopoDS_Shape> shape_b = self.iv_get("@b").call("render");
    return to_ruby(
        BRepAlgoAPI_Fuse(*shape_a, *shape_b).Shape());
}


Object difference_render(Object self)
{
    Data_Object<TopoDS_Shape> shape_a = self.iv_get("@a").call("render");
    Data_Object<TopoDS_Shape> shape_b = self.iv_get("@b").call("render");
    return to_ruby(
        BRepAlgoAPI_Cut(*shape_a, *shape_b).Shape());
}


Object intersection_render(Object self)
{
    Data_Object<TopoDS_Shape> shape_a = self.iv_get("@a").call("render");
    Data_Object<TopoDS_Shape> shape_b = self.iv_get("@b").call("render");
    return to_ruby(
        BRepAlgoAPI_Common(*shape_a, *shape_b).Shape());
}


extern "C"
void Init__yrcad()
{
    Data_Type<TopoDS_Shape> rb_cRenderedShape =
        define_class<TopoDS_Shape>("RenderedShape");

    Data_Type<Standard_Failure> rb_cOCEError =
        define_class("rb_cOCEError", rb_eRuntimeError);

    Class rb_cShape = define_class("Shape")
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("write_stl", &shape_write_stl);

    Class rb_cBox = define_class("Box", rb_cShape)
        .define_method("initialize", &box_initialize);

    Class rb_cCombination = define_class("Combination", rb_cShape)
        .define_method("initialize", &combination_initialize);

    Class rb_cUnion = define_class("Union", rb_cCombination)
        .define_method("render", &union_render);

    Class rb_cDifference = define_class("Difference", rb_cCombination)
        .define_method("render", &difference_render);

    Class rb_cIntersection = define_class("Intersection", rb_cCombination)
        .define_method("render", &intersection_render);
}
