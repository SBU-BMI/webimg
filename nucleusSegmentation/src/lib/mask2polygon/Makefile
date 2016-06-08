INC=-I/cm/shared/apps/opencv/opencv/include
LIB=-L/cm/shared/apps/opencv/opencv/lib -lopencv_highgui -lopencv_imgproc


all: processMask

processMask: processMask.cpp MaskToPoly.o
	g++ -o processMask processMask.cpp MaskToPoly.o ${LIB}

MaskToPoly.o: MaskToPoly.cpp MaskToPoly.h
	g++ ${INC} -c MaskToPoly.cpp 

clean:
	\rm -f processMask MaskToPoly.o

