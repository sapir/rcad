#include <gp_Pnt2d.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <TopoDS.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeEdge2d.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <StlAPI_Reader.hxx>
#include <StlAPI_Writer.hxx>
#include <Standard_Failure.hxx>
#include <rice/Class.hpp>
#include <rice/Exception.hpp>
#include <rice/Array.hpp>

using namespace Rice;


Data_Type<Standard_Failure> rb_cOCEError;
Class rb_cShape;


void translate_oce_exception(const Standard_Failure &e)
{
    //Data_Object<Standard_Failure> e_obj(
    //    new Standard_Failure(e), rb_cOCEError);
    throw Exception(rb_cOCEError, "%s", e.GetMessageString());
}


template<>
gp_Pnt2d from_ruby<gp_Pnt2d>(Object obj)
{
    Array ary(obj);

    if (ary.size() != 2) {
        throw Exception(rb_eArgError,
            "2D points must be arrays with 2 numbers each");
    }

    return gp_Pnt2d(
        from_ruby<Standard_Real>(ary[0]),
        from_ruby<Standard_Real>(ary[1]));
}

template<>
gp_Vec from_ruby<gp_Vec>(Object obj)
{
    Array ary(obj);

    if (ary.size() != 3) {
        throw Exception(rb_eArgError,
            "Vectors must be arrays with 3 numbers each");
    }

    return gp_Vec(
        from_ruby<Standard_Real>(ary[0]),
        from_ruby<Standard_Real>(ary[1]),
        from_ruby<Standard_Real>(ary[2]));
}

template<>
gp_Dir from_ruby<gp_Dir>(Object obj)
{
    return gp_Dir(from_ruby<gp_Vec>(obj));
}


static Data_Object<TopoDS_Shape> render_shape(Object shape)
{
    do {
        shape = shape.call("render");
    } while (shape.is_a(rb_cShape));
    
    if (shape.is_nil()) {
        throw Exception(rb_eArgError, "render returned nil");
    }

    return shape;
}

static Object wrap_rendered_shape(const TopoDS_Shape &shape)
{
    Object shape_obj = rb_cShape.call("new");
    shape_obj.iv_set("@shape", to_ruby(shape));
    return shape_obj;
}

static Object shape_transform(Object self, const gp_Trsf &transform)
{
    Data_Object<TopoDS_Shape> rendered = render_shape(self);
    return wrap_rendered_shape(
        BRepBuilderAPI_Transform(*rendered, transform, Standard_True).Shape());
}

Object shape_move(Object self, Standard_Real x, Standard_Real y,
    Standard_Real z)
{
    gp_Trsf transform;
    transform.SetTranslation(gp_Vec(x, y, z));
    return shape_transform(self, transform);
}

Object shape_rotate(Object self, Standard_Real angle, gp_Dir axis)
{
    gp_Trsf transform;
    transform.SetRotation(gp_Ax1(gp_Pnt(), axis), angle);
    return shape_transform(self, transform);
}

Object shape_scale(Object self, Standard_Real x, Standard_Real y,
    Standard_Real z)
{
    gp_GTrsf transform;
    transform.SetVectorialPart(
        gp_Mat(x, 0, 0,
               0, y, 0,
               0, 0, z));

    Data_Object<TopoDS_Shape> rendered = render_shape(self);
    return wrap_rendered_shape(
        BRepBuilderAPI_GTransform(*rendered, transform, Standard_True).Shape());
}

void shape_write_stl(Object self, String path)
{
    Data_Object<TopoDS_Shape> shape = render_shape(self);

    StlAPI_Writer writer;
    writer.ASCIIMode() = false;
    writer.RelativeMode() = false;
    writer.SetDeflection(0.05);     // TODO: deflection param
    writer.Write(*shape, path.c_str());
}


Object shape_from_stl(String path)
{
    TopoDS_Shape shape;
    StlAPI_Reader reader;
    reader.Read(shape, path.c_str());
    return wrap_rendered_shape(shape);
}


static TopoDS_Wire make_2d_wire_from_path(Array points, Array path)
{
    BRepBuilderAPI_MakeWire wire_maker;

    for (size_t i = 0; i < path.size(); ++i) {
        const size_t j = (i + 1) % path.size();

        const size_t p1_idx = from_ruby<size_t>(path[i]);
        const size_t p2_idx = from_ruby<size_t>(path[j]);

        gp_Pnt2d gp_p1(from_ruby<gp_Pnt2d>(points[p1_idx]));
        gp_Pnt2d gp_p2(from_ruby<gp_Pnt2d>(points[p2_idx]));

        wire_maker.Add(
            BRepBuilderAPI_MakeEdge2d(gp_p1, gp_p2).Edge());
    }

    return wire_maker.Wire();
}

Object polygon_render(Object self)
{
    const Array points = self.iv_get("@points");
    const Array paths = self.iv_get("@paths");

    if (paths.size() == 0) {
        throw Exception(rb_eArgError,
            "Polygon must have at least 1 path!");
    }

    BRepBuilderAPI_MakeFace face_maker(
        make_2d_wire_from_path(points, paths[0]));
    for (size_t i = 1; i < paths.size(); ++i) {
        TopoDS_Wire wire = make_2d_wire_from_path(points, paths[i]);

        // all paths except the first are inner loops,
        // so they should be reversed
        face_maker.Add(TopoDS::Wire(wire.Oriented(TopAbs_REVERSED)));
    }

    return wrap_rendered_shape(face_maker.Shape());
}


