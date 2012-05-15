#include "gcode.h"
#include <cmath>
#include <float.h>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <iostream>
#include <QMessageBox>


using namespace std;
using namespace mgl;
using namespace libthing;


char gcode::codes[] =  "ABDEFGHIJKLMPQRSTXYZ";


gcode::gcode(string command) :
    command(command) {

    //	cout << "parsing command: " << command << std::endl;

    // Parse (and strip) any comments out into a comment string
    parseComments();

    // Parse any codes out into the code tables
    parseCodes();
}


// Find any comments, store them, then remove them from the command
// TODO: Handle this correctly. For now, we just look for ( and bail.
void gcode::parseComments() {
    if (command.find_first_of(")") != string::npos) {
        comment = command.substr(command.find_first_of("(")+1);
        command = command.substr(0,command.find_first_of("("));
        //		cout << " comment=" << comment << std::endl;
    }

}


// Find any codes, and store them
// TODO: write this correctly, handle upper/lower, spacing between code and numbers, error reporting, etc.
void gcode::parseCodes() {
    // For each code letter we know about, scan for it and record it's value.
    int codeIndex = 0;

    while (codes[codeIndex]!=0) {
        // Search the command for an occurance of said code letter
        if (command.find_first_of(codes[codeIndex]) != string::npos) {
            double value = atof(command.substr(command.find_first_of(codes[codeIndex])+1).c_str());

            //			cout << " code=" << codes[codeIndex] << " value=" << value << std::endl;
            parameters.push_back(gCodeParameter(codes[codeIndex],value));
        }
        codeIndex++;
    }
}


string gcode::getCommand() {
    // TODO: Note that this is the command minus any comments.
    return string(command);
}


string gcode::getComment() {
    return string(comment);
}


bool gcode::hasCode(char searchCode) {
    for (unsigned int i = 0; i < parameters.size(); i++) {
        if(parameters[i].code == searchCode) {
            return true;
        }
    }

    return false;
}


double gcode::getCodeValue(char searchCode) {
    for (unsigned int i = 0; i < parameters.size(); i++) {
        if(parameters[i].code == searchCode) {
            return parameters[i].value;
        }
    }

    return -1;	// TODO: What do we return if there is no code?
}


bool layerMap::heightInLayer(int layer, float height) {
    return (std::fabs(heights[layer] - height) < .07);
}

bool layerMap::heightGreaterThanLayer(int layer, float height) {
    return (!heightInLayer(layer, height) && height > heights[layer]);
}

bool layerMap::heightLessThanLayer(int layer, float height) {
    return (!heightInLayer(layer, height) && height < heights[layer]);
}

// Record that we've seen a specific z height. If it's already in the list, it is ignored, otherwise it is added.
void layerMap::recordHeight(float height) {
    for (unsigned int i = 0; i < heights.size(); i++) {
        if (heightInLayer(i, height)) {
            return;
        }
    }
    heights.push_back(height);
}

// Get the height corresponding to a given layer
float layerMap::getLayerHeight(int layer) {
    return heights.at(layer);
}

// Return the number of layers that we know about
int layerMap::size() {
    return heights.size();
}

// Clear the map out.
void layerMap::clear() {
    heights.clear();
}

gcodeModel::gcodeModel()
    :layerMeasure(0,0)
{
    toolEnabled = false;
    viewSurfs = true;
    viewRoofs = true;
    viewFloors =true;
    viewLoops =true;
    viewInfills = true;
}

gcodeModel::~gcodeModel()
{
    cout << "~gcodeModel()" << this << endl;
}

float gcodeModel::getModelZCenter() {
    return (zHeightBounds.getMax() - zHeightBounds.getMin())/2 + zHeightBounds.getMin();
}


void gcodeModel::saveMiracleGcode(const char *filename, void *prog)
{

    ProgressBar *progress = (ProgressBar *)prog;

    cout << "gcodeModel::saveMiracleGcode: " << filename << endl;
    std::ofstream gout(filename);
    GCoder gcoder(gcoderCfg, progress);
    gcoder.writeGcodeFile(slices, layerMeasure, gout, modelFile.c_str());
    gout.close();

}

void gcodeModel::exportGCode(QString filename) {
    //Open the file that was specificied and create a stream to write to it.
    QFile f(filename);
    f.open( QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&f);

/*
    //"Unparse" and send Gcode to the output stream
    out << "(This is a G-Code file generated by Makerbot's GCode Viewer Application)\n";

    float lastflowrate = FLT_MIN;
    for(unsigned int i=0; i<.size(); i++)
    {
        point data = points.at(i);

            if (data.flowrate != lastflowrate){
                out << "M108 R";
                out << data.flowrate;
                out << "\n";
            }
            out << "G1";
            if (data.x > 1e-36 || data.x <= 0.0){
                out << " X";
                out << data.x;
            }
            if (data.y > 1e-36 || data.y <= 0.0){
                out << " Y";
                out << data.y;
            }
            if (data.z > 1e-36 || data.z <= 0.0){
                out << " Z";
                out << data.z;
            }
            if (data.feedrate > 1e-36){
                out << " F";
                out << data.feedrate;
            }
            out << "\n";

            lastflowrate = data.flowrate;


    }
*/
    f.close();
}

