// std includes
#include <cstdint>
#include <memory>

// VTK includes
#include "vtkSmartPointer.h"
#include "vtkPolyData.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkDataArray.h"
#include "vtkXMLPolyDataReader.h"

// DCMTK includes
#include "dcmtk/config/osconfig.h"    /* make sure OS specific configuration is included first */

#define INCLUDE_CMATH
#include "dcmtk/ofstd/ofstdinc.h"
#include "dcmtk/ofstd/ofcond.h"
#include "dcmtk/dcmiod/iodreferences.h"
#include "dcmtk/dcmtract/trctractographyresults.h"
#include "dcmtk/dcmtract/trctrackset.h"
#include "dcmtk/dcmtract/trcmeasurement.h"
#include "dcmtk/dcmtract/trctrack.h"
#include "dcmtk/dcmtract/trcutils.h"

// Shared ptr aliases
#define SP std::shared_ptr
#define vtkSP vtkSmartPointer

#define TIO_MANUFACTURER "libTractIO";
#define TIO_MANUFACTURER_MODELNAME "vtktodicom";
#define TIO_DEVICESERIALNUMBER "0000";
#define TIO_SOFTWAREVERSIONS "TractIO 0.1\\DCMTK 3.6.1";


/* ------------------------------------------------------------------------- */
SP<TrcTractographyResults> create_dummy_dicom()
{
  OFCondition result;
  SP<TrcTractographyResults> tract;
  TrcTractographyResults *p_tract;

  ContentIdentificationMacro* contentID = NULL;
  result = ContentIdentificationMacro::create("1", "TRACT_TEST_LABEL",
                                              "Tractography from VTK file",
                                              "TractIO",
                                              contentID);

  if (result.good())
    {
    /* image reference */
    OFString uidroot("1.2.3.4.5.6.7.8");
    char studyUID[65];
    char seriesUID[65];
    char instanceUID[65];
    sprintf(studyUID, "%s.1.%u", uidroot.c_str(), 1);
    sprintf(seriesUID, "%s.10.%u", uidroot.c_str(), 1);
    sprintf(instanceUID, "%s.100.%u", uidroot.c_str(), 1);

    IODReference* ref = new IODReference();
    ref->m_StudyInstanceUID = studyUID;
    ref->m_SeriesInstanceUID = seriesUID;
    ref->m_SOPClassUID = UID_MRImageStorage;
    ref->m_SOPInstanceUID = instanceUID;

    IODReferences references;
    if (!references.add(ref))
      {
      delete ref;
      std::cerr << "Error creating reference" << std::endl;
      return tract;
      }

    /* creation information */
    IODEnhGeneralEquipmentModule::EquipmentInfo eq;
    eq.m_Manufacturer           = TIO_MANUFACTURER;
    eq.m_ManufacturerModelName  = TIO_MANUFACTURER_MODELNAME;
    eq.m_DeviceSerialNumber     = TIO_DEVICESERIALNUMBER;
    eq.m_SoftwareVersions       = TIO_SOFTWAREVERSIONS;

    result = TrcTractographyResults::create(
               *contentID,
               "20160329", // TODO actual date
               "124200",   // TODO actual time
               eq,
               references,
               p_tract);
    }

  if (result.good())
    {
    /* add study information */
    tract->getPatient().setPatientBirthDate("19010101");
    tract->getPatient().setPatientName("Flintstone^Fred");
    tract->getPatient().setPatientSex("M");
    tract->getPatient().setPatientID("0057");
    tract->getStudy().setStudyDate("20160301");
    tract->getStudy().setStudyDescription("Test study");
    tract->getSeries().setSeriesDate("20160305");
    tract->getSeries().setSeriesDescription("Very small tractography series");
    tract->getFrameOfReference().setFrameOfReferenceUID("5.6.7.8");
    }

  if (result.good())
    {
    tract.reset(p_tract);
    }
  else
    {
    delete contentID;
    contentID = NULL;
    delete p_tract;
    p_tract = NULL;
    }
  return tract;
}

/* ------------------------------------------------------------------------- */

SP<TrcTractographyResults> create_dicom(std::vector<std::string> files)
{
  OFCondition result;
  SP<TrcTractographyResults> tract;

  ContentIdentificationMacro* contentID = NULL;
  result = ContentIdentificationMacro::create("1", "TRACT_TEST_LABEL",
                                              "Tractography from VTK file",
                                              "TractIO",
                                              contentID);

  if (result.bad())
    return tract;

  // TODO: auto-convert
  OFVector<OFString> of_files;
  for (auto f: files)
    of_files.push_back(f.c_str());

  std::cout << "Reference file: " << of_files[0] << std::endl;
  IODReferences references;
  if ( references.addFromFiles(of_files) != files.size() )
    return tract;

  IODEnhGeneralEquipmentModule::EquipmentInfo eq;
  eq.m_Manufacturer           = TIO_MANUFACTURER;
  eq.m_ManufacturerModelName  = TIO_MANUFACTURER_MODELNAME;
  eq.m_DeviceSerialNumber     = TIO_DEVICESERIALNUMBER;
  eq.m_SoftwareVersions       = TIO_SOFTWAREVERSIONS;

  TrcTractographyResults *p_tract;
  result = TrcTractographyResults::create(
             *contentID,
             "20160329", // TODO actual date
             "124200",   // TODO actual time
             eq,
             references,
             p_tract);


  if (result.good())
    {
    result = p_tract->importPatientStudyFoR(files[0].c_str(),
                                            true, /* usePatient*/
                                            true, /* useStudy */
                                            true, /* useSeries */
                                            true  /* useFoR = use Frame of Reference */
                                            );    /*          i.e. patient space     */
    }

  if (result.good())
    {
    // take pointer
    tract.reset(p_tract);
    }
  else
    {
    delete contentID;
    contentID = NULL;
    delete p_tract;
    p_tract = NULL;
    }

  return tract;
}

