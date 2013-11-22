#include <sstream>
#include <gp_Pnt2d.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <gp_Circ.hxx>
#include <TColgp_Array2OfPnt.hxx>
#include <Poly_Triangulation.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_Circle.hxx>
#include <GCE2d_MakeSegment.hxx>
#include <TopoDS.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepOffsetAPI_MakePipeShell.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <BRepTopAdaptor_FClass2d.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepBndLib.hxx>
#include <StlAPI_Reader.hxx>
#include <StlAPI_Writer.hxx>
#include <Standard_Failure.hxx>
#include <rice/Class.hpp>
#include <rice/Exception.hpp>
#include <rice/Array.hpp>
#include <rice/global_function.hpp>

extern "C" {
#include <qhull/qhull_a.h>
}


using namespace Rice;


Data_Type<Standard_Failure> rb_cOCEError;
Data_Type<TopoDS_Shape> rb_cRenderedShape;
Class rb_cShape;


// for debugging
std::ostream &operator <<(std::ostream &lhs, gp_Pnt rhs)
{
    return lhs << "(" << rhs.X() << "," << rhs.Y() << "," << rhs.Z() << ")";
}

std::ostream &operator <<(std::ostream &lhs, gp_Vec rhs)
{
    return lhs << "(" << rhs.X() << "," << rhs.Y() << "," << rhs.Z() << ")";
}


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
gp_XYZ from_ruby<gp_XYZ>(Object obj)
{
    Array ary(obj);

    if (ary.size() == 2) {
        return gp_XYZ(
            from_ruby<Standard_Real>(ary[0]),
            from_ruby<Standard_Real>(ary[1]),
            0);
    } else if (ary.size() == 3) {
        return gp_XYZ(
            from_ruby<Standard_Real>(ary[0]),
            from_ruby<Standard_Real>(ary[1]),
            from_ruby<Standard_Real>(ary[2]));
    } else {
        throw Exception(rb_eArgError,
            "3D points must be arrays with 2 or 3 numbers each");
    }
}

