#include "vtkNew.h"
#include "vtkSmartPointer.h"


#define vtkSP vtkSmartPointer

//=============================================================================
// make_unique per N3656
// TODO ifdef for C++14
namespace std {
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args )
{
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}

} // end namespace std

enum TIO_STATUS {
    TIO_FAIL = -1,
    TIO_SUCCESS = 0
};
