
#g++ *.cpp  -o qget `pkg-config --cflags --libs ACE`

#static
#g++ -static *.cpp  -o qget  -I/usr/local/include  -L/usr/local/lib -lACE -lpthread -ldl -lrt

#for large file support,we need to add -D_FILE_OFFSET_BITS=64 to CPPFLAGS, and build ACE is also needed.
#CPPFLAGS+=-D_FILE_OFFSET_BITS=64
#CXXFLAGS+=-D_FILE_OFFSET_BITS=64
g++ $CXXFLAGS *.cpp  -o qget `pkg-config --cflags --libs ACE` -static -lpthread



#for use gcc oprofile
#generate
#g++  -fprofile-generate *.cpp  -o qget `pkg-config --cflags --libs ACE` -static -lpthread

#run
#./qget http://download.dre.vanderbilt.edu/previous_versions/ACE-6.0.5.tar.bz2 5

#use
#g++  -fprofile-use *.cpp  -o qget `pkg-config --cflags --libs ACE` -static -lpthread


#for ace building, set system's environment variable "export CXXFLAGS+=-D_FILE_OFFSET_BITS=64", then configure,make,make install.