template<>
gp_Pnt from_ruby<gp_Pnt>(Object obj)
{
    return gp_Pnt(from_ruby<gp_XYZ>(obj));
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

template<>
Object to_ruby<gp_Mat>(const gp_Mat &mat)
{
    Array mat_ary;

    for (int i = 1; i <= 3; ++i) {
        Array row_ary;
        for (int j = 1; j <= 3; ++j) {
            row_ary.push(mat(i, j));
        }

        mat_ary.push(row_ary);
    }

    return mat_ary;
}

template<>
Object to_ruby<gp_XYZ>(const gp_XYZ &xyz)
{
    Array ary;
    ary.push(xyz.X());
    ary.push(xyz.Y());
    ary.push(xyz.Z());
    return ary;
}


static Standard_Real get_tolerance()
{
    return from_ruby<Standard_Real>(
        Object(rb_gv_get("$tol")));
}


static String transform_to_s(gp_GTrsf self)
{
    gp_Mat vec = self.VectorialPart();
    gp_XYZ trn = self.TranslationPart();

    std::stringstream s;
    s << "#<Transform:";

    for (int i = 1; i <= 3; ++i) {
        for (int j = 1; j <= 3; ++j) {
            s << vec(i, j)
                << ((j < 3) ? ", " : "; ");
        }
    }

    s << "x=" << trn.X()
        << ", y=" << trn.Y()
        << ", z=" << trn.Z();

    s << ">";
    return s.str();
}

static gp_Mat transform_mat_part(gp_GTrsf self)
{
    return self.VectorialPart();
}

static void transform_mat_part_set(gp_GTrsf &self, gp_Mat mat)
{
    self.SetVectorialPart(mat);
}

static gp_XYZ transform_ofs_part(gp_GTrsf self)
{
    return self.TranslationPart();
}

static void transform_ofs_part_set(gp_GTrsf &self, gp_XYZ ofs)
{
    self.SetTranslationPart(ofs);
}

static gp_GTrsf transform_multiply(gp_GTrsf self, gp_GTrsf right)
{
    gp_GTrsf gtrsf = self;
    gtrsf.Multiply(right);
    return gtrsf;
}

static gp_GTrsf transform__move(gp_GTrsf self, Standard_Real x, Standard_Real y,
    Standard_Real z)
{
    gp_GTrsf gtrsf;
    gtrsf.SetTranslationPart(gp_XYZ(x, y, z));
    gtrsf.Multiply(self);       // gtrsf *= self
    return gtrsf;
}

static gp_GTrsf transform__rotate(gp_GTrsf self, Standard_Real angle,
    gp_Dir axis)
{
    gp_Trsf rot;
    rot.SetRotation(gp_Ax1(gp_Pnt(), axis), angle);

    gp_GTrsf gtrsf(rot);
    gtrsf.Multiply(self);       // gtrsf *= self
    return gtrsf;
}

static gp_GTrsf transform__scale(gp_GTrsf self,
    Standard_Real x, Standard_Real y, Standard_Real z)
{
    gp_GTrsf gtrsf;
    gtrsf.SetVectorialPart(
        gp_Mat(x, 0, 0,
               0, y, 0,
               0, 0, z));

    gtrsf.Multiply(self);       // gtrsf *= self
    return gtrsf;
}

static gp_GTrsf transform__mirror(gp_GTrsf self,
    Standard_Real x, Standard_Real y, Standard_Real z)
{
    gp_Ax2 mirror_plane(gp::Origin(), gp_Dir(x, y, z));

    gp_Trsf mirror;
    mirror.SetMirror(mirror_plane);

    gp_GTrsf gtrsf(mirror);
    gtrsf.Multiply(self);       // gtrsf *= self
    return gtrsf;
}

static gp_GTrsf transform_inverse(gp_GTrsf self)
{
    gp_GTrsf gtrsf = self;
    gtrsf.Invert();
    return gtrsf;
}


static Data_Object<TopoDS_Shape> render_shape(Object shape)
{
    if (shape.is_a(rb_cRenderedShape)) {
        return shape;
    }

    if (!shape.is_a(rb_cShape)) {
        String shape_str = shape.to_s();
        throw Exception(rb_eArgError,
            "attempt to render %s which is not a Shape",
            shape_str.c_str());
    }

    while (shape.is_a(rb_cShape)) {
        shape = shape.call("render");
    }

    if (!shape.is_a(rb_cRenderedShape)) {
        String shape_str = shape.to_s();
        throw Exception(rb_eArgError,
            "render returned %s instead of a rendered shape",
            shape_str.c_str());
    }

    return shape;
}

// TODO: better to just put things in a Data_Object to begin with, than to
// allocate them twice
static Object wrap_rendered_shape(const TopoDS_Shape &shape)
{
    return Data_Object<TopoDS_Shape>(new TopoDS_Shape(shape));
}

void shape_write_stl(Object self, String path)
{
    Data_Object<TopoDS_Shape> shape = render_shape(self);

    StlAPI_Writer writer;
    writer.ASCIIMode() = false;
    writer.RelativeMode() = false;
    writer.SetDeflection(get_tolerance());
    writer.Write(*shape, path.c_str());
}

Object shape__bbox(Object self)
{
    Data_Object<TopoDS_Shape> shape = render_shape(self);

    Standard_Real minXYZ[3];
    Standard_Real maxXYZ[3];
    Bnd_Box bbox;
    BRepBndLib::Add(*shape, bbox);
    bbox.Get(
        minXYZ[0], minXYZ[1], minXYZ[2],
        maxXYZ[0], maxXYZ[1], maxXYZ[2]);
    
    const Standard_Real gap = bbox.GetGap();
    for (int i = 0; i < 3; ++i) {
        minXYZ[i] += gap;
        maxXYZ[i] -= gap;
    }

    Array res;
    res.push(Array(minXYZ));
    res.push(Array(maxXYZ));
    return res;
}


Object shape_from_stl(String path)
{
    TopoDS_Shape shape;
    StlAPI_Reader reader;
    reader.Read(shape, path.c_str());
    return wrap_rendered_shape(shape);
}


Object transformed_shape_render(Object self)
{
    gp_GTrsf transform = from_ruby<gp_GTrsf>(self.iv_get("@trsf"));

    Object shape = self.iv_get("@shape");
    Data_Object<TopoDS_Shape> rendered = render_shape(shape);

    return wrap_rendered_shape(
        BRepBuilderAPI_GTransform(*rendered, transform, Standard_True).Shape());
}

static TopoDS_Wire make_wire_from_path(Array points, Array path)
{
    BRepBuilderAPI_MakeWire wire_maker;

    for (size_t i = 0; i < path.size(); ++i) {
        const size_t j = (i + 1) % path.size();

        const size_t p1_idx = from_ruby<size_t>(path[i]);
        const size_t p2_idx = from_ruby<size_t>(path[j]);

        gp_Pnt gp_p1(from_ruby<gp_Pnt>(points[p1_idx]));
        gp_Pnt gp_p2(from_ruby<gp_Pnt>(points[p2_idx]));

        wire_maker.Add(BRepBuilderAPI_MakeEdge(gp_p1, gp_p2).Edge());
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
        make_wire_from_path(points, paths[0]));
    for (size_t i = 1; i < paths.size(); ++i) {
        TopoDS_Wire wire = make_wire_from_path(points, paths[i]);

        // all paths except the first are inner loops,
        // so they should be reversed
        face_maker.Add(TopoDS::Wire(wire.Oriented(TopAbs_REVERSED)));
    }

    return wrap_rendered_shape(face_maker.Shape());
}

Object circle_render(Object self)
{
    Standard_Real dia = from_ruby<Standard_Real>(self.iv_get("@dia"));
    gp_Circ circ(gp_Ax2(), dia / 2.0);
    TopoDS_Edge edge = BRepBuilderAPI_MakeEdge(circ).Edge();
    TopoDS_Wire wire = BRepBuilderAPI_MakeWire(edge).Wire();
    return wrap_rendered_shape(BRepBuilderAPI_MakeFace(wire).Shape());
}


Object box_render(Object self)
{
    Standard_Real xsize = from_ruby<Standard_Real>(self.iv_get("@xsize"));
    Standard_Real ysize = from_ruby<Standard_Real>(self.iv_get("@ysize"));
    Standard_Real zsize = from_ruby<Standard_Real>(self.iv_get("@zsize"));

    return to_ruby(BRepPrimAPI_MakeBox(xsize, ysize, zsize).Shape());
}


Object cone_render(Object self)
{
    Standard_Real height = from_ruby<Standard_Real>(self.iv_get("@height"));
    Standard_Real dia1 = from_ruby<Standard_Real>(self.iv_get("@bottom_dia"));
    Standard_Real dia2 = from_ruby<Standard_Real>(self.iv_get("@top_dia"));
    return to_ruby(
        BRepPrimAPI_MakeCone(dia1 / 2.0, dia2 / 2.0, height).Shape());
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


// check if shape is inside-out and fix it if it is
static void fix_inside_out_solid(TopoDS_Solid &solid)
{
    BRepClass3d_SolidClassifier classifier(solid);
    classifier.PerformInfinitePoint(Precision::Confusion());
    if (classifier.State() == TopAbs_IN) {
        solid = TopoDS::Solid(solid.Oriented(TopAbs_REVERSED));
    }
}

static Object polyhedron_render_internal(const Array points, const Array faces)
{
    if (faces.size() < 4) {
        throw Exception(rb_eArgError,
            "Polyhedron must have at least 4 faces!");
    }

    BRepBuilderAPI_Sewing sewing;

    for (size_t i = 0; i < faces.size(); ++i) {
        TopoDS_Wire wire = make_wire_from_path(points, faces[i]);

        sewing.Add(BRepBuilderAPI_MakeFace(wire).Face());
    }

    sewing.Perform();

    TopoDS_Shell shell = TopoDS::Shell(sewing.SewedShape());
    // TODO: check for free/multiple edges and problems from sewing object

    TopoDS_Solid solid = BRepBuilderAPI_MakeSolid(shell).Solid();

    fix_inside_out_solid(solid);

    return wrap_rendered_shape(solid);
}

Object polyhedron_render(Object self)
{
    const Array points = self.iv_get("@points");
    const Array faces = self.iv_get("@faces");
    return polyhedron_render_internal(points, faces);
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


static bool is_inner_wire_of_face(TopoDS_Wire wire, TopoDS_Face face)
{
    // recipe from http://opencascade.wikidot.com/recipes
    TopoDS_Face newface = TopoDS::Face(
        face.EmptyCopied().Oriented(TopAbs_FORWARD));

    BRep_Builder builder;
    builder.Add(newface, wire);

    BRepTopAdaptor_FClass2d fclass2D(newface, Precision::PConfusion());
    return (fclass2D.PerformInfinitePoint() != TopAbs_OUT);
}

static TopoDS_Shape extrude_wire(TopoDS_Wire profile, TopoDS_Wire spine,
    TopoDS_Face spine_support)
{
    BRepOffsetAPI_MakePipeShell pipe_maker(spine);

    if (!spine_support.IsNull()) {
        if (!pipe_maker.SetMode(spine_support)) {
            throw Exception(rb_cOCEError,
                "failed setting twisted surface-normal for PipeShell");
        }
    }

    pipe_maker.Add(profile);
    const Standard_Real tolerance = get_tolerance();
    pipe_maker.SetTolerance(tolerance, tolerance);
    pipe_maker.Build();

    if (!pipe_maker.MakeSolid()) {
        throw Exception(rb_cOCEError, "failed making extrusion solid");
    }

    return pipe_maker.Shape();
}

static TopoDS_Shape extrude_face(TopoDS_Face profile, TopoDS_Wire spine,
    TopoDS_Face spine_support)
{
    // extrude outer and inner wires separately, then subtract the inner
    // shapes from the outer shape. there should be only one outer shape,
    // and zero or more inner shapes.

    TopoDS_Shape outer;

    TopoDS_Compound inner;
    BRep_Builder builder;
    builder.MakeCompound(inner);

    TopExp_Explorer texp;

    TopoDS_Face orface = TopoDS::Face(profile.Oriented(TopAbs_FORWARD));
    for (texp.Init(orface, TopAbs_WIRE); texp.More(); texp.Next()) {
        TopoDS_Wire wire = TopoDS::Wire(texp.Current());
        TopoDS_Shape ext_wire = extrude_wire(wire, spine, spine_support);

        if (is_inner_wire_of_face(wire, orface)) {
            builder.Add(inner, ext_wire);
        } else {
            outer = ext_wire;
        }
    }

    return BRepAlgoAPI_Cut(outer, inner).Shape();
}

static TopoDS_Shape extrude_shape(TopoDS_Shape profile, TopoDS_Wire spine,
    TopoDS_Face spine_support)
{
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);

    TopExp_Explorer texp;
    for (texp.Init(profile, TopAbs_FACE); texp.More(); texp.Next()) {
        builder.Add(compound,
            extrude_face(
                TopoDS::Face(texp.Current()), spine, spine_support));
    }

    return compound;
}

static TopoDS_Shape twist_extrude(TopoDS_Shape shape, Standard_Real height,
    Standard_Real twist)
{
    // split height into segments. each segment will twist no more than
    // 90 degrees.
    const int num_twist_segments = (int)(fabs(twist) / M_PI_2 + 1);
    // note that height and twist are doubles so division is not integer
    // division
    const Standard_Real seg_height = height / num_twist_segments;
    const Standard_Real seg_twist = twist / num_twist_segments;

    Handle_Geom_BezierSurface surf_hnd(
        new Geom_BezierSurface(
            TColgp_Array2OfPnt(
                1, num_twist_segments + 1,
                1, 2)));

    for (int i = 1; i <= num_twist_segments + 1; ++i) {
        const Standard_Real z = seg_height * (i - 1);
        const Standard_Real angle = seg_twist * (i - 1);
        surf_hnd->SetPole(i, 1, gp_Pnt(0, 0, z));
        surf_hnd->SetPole(i, 2, gp_Pnt(cos(angle), sin(angle), z));
    }

    TopoDS_Face spine_support =
        // TODO: tolerance
        BRepBuilderAPI_MakeFace(surf_hnd, 0, 1, 0, 1,
            Precision::Confusion());

    Handle_Geom2d_Curve uv_curve_hnd =
        GCE2d_MakeSegment(gp_Pnt2d(0, 0), gp_Pnt2d(1, 0));
    TopoDS_Edge spine = BRepBuilderAPI_MakeEdge(uv_curve_hnd, surf_hnd);
    TopoDS_Wire spine_wire = BRepBuilderAPI_MakeWire(spine);

    return extrude_shape(shape, spine_wire, spine_support);
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
        return wrap_rendered_shape(twist_extrude(*shape, height, twist));
    }
}


Object revolution_render(Object self)
{
    Object profile = self.iv_get("@profile");
    Data_Object<TopoDS_Shape> shape = render_shape(profile);

    Object angle = self.iv_get("@angle");

    gp_Circ circ(gp_Ax2(gp::Origin(), -gp::DY(), gp::DX()), 1.0);
    TopoDS_Edge edge;

    if (angle.is_nil()) {
        edge = BRepBuilderAPI_MakeEdge(circ);
    } else {
        Standard_Real angle_num = from_ruby<Standard_Real>(angle);
        angle_num = std::max(angle_num, 0.0);
        angle_num = std::min(angle_num, M_PI * 2);

        Handle_Geom_Curve curve_hnd(new Geom_Circle(circ));
        edge = BRepBuilderAPI_MakeEdge(curve_hnd, 0, angle_num);
    }

    TopoDS_Wire spine = BRepBuilderAPI_MakeWire(edge);
    return wrap_rendered_shape(
        extrude_shape(*shape, spine, TopoDS_Face()));
}


static void get_points_from_shape(const TopoDS_Shape &shape,
    std::vector<gp_Pnt> &points)
{
    TopExp_Explorer ex(shape, TopAbs_FACE);
    for (; ex.More(); ex.Next()) {
        const TopoDS_Face& face = TopoDS::Face(ex.Current());

        TopLoc_Location loc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, loc);

        if (tri.IsNull()) {
            throw Exception(rb_cOCEError, "No triangulation");
        }

        const Standard_Integer numNodes = tri->NbNodes();
        const TColgp_Array1OfPnt& nodes = tri->Nodes();
        for (Standard_Integer i = 1; i <= numNodes; i++) {
            points.push_back(
                loc.IsIdentity()
                ? nodes(i)
                : nodes(i).Transformed(loc));
        }
    }
}

