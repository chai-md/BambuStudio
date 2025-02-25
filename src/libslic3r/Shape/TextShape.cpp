#include "../libslic3r.h"
#include "../Model.hpp"
#include "../TriangleMesh.hpp"

#include "TextShape.hpp"

#include <string>
#include <vector>

#include "Standard_TypeDef.hxx"
#include "STEPCAFControl_Reader.hxx"
#include "BRepMesh_IncrementalMesh.hxx"
#include "Interface_Static.hxx"
#include "XCAFDoc_DocumentTool.hxx"
#include "XCAFDoc_ShapeTool.hxx"
#include "XCAFApp_Application.hxx"
#include "TopoDS_Solid.hxx"
#include "TopoDS_Compound.hxx"
#include "TopoDS_Builder.hxx"
#include "TopoDS.hxx"
#include "TDataStd_Name.hxx"
#include "BRepBuilderAPI_Transform.hxx"
#include "TopExp_Explorer.hxx"
#include "TopExp_Explorer.hxx"
#include "BRep_Tool.hxx"
#include "Font_BRepFont.hxx"
#include "Font_BRepTextBuilder.hxx"
#include "BRepPrimAPI_MakePrism.hxx"
#include "Font_FontMgr.hxx"

namespace Slic3r {

std::vector<std::string> init_occt_fonts()
{
    std::vector<std::string> stdFontNames;

    Handle(Font_FontMgr) aFontMgr = Font_FontMgr::GetInstance();
    aFontMgr->InitFontDataBase();

    TColStd_SequenceOfHAsciiString availFontNames;
    aFontMgr->GetAvailableFontsNames(availFontNames);
    stdFontNames.reserve(availFontNames.Size());

    for (auto afn : availFontNames)
        stdFontNames.push_back(afn->ToCString());

    return stdFontNames;
}

static bool TextToBRep(const char* text, const char* font, const float theTextHeight, Font_FontAspect& theFontAspect, TopoDS_Shape& theShape)
{
    Standard_Integer anArgIt = 1;
    Standard_CString aName = "text_shape";
    Standard_CString aText = text;

    Font_BRepFont           aFont;
    //TCollection_AsciiString aFontName("Courier");
    TCollection_AsciiString aFontName(font);
    Standard_Real           aTextHeight = theTextHeight;
    Font_FontAspect         aFontAspect = theFontAspect;
    Standard_Boolean        anIsCompositeCurve = Standard_False;
    gp_Ax3                  aPenAx3(gp::XOY());
    gp_Dir                  aNormal(0.0, 0.0, 1.0);
    gp_Dir                  aDirection(1.0, 0.0, 0.0);
    gp_Pnt                  aPenLoc;

    Graphic3d_HorizontalTextAlignment aHJustification = Graphic3d_HTA_LEFT;
    Graphic3d_VerticalTextAlignment   aVJustification = Graphic3d_VTA_BOTTOM;
    Font_StrictLevel aStrictLevel = Font_StrictLevel_Any;

    aFont.SetCompositeCurveMode(anIsCompositeCurve);
    if (!aFont.FindAndInit(aFontName.ToCString(), aFontAspect, aTextHeight, aStrictLevel))
        return false;

    aPenAx3 = gp_Ax3(aPenLoc, aNormal, aDirection);

    Font_BRepTextBuilder aBuilder;
    theShape = aBuilder.Perform(aFont, aText, aPenAx3, aHJustification, aVJustification);
    return true;
}

static bool Prism(const TopoDS_Shape& theBase, const float thickness, TopoDS_Shape& theSolid)
{
    if (theBase.IsNull()) return false;

    gp_Vec V(0.f, 0.f, thickness);
    BRepPrimAPI_MakePrism* Prism = new BRepPrimAPI_MakePrism(theBase, V, Standard_False);

    theSolid = Prism->Shape();
    return true;
}

static void MakeMesh(TopoDS_Shape& theSolid, TriangleMesh& theMesh)
{
    const double STEP_TRANS_CHORD_ERROR = 0.005;
    const double STEP_TRANS_ANGLE_RES = 1;

    BRepMesh_IncrementalMesh mesh(theSolid, STEP_TRANS_CHORD_ERROR, false, STEP_TRANS_ANGLE_RES, true);
    int aNbNodes = 0;
    int aNbTriangles = 0;
    for (TopExp_Explorer anExpSF(theSolid, TopAbs_FACE); anExpSF.More(); anExpSF.Next()) {
        TopLoc_Location aLoc;
        Handle(Poly_Triangulation) aTriangulation = BRep_Tool::Triangulation(TopoDS::Face(anExpSF.Current()), aLoc);
        if (!aTriangulation.IsNull()) {
            aNbNodes += aTriangulation->NbNodes();
            aNbTriangles += aTriangulation->NbTriangles();
        }
    }

    stl_file stl;
    stl.stats.type = inmemory;
    stl.stats.number_of_facets = (uint32_t)aNbTriangles;
    stl.stats.original_num_facets = stl.stats.number_of_facets;
    stl_allocate(&stl);

    std::vector<Vec3f> points;
    points.reserve(aNbNodes);
    //BBS: count faces missing triangulation
    Standard_Integer aNbFacesNoTri = 0;
    //BBS: fill temporary triangulation
    Standard_Integer aNodeOffset = 0;
    Standard_Integer aTriangleOffet = 0;
    for (TopExp_Explorer anExpSF(theSolid, TopAbs_FACE); anExpSF.More(); anExpSF.Next()) {
        const TopoDS_Shape& aFace = anExpSF.Current();
        TopLoc_Location aLoc;
        Handle(Poly_Triangulation) aTriangulation = BRep_Tool::Triangulation(TopoDS::Face(aFace), aLoc);
        if (aTriangulation.IsNull()) {
            ++aNbFacesNoTri;
            continue;
        }
        //BBS: copy nodes
        gp_Trsf aTrsf = aLoc.Transformation();
        for (Standard_Integer aNodeIter = 1; aNodeIter <= aTriangulation->NbNodes(); ++aNodeIter) {
            gp_Pnt aPnt = aTriangulation->Node(aNodeIter);
            aPnt.Transform(aTrsf);
            points.emplace_back(std::move(Vec3f(aPnt.X(), aPnt.Y(), aPnt.Z())));
        }
        //BBS: copy triangles
        const TopAbs_Orientation anOrientation = anExpSF.Current().Orientation();
        for (Standard_Integer aTriIter = 1; aTriIter <= aTriangulation->NbTriangles(); ++aTriIter) {
            Poly_Triangle aTri = aTriangulation->Triangle(aTriIter);

            Standard_Integer anId[3];
            aTri.Get(anId[0], anId[1], anId[2]);
            if (anOrientation == TopAbs_REVERSED) {
                //BBS: swap 1, 2.
                Standard_Integer aTmpIdx = anId[1];
                anId[1] = anId[2];
                anId[2] = aTmpIdx;
            }
            //BBS: Update nodes according to the offset.
            anId[0] += aNodeOffset;
            anId[1] += aNodeOffset;
            anId[2] += aNodeOffset;
            //BBS: save triangles facets
            stl_facet facet;
            facet.vertex[0] = points[anId[0] - 1].cast<float>();
            facet.vertex[1] = points[anId[1] - 1].cast<float>();
            facet.vertex[2] = points[anId[2] - 1].cast<float>();
            facet.extra[0] = 0;
            facet.extra[1] = 0;
            stl_normal normal;
            stl_calculate_normal(normal, &facet);
            stl_normalize_vector(normal);
            facet.normal = normal;
            stl.facet_start[aTriangleOffet + aTriIter - 1] = facet;
        }

        aNodeOffset += aTriangulation->NbNodes();
        aTriangleOffet += aTriangulation->NbTriangles();
    }

    theMesh.from_stl(stl);
}

void load_text_shape(const char*text, const char* font, const float text_height, const float thickness, bool is_bold, bool is_italic, TriangleMesh& text_mesh)
{
    Handle(Font_FontMgr) aFontMgr = Font_FontMgr::GetInstance();
    if (aFontMgr->GetAvailableFonts().IsEmpty())
        aFontMgr->InitFontDataBase();

    TopoDS_Shape aTextBase;
    Font_FontAspect aFontAspect = Font_FontAspect_UNDEFINED;
    if (is_bold && is_italic)
        aFontAspect = Font_FontAspect_BoldItalic;
    else if (is_bold)
        aFontAspect = Font_FontAspect_Bold;
    else if (is_italic)
        aFontAspect = Font_FontAspect_Italic;
    else
        aFontAspect = Font_FontAspect_Regular;

    if (!TextToBRep(text, font, text_height, aFontAspect, aTextBase))
        return;

    TopoDS_Shape aTextShape;
    if (!Prism(aTextBase, thickness, aTextShape))
        return;

    MakeMesh(aTextShape, text_mesh);
}

}; // namespace Slic3r
