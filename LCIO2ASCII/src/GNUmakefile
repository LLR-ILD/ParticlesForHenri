# check for private compiler definition
ifdef MY_CXX
  CXX = $(MY_CXX)
else
  CXX = g++
endif

AR = ar

SIOINSTALL = $(LCIOBASE)/sio
INSTALL = $(LCIOBASE)/lib
BINDIR = $(LCIO2ASCIIHOME)/bin
LCIOINCLUDE := -I $(LCIOBASE)/src/cpp/include 
INCLUDE =  $(LCIOINCLUDE) -I$(SIOINSTALL)/include
CPPFLAGS = $(LCIOCPPFLAGS) $(INCLUDE) 

LIBS = -L$(INSTALL) -L$(SIOINSTALL)/lib -llcio -lsio -lz

BINS =  $(BINDIR)/lcio2ascii

bin: $(BINS)


$(BINDIR)/lcio2ascii: lcio2ascii.cc tools.cc $(INSTALL)/liblcio.so
	$(CXX) -o $(BINDIR)/lcio2ascii $(CPPFLAGS) lcio2ascii.cc tools.cc $(LIBS) 


clean:
	rm -f $(BINS)