static std::vector<gp_Pnt> get_points_from_shapes(Array shapes)
{
    std::vector<gp_Pnt> points;

    const Standard_Real tolerance = get_tolerance();

    for (size_t i = 0; i < shapes.size(); ++i) {
        Data_Object<TopoDS_Shape> shape_obj = render_shape(shapes[i]);
        const TopoDS_Shape &shape = *shape_obj;

        BRepMesh_IncrementalMesh(shape, tolerance);

        get_points_from_shape(shape, points);
    }

    return points;
}


// Sort function object to sort the vertices of a convex polygon
// Algorithm from here:
// http://stackoverflow.com/a/15104911/2758814
class ConvexPolygonVertexSortComparator
{
public:
    ConvexPolygonVertexSortComparator(std::vector<gp_Pnt> vertices)
        : vertices(vertices)
    {
        if (vertices.size() == 0) {
            throw std::exception();
        }

        // get center point
        for (size_t i = 0; i < vertices.size(); ++i) {
            center.Translate(gp::Origin(), vertices[i]);
        }
        center.Scale(gp::Origin(), 1.0/vertices.size());

        center_to_first_vert = gp_Vec(center, vertices[0]);

        // TODO: ensure that normals are all in consistent direction
        // by specifying a point that should be on the front or back side of
        // the polygon

        // get a normal with one of the other vectors
        // (must find a non-zero normal, i.e. other vector must not be
        // parallel to first vector)
        for (size_t i = 1; i < vertices.size(); ++i) {
            gp_Vec another_vec = gp_Vec(center, vertices[i]);
            normal = center_to_first_vert ^ another_vec;
            if (normal.Magnitude() > Precision::Confusion()) {
                break;
            }
        }

        if (normal.Magnitude() <= Precision::Confusion()) {
            // couldn't find a non-zero normal. normal still zero
            throw std::exception();
        }
    }