Object box_render(Object self)
{
    Standard_Real xsize = from_ruby<Standard_Real>(self.iv_get("@xsize"));
    Standard_Real ysize = from_ruby<Standard_Real>(self.iv_get("@zsize"));
    Standard_Real zsize = from_ruby<Standard_Real>(self.iv_get("@ysize"));

    return to_ruby(BRepPrimAPI_MakeBox(xsize, ysize, zsize).Shape());
}


Object cylinder_render(Object self)
{
    Standard_Real height = from_ruby<Standard_Real>(self.iv_get("@height"));
    Standard_Real dia = from_ruby<Standard_Real>(self.iv_get("@dia"));
    return to_ruby(BRepPrimAPI_MakeCylinder(dia / 2.0, height).Shape());
}


Object sphere_render(Object self)
{
    Standard_Real dia = from_ruby<Standard_Real>(self.iv_get("@dia"));
    return to_ruby(BRepPrimAPI_MakeSphere(dia / 2.0).Shape());
}


Object torus_render(Object self)
{
    Standard_Real inner_dia = from_ruby<Standard_Real>(
        self.iv_get("@inner_dia"));
    
    Standard_Real outer_dia = from_ruby<Standard_Real>(
        self.iv_get("@outer_dia"));

    Standard_Real r1 = inner_dia / 2.0;
    Standard_Real r2 = outer_dia / 2.0;

    Object angle = self.iv_get("@angle");
    if (angle.is_nil()) {
        return to_ruby(BRepPrimAPI_MakeTorus(r1, r2).Shape());
    } else {
        Standard_Real angle_num = from_ruby<Standard_Real>(angle);
        return to_ruby(BRepPrimAPI_MakeTorus(r1, r2, angle_num).Shape());
    }
}


void combination_initialize(Object self, Object a, Object b)
{
    self.iv_set("@a", a);
    self.iv_set("@b", b);
}


Object union_render(Object self)
{
    Data_Object<TopoDS_Shape> shape_a = render_shape(self.iv_get("@a"));
    Data_Object<TopoDS_Shape> shape_b = render_shape(self.iv_get("@b"));
    return to_ruby(
        BRepAlgoAPI_Fuse(*shape_a, *shape_b).Shape());
}


Object difference_render(Object self)
{
    Data_Object<TopoDS_Shape> shape_a = render_shape(self.iv_get("@a"));
    Data_Object<TopoDS_Shape> shape_b = render_shape(self.iv_get("@b"));
    return to_ruby(
        BRepAlgoAPI_Cut(*shape_a, *shape_b).Shape());
}


Object intersection_render(Object self)
{
    Data_Object<TopoDS_Shape> shape_a = render_shape(self.iv_get("@a"));
    Data_Object<TopoDS_Shape> shape_b = render_shape(self.iv_get("@b"));
    return to_ruby(
        BRepAlgoAPI_Common(*shape_a, *shape_b).Shape());
}


// initialize is defined in Ruby code
Object linear_extrusion_render(Object self)
{
    Object profile = self.iv_get("@profile");
    Standard_Real height = from_ruby<Standard_Real>(self.iv_get("@height"));
    Standard_Real twist = from_ruby<Standard_Real>(self.iv_get("@twist"));

    Data_Object<TopoDS_Shape> shape = render_shape(profile);

    if (0 == twist) {
        return to_ruby(
            BRepPrimAPI_MakePrism(*shape, gp_Vec(0, 0, height),
                Standard_True).Shape());
    } else {
        // TODO
    }

    return Object();
}


// initialize is defined in Ruby code
Object revolution_render(Object self)
{
    Object profile = self.iv_get("@profile");
    Data_Object<TopoDS_Shape> shape = render_shape(profile);
    
    Object angle = self.iv_get("@angle");
    if (angle.is_nil()) {
        return to_ruby(
            BRepPrimAPI_MakeRevol(*shape, gp::OY(), Standard_True).Shape());
    } else {
        Standard_Real angle_num = from_ruby<Standard_Real>(angle);
        return to_ruby(
            BRepPrimAPI_MakeRevol(*shape, gp::OY(), angle_num,
                Standard_True).Shape());
    }
}


extern "C"
void Init__rcad()
{
    Data_Type<TopoDS_Shape> rb_cRenderedShape =
        define_class<TopoDS_Shape>("RenderedShape");

    Data_Type<Standard_Failure> rb_cOCEError =
        define_class("OCEError", rb_eRuntimeError);

    rb_cShape = define_class("Shape")
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("move", &shape_move)
        .define_method("rotate", &shape_rotate)
        .define_method("scale", &shape_scale)
        .define_method("write_stl", &shape_write_stl)
        .define_singleton_method("from_stl", &shape_from_stl);

    Class rb_cPolygon = define_class("Polygon", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &polygon_render);

    Class rb_cBox = define_class("Box", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &box_render);

    Class rb_cCylinder = define_class("Cylinder", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &cylinder_render);

    Class rb_cSphere = define_class("Sphere", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &sphere_render);

    Class rb_cTorus = define_class("Torus", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &torus_render);

    Class rb_cCombination = define_class("Combination", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("initialize", &combination_initialize);

    Class rb_cUnion = define_class("Union", rb_cCombination)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &union_render);

    Class rb_cDifference = define_class("Difference", rb_cCombination)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &difference_render);

    Class rb_cIntersection = define_class("Intersection", rb_cCombination)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &intersection_render);

    Class rb_cLinearExtrusion = define_class("LinearExtrusion", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &linear_extrusion_render);

    Class rb_cRevolution = define_class("Revolution", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &revolution_render);
}