void gcodeModel::loadGcodeLine(const char* lineStr)
{
        string line(lineStr);
        gcode code = gcode(line);

        float feedrate = FLT_MIN;
        float flowrate = FLT_MIN;
        float xPos = FLT_MIN;
        float yPos = FLT_MIN;
        float zPos = FLT_MIN;

        PointKind kind = unknown;
        int nb = 0;

        // cout << " hascodeG:" << code.hasCode('G') << std::endl;

        // If the code contains a flowrate
        if (code.hasCode('M') && (int)code.getCodeValue('M') == 101) {
            toolEnabled = true;
        }
        else if (code.hasCode('M') && (int)code.getCodeValue('M') == 102) {
            toolEnabled = true;
            // reversal!
        }
        else if (code.hasCode('M') && (int)code.getCodeValue('M') == 103) {

            toolEnabled = false;
        }
        else if (code.hasCode('M') && (int)code.getCodeValue('M') == 108) {
          if (code.hasCode('S')) {
              flowrate = code.getCodeValue('S');
              flowrateBounds.evaluate(flowrate);
          }
          else if (code.hasCode('R')) {
              flowrate = code.getCodeValue('R');
              flowrateBounds.evaluate(flowrate);
          }
          else {
              // TODO
          }
        }
        // If the code contains a movement
        else if (code.hasCode('G') && (int)code.getCodeValue('G') == 1) {
            // Pull coordinates out of it. This is not to spec (i think any time these are present, they mean go here)
            // but we need to just define a standard and stick to it. The standard is in the form of an imaginary test suite.
            if (code.hasCode('X')) {
                xPos = code.getCodeValue('X');
            }
            if (code.hasCode('Y')) {
                yPos = code.getCodeValue('Y');
            }
            if (code.hasCode('Z')) {
                zPos = code.getCodeValue('Z');
                zHeightBounds.evaluate(zPos);
                map.recordHeight(zPos);
            }
            if (code.hasCode('F')) {
                feedrate = code.getCodeValue('F');
                feedrateBounds.evaluate(feedrate);
            }

            if(!toolEnabled)
            {
                kind = travel;
            }
            // let's add it to our list.
            points.push_back(point(kind, nb,  xPos, yPos, zPos, feedrate, flowrate));


        }

}



void addPointsFromPolygon(const Polygon &poly,
                          float xOff,
                          float yOff,
                          float z,
                          PointKind kind, int nb, float feedrate, float flowrate, vector<point> &points,
                          layerMap& )
{

    for(unsigned int i=0; i < poly.size(); i++)
    {
        Vector2 p = poly[i];
        points.push_back(point(kind, nb,  p[0] + xOff, p[1] + yOff, z, feedrate, flowrate));

    }
}

void addPointsFromPolygons(const Polygons& polys, float xOff, float yOff, float z, PointKind kind, PointKind interPointKind, int nb,  float feedrate, float flowrate, vector<point> &points, layerMap& map)
{
    map.recordHeight(z);
    for(unsigned int i=0; i < polys.size(); i++)
    {
       const Polygon &poly = polys[i];
       if(!poly.size()) continue;
       // move to
       const Vector2 p = poly[0];

       points.push_back(point(interPointKind, 0, xOff + p[0], yOff + p[1] , z, feedrate, flowrate));
       // polygon
       addPointsFromPolygon(poly, xOff, yOff, z, kind, nb, feedrate , flowrate, points, map);
    }
}


void addPointsFromSurface(const GridRanges& gridRanges, const Grid & grid, float z, PointKind kind, float xOff, float yOff, vector<point> &points, layerMap& map)
{
    float feedrate = 10;
    float flowrate = 5;


    map.recordHeight(z);
    for(size_t i=0; i < gridRanges.xRays.size(); i++)
    {
        float y = grid.readYvalues()[i];
        const std::vector<ScalarRange>  &line = gridRanges.xRays[i];
        for(size_t j=0; j < line.size(); j++)
        {
            const ScalarRange &range = line[j];
            points.push_back(point(invisible, 0, xOff + range.min, yOff+y, z, 0, 0));
            points.push_back(point(kind, 0,  xOff + range.min, yOff+y, z, feedrate, flowrate));
            points.push_back(point(kind, 0,  xOff + range.max, yOff+y, z, feedrate, flowrate));
        }
    }

    for(size_t i=0; i < gridRanges.yRays.size(); i++)
    {
        float x = grid.readXvalues()[i];
        const std::vector<ScalarRange>  &line = gridRanges.yRays[i];
        for(size_t j=0; j < line.size(); j++)
        {
            const ScalarRange &range = line[j];
            points.push_back(point(invisible, 0,  xOff + x, yOff+range.min, z, 0, 0));
            points.push_back(point(kind, 1,  xOff + x, yOff+range.min, z, feedrate, flowrate));
            points.push_back(point(kind, 1, xOff + x, yOff+range.max, z, feedrate, flowrate));

        }
    }
}

