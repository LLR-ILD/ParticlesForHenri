#include "lcio.h"
#include <stdio.h>

#include "IO/LCReader.h"
#include "tools.h"
#include "EVENT/LCRunHeader.h" 

#include "EVENT/SimCalorimeterHit.h" 
#include "EVENT/CalorimeterHit.h" 
#include "EVENT/RawCalorimeterHit.h" 
// #include "EVENT/SimTrackerHit.h" 

#include "UTIL/CellIDDecoder.h"

#include <cstdlib>

#include <sys/types.h>
#include <sys/stat.h>

using namespace std ;
using namespace lcio ;
using namespace convert ;

/** dump the given event to screen
 */

int main(int argc, char** argv ){


  char* FILEN;
  string OutDirName;
  int runNumber=0 ;
  int firstEvtNumber=0 ;
  int lastEvtNumber=0 ;

  // read file name from command line (only argument) 
  if( (argc != 5 ) && (argc != 6 )){

    cout << " usage: lcio2ascii outdir filename runNum evtNum " << endl ;
    cout << "    or: lcio2ascii outdir filename runNum firstEvtNum lastEvtNum" 
		<< endl ;
    cout << "  where the first dumps the event with the specified run and event number" << endl ;
    cout << "  and the second dumps the events with the specified run number "
	 << "and event numbers from firstEvtNum to lastEvtNum included"  << endl ;

    exit(1) ;
  }
  
  OutDirName = argv[1] ;
  FILEN = argv[2] ;

  bool dumpNthEvent( argc == 5 ) ;
 

  runNumber = atoi( argv[3] ) ;
  firstEvtNumber=atoi( argv[4] ) ;

  if( dumpNthEvent ) {

    lastEvtNumber=firstEvtNumber;

  }else{

    lastEvtNumber=atoi( argv[5] ) ;
  }
 
  if( mkdir(OutDirName.c_str(),
               S_IRUSR|S_IWUSR|S_IXUSR|
               S_IRGRP|S_IXGRP|
               S_IROTH|S_IXOTH) == -1) {

  	cout << "WARNING: Using existing output directory " << OutDirName << 
		" \nDo you wish to continue [y/n] ?"  <<endl;
	char response = getchar();
	if((response == 'n') || (response == 'N'))
		exit(0);
  }	
  else
  	cout << "Creating the output directory " << OutDirName << endl;

  // set the default encoding for cellid's according to the old Mokka convention
  CellIDDecoder<SimCalorimeterHit>::setDefaultEncoding("M:3,S-1:3,I:9,J:9,K-1:6") ;
  CellIDDecoder<CalorimeterHit>::setDefaultEncoding("M:3,S-1:3,I:9,J:9,K-1:6") ;
  CellIDDecoder<RawCalorimeterHit>::setDefaultEncoding("M:3,S-1:3,I:9,J:9,K-1:6") ;

  LCReader* lcReader ;
  lcReader = LCFactory::getInstance()->createLCReader(LCReader::directAccess) ;
  
  LCEvent* evt(0) ;

  try{

 lcReader->open( FILEN ) ;
       
 for(int evtNumber=firstEvtNumber; evtNumber<=lastEvtNumber;evtNumber++)
 {  
    
     evt = lcReader->readEvent(runNumber,  evtNumber) ; 
     if( !evt  ){

       cout << " couldn't find event " << evtNumber << " - run " << runNumber 
	      << " in file " << FILEN << endl ;    
       exit(1) ;
     }

     tools::dumpEventDetailed( evt, OutDirName ) ;
     
 }
     
     lcReader->close() ;
     
   }
   catch( IOException& e) {
     cout << e.what() << endl ;
     exit(1) ;
   }
   return 0 ;
}