    bool operator() (gp_Pnt a, gp_Pnt b) {
        double angle1 = get_angle_to_vec(a);
        double angle2 = get_angle_to_vec(b);
        return angle1 < angle2;
    }

private:
    double get_angle_to_vec(gp_Pnt vertex) {
        gp_Vec center_to_vertex(center, vertex);

        // use normal as reference to define positive rotation angle
        return center_to_first_vert.AngleWithRef(center_to_vertex, normal);
    }

    std::vector<gp_Pnt> vertices;
    gp_Pnt center;
    gp_Vec center_to_first_vert;
    gp_Vec normal;
};

static TopoDS_Solid make_solid_from_qhull()
{
    BRepBuilderAPI_Sewing sewing;

    facetT *facet;
    FORALLfacets {
        // get vertices into an std::vector
        std::vector<gp_Pnt> vertices;
        vertices.reserve(qh_setsize(facet->vertices));

        vertexT *vertex, **vertexp;
        FOREACHvertex_(facet->vertices) {
            vertices.push_back(gp_Pnt(vertex->point[0], vertex->point[1],
                vertex->point[2]));
        }

        sort(vertices.begin(), vertices.end(),
            ConvexPolygonVertexSortComparator(vertices));

        BRepBuilderAPI_MakePolygon poly_maker;
        for (size_t i = 0; i < vertices.size(); ++i) {
            poly_maker.Add(vertices[i]);
        }
        poly_maker.Close();

        sewing.Add(BRepBuilderAPI_MakeFace(poly_maker.Wire()).Face());
    }

    sewing.Perform();

    TopoDS_Shell shell = TopoDS::Shell(sewing.SewedShape());
    // TODO: check for free/multiple edges and problems from sewing object

    TopoDS_Solid solid = BRepBuilderAPI_MakeSolid(shell).Solid();

    fix_inside_out_solid(solid);

    return solid;
}