void gcodeModel::loadRegions(const Tomograph &tomograph, const mgl::Regions &regions)
{
    float xOff = 3 * tomograph.grid.readXvalues()[0] + tomograph.grid.readXvalues().back();
    float yOff = 3 * tomograph.grid.readYvalues()[0] + tomograph.grid.readYvalues().back();

    for(size_t i=0; i<regions.roofings.size(); i++)
    {
        const GridRanges &surface = regions.roofings[i];
        float z = tomograph.layerMeasure.sliceIndexToHeight(i);
       // addPointsFromSurface(surface, skeleton.grid,  z, roofing, xOff, 0, points, map);

        addPointsFromSurface(surface, tomograph.grid,  z, roofing, 0, -yOff, points, map);
    }

    for(size_t i=0; i< regions.floorings.size(); i++)
    {
        const GridRanges &surface = regions.floorings[i];
        float z = tomograph.layerMeasure.sliceIndexToHeight(i);
     //   addPointsFromSurface(surface, skeleton.grid,  z, flooring, xOff, 0, points, map);
        addPointsFromSurface(surface, tomograph.grid,  z, flooring,    0, yOff, points, map);
    }

    for(size_t i=0; i < regions.flatSurfaces.size(); i++)
    {
        const GridRanges &surf = regions.flatSurfaces[i];
        float z = tomograph.layerMeasure.sliceIndexToHeight(i);
        addPointsFromSurface(surf, tomograph.grid,  z, surface, xOff, 0, points, map);
    }

}

void gcodeModel::loadSliceData(const Tomograph& tomograph,
                                const mgl::Regions &regions,
                                 const std::vector<mgl::SliceData> &slices)
{

    points.clear();
    map.clear();
    map.recordHeight(0);


    loadRegions(tomograph, regions);


    float feedrate = 2400;
    float flowrate = 4;
    feedrateBounds.evaluate(feedrate);
    flowrateBounds.evaluate(flowrate);

    for (unsigned int i = 0; i < slices.size(); i++)
    {
        const SliceData &sliceData = slices[i];
        for(unsigned int extruderId = 0; extruderId < sliceData.extruderSlices.size(); extruderId++)
        {
            const ExtruderSlice &slice = sliceData.extruderSlices[extruderId];
            float z = (float) sliceData.getZHeight();

            // cout << "sazz "  <<endl;
            const Polygons &boundaries = slice.boundary;
            const Polygons &infills = slice.infills;

            // main view
            addPointsFromPolygons(boundaries, 0, 0, z,  perimeter, travel, 0, feedrate, flowrate, points, map);
            addPointsFromPolygons(infills, 0, 0, z, infill, travel, 0, feedrate, flowrate,  points, map);

            for(unsigned int j=0; j< slice.insetLoopsList.size(); j++)
            {
                const Polygons& insetLoops = slice.insetLoopsList[j];
                addPointsFromPolygons(insetLoops, 0, 0, z, shell, travel, j, feedrate, flowrate,  points, map);
            }
        }
    }

//    if(viewLoops)
//    {
//        for (unsigned int i = 0; i < slices.size(); i++)
//        {
//            const SliceData &sliceData = slices[i];
//            for(unsigned int extruderId = 0; extruderId < sliceData.extruderSlices.size(); extruderId++)
//            {
//                const ExtruderSlice &slice = sliceData.extruderSlices[extruderId];
//                float z = (float) sliceData.getZHeight();

//                //cout << "sazz "  <<endl;
//                const Polygons &boundaries = slice.boundary;
//                const Polygons &infills = slice.infills;
//                //cout << "slice " << slice.insetLoopsList.size() << endl;
//                //cout << "BOUNDARY COUNT " << boundary.size() << endl;

//                 // right side
//                addPointsFromPolygons(boundaries, xOff, 0, z,  perimeter, invisible, 0, feedrate, flowrate, points, map);

//                for(unsigned int j=0; j< slice.insetLoopsList.size(); j++)
//                {
//                    const Polygons& insetLoops = slice.insetLoopsList[j];
//                    // right side
//                    addPointsFromPolygons(insetLoops, xOff, 0, z, shell, invisible, j, feedrate, flowrate,  points, map);
//                }
//            }

//        }
//    }

}

void gcodeModel::loadGCode(QString q)
{
    string filename = q.toStdString();


    cout << "loadGCode: " << filename << endl;

    string extension = filename.substr(filename.find_last_of('.'), filename.size());
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    cout << "extension "<< extension << endl;

    if(extension.find("gcode") != string::npos)
    {
        cout << "loading gcode: " << filename << endl;
        ifstream file;
        file.open(filename.c_str());
        points.clear();
        map.clear();


        while (file.good())
        {
            string line;
            std::getline(file, line);
            loadGcodeLine( line.c_str());
        }
        file.close();
    }

}