vtkSP<vtkPolyData> load_polydata(std::string polydata_file)
{
  // TODO: handle .vtk files
  auto reader = vtkSP<vtkXMLPolyDataReader>::New();

  reader->SetFileName(polydata_file.c_str());
  reader->Update();

  if (!reader->GetOutput())
    {
    std::cerr << "Error: failed to read VTK file \"" << polydata_file << "\"" << std::endl;
    return vtkSP<vtkPolyData>();
    }

  return vtkSP<vtkPolyData>(reader->GetOutput());
}

int insert_polydata_tracts(TrcTrackSet *trackset,
                           vtkSP<vtkPolyData> polydata)
{
  OFCondition result;
  vtkPoints *points = polydata->GetPoints();
  vtkFloatArray *pointdata = vtkFloatArray::SafeDownCast(points->GetData());


  if (!pointdata)
    {
    std::cerr << "Missing point data for polydata!" << std::endl;
    return 1;
    }


  /* add points for each track */
  for (int i = 0; i < polydata->GetNumberOfCells(); i++)
    {
    vtkCell *cell = polydata->GetCell(i);
    vtkIdType numPoints   = cell->GetNumberOfPoints();
    vtkIdType cellStartIdx = cell->GetPointId(0);
    TrcTrack *track_dontcare = NULL;

    // TODO: verify contiguity assumption is correct in general.
    float *cellPointData = (float*)pointdata->GetVoidPointer(cellStartIdx * 3);
    result = trackset->addTrack(cellPointData,
                                numPoints,
                                NULL, 0,
                                track_dontcare);

    if (result.bad())
      {
      std::cerr << "Error adding polydata track to trackset." << std::endl;
      return 1;
      }
    }
  return 0;
}

int add_tracts(SP<TrcTractographyResults> dcmtract,
               vtkSP<vtkPolyData> polydata,
               std::string label = "TRACKSET")
{
  assert(polydata);

  OFCondition result;

  CodeWithModifiers anatomyCode;
  anatomyCode.set("T-A0095", "SRT", "White matter of brain and spinal cord");
  CodeSequenceMacro diffusionModelCode("113231", "DCM", "Single Tensor");
  CodeSequenceMacro algorithmCode("113211", "DCM", "Deterministic");

  char buf_label[100];
  sprintf(buf_label, "%s", label.c_str());

  TrcTrackSet *trackset = NULL;
  result = dcmtract->addTrackSet(
    buf_label,
    buf_label,
    anatomyCode,
    diffusionModelCode,
    algorithmCode,
    trackset);

  if (result.bad())
    return 1;

  /* required by standard */
  trackset->setRecommendedDisplayCIELabValue(1,1,1);

  return insert_polydata_tracts(trackset, polydata);
}

int main(int argc, char *argv[])
{
  if (argc < 4)
    {
    std::cerr << "usage: vtkconvert INPUTFILE.vtk/.vtp OUTPUTTRACTNAME.dcm [dicom reference files]" << std::endl;
    return EXIT_FAILURE;
    }

  int result = 0;
  std::vector<std::string> ref_files;
  std::string polydata_file, output_file;
  polydata_file = std::string(argv[1]);
  output_file = std::string(argv[2]);

  // collect reference DICOM files
  for (int i = 3; i < argc; i++)
    ref_files.push_back(std::string(argv[i]));

  // read polydata file
  vtkSP<vtkPolyData> polydata = load_polydata(polydata_file);

  if (!polydata)
    {
    std::cerr << "Error: failed to load polydata" << std::endl;
    return EXIT_FAILURE;
    }

  OFLog::configure(OFLogger::TRACE_LOG_LEVEL);

  // create DICOM object associated to reference files
  SP<TrcTractographyResults> dicom = create_dicom(ref_files);
  //SP<TrcTractographyResults> dicom = create_dummy_dicom();
  if (!dicom)
    {
    std::cerr << "Error: Tract DICOM object creation failed!" << std::endl;
    return EXIT_FAILURE;
    }

  // add tracks from polydata
  if ( add_tracts(dicom, polydata) != 0)
    {
    std::cerr << "Error: Failed to add tracks from polydata." << std::endl;
    return EXIT_FAILURE;
    }

  // write DICOM to disk
  OFCondition ofresult;
  ofresult = dicom->saveFile(output_file.c_str());
  if (ofresult.bad())
    {
    std::cerr << "Error: Failed to save tractography DICOM file." << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