static Object _hull(Array shapes)
{
    try {
        std::vector<gp_Pnt> points = get_points_from_shapes(shapes);

        char flags[128];
        strcpy(flags, "qhull Qt");
        int err = qh_new_qhull(3, points.size(),
            // each point contains a gp_XYZ which contains X,Y,Z as Standard_Reals
            reinterpret_cast<Standard_Real*>(points.data()),
            false, flags, NULL, stderr);
        if (err) {
            throw Exception(rb_cOCEError, "Error running qhull");
        }

        TopoDS_Solid solid = make_solid_from_qhull();

        qh_freeqhull(!qh_ALL);
        int curlong, totlong;
        qh_memfreeshort(&curlong, &totlong);
        if (curlong || totlong) {
            throw Exception(rb_cOCEError,
                "did not free %d bytes of long memory (%d pieces)",
                totlong, curlong);
        }

        return wrap_rendered_shape(solid);
    } catch (const Standard_Failure &e) {
        // this throws an exception, so return won't be reached
        translate_oce_exception(e);
        return Object(Qnil);
    }
}


extern "C"
void Init__rcad()
{
    rb_cRenderedShape = define_class<TopoDS_Shape>("RenderedShape");

    Data_Type<Standard_Failure> rb_cOCEError =
        define_class("OCEError", rb_eRuntimeError);

    Class rb_cTransform = define_class<gp_GTrsf>("Transform")
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("to_s", &transform_to_s)
        .define_method("mat_part", &transform_mat_part)
        .define_method("mat_part=", &transform_mat_part_set)
        .define_method("ofs_part", &transform_ofs_part)
        .define_method("ofs_part=", &transform_ofs_part_set)
        .define_method("*", &transform_multiply)
        .define_method("_move", &transform__move)
        .define_method("_rotate", &transform__rotate)
        .define_method("_scale", &transform__scale)
        .define_method("_mirror", &transform__mirror)
        .define_method("inverse", &transform_inverse);

    rb_const_set(rb_cObject, Identifier("I"), to_ruby<gp_GTrsf>(gp_GTrsf()));


    rb_cShape = define_class("Shape")
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("write_stl", &shape_write_stl)
        .define_method("_bbox", &shape__bbox)
        .define_singleton_method("from_stl", &shape_from_stl);

    Class rb_cTransformedShape = define_class("TransformedShape", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &transformed_shape_render);

    Class rb_cPolygon = define_class("Polygon", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &polygon_render);

    Class rb_cCircle = define_class("Circle", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &circle_render);


    Class rb_cBox = define_class("Box", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &box_render);

    Class rb_cCone = define_class("Cone", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &cone_render);

    Class rb_cCylinder = define_class("Cylinder", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &cylinder_render);

    Class rb_cSphere = define_class("Sphere", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &sphere_render);

    Class rb_cPolyhedron = define_class("Polyhedron", rb_cShape)
        .add_handler<Standard_Failure>(translate_oce_exception)
        .define_method("render", &polyhedron_render);

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

    define_global_function("_hull", &_hull);
}
