add_library(u24_pe BoundaryFixFunction2.cpp
                   BoundaryFixFunction.cpp
                   clipper.cpp
) 

install(FILES clipper.h BoundaryFix.h DESTINATION include)
