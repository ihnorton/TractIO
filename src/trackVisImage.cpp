#include <memory>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <numeric>

#include "vtkPolyData.h"
#include "vtkMatrix4x4.h"
#include "vtkSmartPointer.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"

#include "tractIOCommon.h"
#include "trackVisImage.h"

namespace {
    size_t tv_trackblock_size(size_t n_pts, size_t n_scalars, size_t n_properties) {
       return sizeof(float) * TV_TRACK_NUMPTS * (n_pts + (n_pts * n_scalars) + (n_properties));
    }
};

/*
class TVTrack {
    public:
        int n_points;
        float **track_data;
        float *track_properties;

    private:
        namespace {
            class _datadeleter {
            void operator()(char* mem) { if (mem) free(mem); }
            }
        }
        std::unique_ptr<char*, _datadeleter> data;
};
*/

struct TVTrack {
    int n_points;
    float **track_data;
    float *track_properties;
};

class TVReader {
// methods
    public:
        TVReader(std::string filename);
        void dump_header(std::ostream &s);

        vtkSP<vtkMatrix4x4> get_vox_to_ras();
        vtkSP<vtkPolyData> get_polydata();

// methods
    private:
        std::unique_ptr<TVTrack> read_track(size_t n);

// data
    private:
        TVHeader header;
        size_t filesize;
        std::unique_ptr<std::istream> stream;
        std::vector< std::pair<size_t, size_t> > offsets;
};

TVReader::TVReader(std::string filename)
    : stream( std::make_unique<std::fstream>(std::fstream(filename, ios::in | ios::binary)) )
{
    if (stream->fail())
        throw;
    this->stream->seekg(0, ios::end);
    if ( (this->filesize = this->stream->tellg()) < TV_HEADERLEN)
        throw;

    this->stream->seekg(0);
    stream->read((char*)&header, sizeof(TVHeader));
}

vtkSP<vtkMatrix4x4>
TVReader::get_vox_to_ras() {
    vtkNew<vtkMatrix4x4> mat;

    for (size_t i = 0; i < 4; i++)
        for (size_t j = 0; j < 4; j++)
            mat->SetElement(i,j, header.vox_to_ras[i][j]);
    return vtkSP<vtkMatrix4x4>(mat.GetPointer());
}

std::unique_ptr<TVTrack>
TVReader::read_track(size_t n) {
    size_t cur_offset = (offsets.size() > 0 ? offsets.back().second : TV_HEADERLEN);
    while (offsets.size() < n) {
        int n_pts;
        if (cur_offset >= this->filesize)
            continue;
        stream->seekg(cur_offset);

        this->stream->read((char*)&n_pts, sizeof(int));
        cur_offset += tv_trackblock_size(offsets.back().first, header.n_scalars, header.n_properties);
        offsets.push_back(std::make_pair(n_pts, cur_offset));
    }

    if (cur_offset == TV_HEADERLEN || cur_offset >= this->filesize)
        return nullptr;

    this->stream->seekg(offsets[n].second, ios::beg);
    size_t blocksize = tv_trackblock_size(offsets[n].first, header.n_scalars, header.n_properties);

    char *block = (char*)malloc(blocksize);

}

vtkSP<vtkPolyData>
TVReader::get_polydata() {
    vtkNew<vtkPolyData> pd;
    stream->seekg(TV_HEADERLEN, ios::beg);

    // default case for data from DiffusionToolkit
    if (header.n_scalars == 0 && header.n_properties == 0) {

        vtkNew<vtkCellArray>  lines;
        vtkNew<vtkFloatArray> pointdata;
        pointdata->SetNumberOfComponents(TV_TRACK_NUMPTS);

        size_t cur_idx = 0; // global index to point list
        int    cur_pts = 0; // point count: int by TrackVis definition
        while (stream->tellg() < this->filesize) {

            stream->read((char*)&cur_pts, sizeof(int));
            stream->read(// get pointer for direct write to new points
                         (char*)pointdata->WriteVoidPointer(
                                               cur_idx * TV_TRACK_NUMPTS,
                                               cur_pts * TV_TRACK_NUMPTS),
                         // points * components * size
                         cur_pts * TV_TRACK_NUMPTS * sizeof(float));

            /* associate point indices to new cell for this line */
            std::vector<vtkIdType> idlist(cur_pts);
            std::iota(idlist.begin(), idlist.end(), cur_idx);
            lines->InsertNextCell(cur_pts, idlist.data());

            //std::cout << "pos:" << stream->tellg() << " cur_idx: " << cur_idx << " cur_pts: " << cucurr_pts << std::endl;
            cur_idx += cur_pts;
        }

        vtkNew<vtkPoints> points;
        points->SetDataTypeToFloat();
        points->SetData(pointdata.GetPointer());
        pd->SetLines(lines.GetPointer());
        pd->SetPoints(points.GetPointer());
    } else {
        // not implemented. properties and scalars not currently used by TrackVis/DiffusionToolkit
    }
    return pd.GetPointer();
}

// Util

void
TVReader::dump_header(std::ostream &ostr) {
    ostr << std::left << std::setw(10) << "id_string:" << header.id_string << std::endl;
    ostr << std::left << "dim[0]: " << header.dim[0] <<
                         " dim[1]: " << header.dim[1] <<
                         " dim[2]: " << header.dim[2] << std::endl;
    ostr << std::left << std::setw(10) << "n_scalars: " << header.n_scalars << std::endl;
    ostr << std::left << std::setw(10) << "n_properties: " << header.n_properties << std::endl;
    ostr << std::left << std::setw(10) << "n_points: " << header.n_count << std::endl;
}

// C API
#include "vtkPolyDataWriter.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"

TrackVisReader* tv_reader_load(const char *filename) {
    std::cout << "sizeof TVHeader: " << sizeof(TVHeader) << std::endl;
    try {
        auto r = new TVReader(filename);
        if (!r)
            return NULL;
        r->dump_header(std::cout);
        auto mat = r->get_vox_to_ras();
        mat->Print(std::cout);
        auto pd = r->get_polydata();
        pd->Print(std::cout);
        std::cout << "pd lines: " << pd->GetNumberOfLines() << std::endl;

        vtkNew<vtkTransformPolyDataFilter> tfmpd;
        vtkNew<vtkTransform>               tfm;
//        tfm->SetMatrix(mat);
        tfmpd->SetTransform(tfm.GetPointer());
        tfmpd->SetInputData(pd.GetPointer());
        tfmpd->Update();

        vtkNew<vtkPolyDataWriter> writer;
        writer->SetFileName("out.vtk");
        writer->SetInputConnection(tfmpd->GetOutputPort());
        writer->Update();
        writer->Write();

        return r;
    } catch (...) {
        std::cerr << "tv_reader_load failed." << std::endl;
        return nullptr;
    }
}
