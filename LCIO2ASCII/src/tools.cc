#include "tools.h"
#include "UTIL/Operators.h"

#include "EVENT/LCCollection.h"
#include "EVENT/SimCalorimeterHit.h"
#include "EVENT/CalorimeterHit.h"
#include "EVENT/RawCalorimeterHit.h"
#include "EVENT/SimTrackerHit.h"
#include "EVENT/TPCHit.h"
#include "EVENT/TrackerRawData.h"
#include "EVENT/TrackerData.h"
#include "EVENT/TrackerPulse.h"
#include "EVENT/LCIO.h"
#include "EVENT/MCParticle.h"
#include "EVENT/LCFloatVec.h"
#include "EVENT/LCIntVec.h"
#include "IMPL/LCFlagImpl.h"
#include "EVENT/Track.h"
#include "EVENT/Cluster.h"
#include "EVENT/ReconstructedParticle.h"
#include "EVENT/Vertex.h"
#include "EVENT/LCGenericObject.h"

#include "EVENT/LCRelation.h"
#include "LCIOSTLTypes.h"

#include <cstdlib>
#include <iostream>

// #ifdef USE_CLHEP
// #include "UTIL/LCFourVector.h"
// #endif
#include "UTIL/LCObjectHandle.h"
#include "UTIL/LCTime.h"
#include "UTIL/CellIDDecoder.h"
#include "UTIL/PIDHandler.h"
#include <map>
#include <set>
#include <cstdio>


typedef std::vector<std::string> LCStrVec ;

typedef std::map< MCParticle*, int > PointerToIndexMap ;
PointerToIndexMap p2i_map ;

using namespace std ;
using namespace EVENT ;
using namespace IMPL ;

LCCollection* mcParticleCollection = 0;

namespace convert {

  static int MAX_HITS = 1000 ;



  void tools::dumpEvent(const LCEvent* evt){
    
    // the event:
    cout << "///////////////////////////////////" << endl;
    cout << "EVENT: " << evt->getEventNumber() << endl
         << "RUN: " << evt->getRunNumber() << endl
         << "DETECTOR: " << evt->getDetectorName() << endl
         << "COLLECTIONS: (see below)" << endl;
    cout << "///////////////////////////////////" << endl << endl;
     
    cout << "---------------------------------------------------------------------------" << endl;
    cout.width(30); cout << left << "COLLECTION NAME";
    cout.width(25); cout << left << "COLLECTION TYPE";
    cout.width(20); cout << left << "NUMBER OF ELEMENTS" << endl;
    cout << "===========================================================================" << endl;
    
    const std::vector< std::string >* strVec = evt->getCollectionNames() ;

    // loop over collections:
    for( std::vector< std::string >::const_iterator name = strVec->begin() ; name != strVec->end() ; name++){
    
      cout.width(30); cout << left << *name;
      cout.width(25); cout << left << evt->getCollection( *name )->getTypeName();
      cout.width(9); cout << right << evt->getCollection( *name )->getNumberOfElements();
      cout << endl;
      //cout << "---------------------------------------------------------------------------" << endl;
    }
    cout << "---------------------------------------------------------------------------" << endl;
    cout << endl << endl << endl;
  }

  
  void tools::dumpEventDetailed(const LCEvent* evt, string OutDirName){
    
    // the event:
    cout << endl 
	 << "============================================================================" << endl ;
    cout << "        Event  : " << evt->getEventNumber() 
	 << " - run:  "         << evt->getRunNumber()
         << " - timestamp "     << evt->getTimeStamp()   
         << " - weight "        << evt->getWeight()   
	 << endl ;
    cout << "============================================================================" << endl ;    

    LCTime evtTime( evt->getTimeStamp() ) ;
    cout << " date:      "      << evtTime.getDateString() << endl ;     
    cout << " detector : "      << evt->getDetectorName() << endl ;
    
    cout << " event parameters: " << endl ; 

    printParameters( evt->getParameters() ) ;

/*
PointerToIndexMap::iterator it;
for(it = p2i_map.begin(); it != p2i_map.end(); it++)
{
	if(it->first) { 
		//delete (it->first);
	}
}
*/

p2i_map.clear();

    const std::vector< std::string >* strVec = evt->getCollectionNames() ;

    // loop over all collections:
    std::vector< std::string >::const_iterator name ;
    
    LCCollection* col = 0;

    for( name = strVec->begin() ; name != strVec->end() ; name++)
    {
    
      col = evt->getCollection( *name ) ;
      if( col->getTypeName() == LCIO::MCPARTICLE )
      {
	mcParticleCollection = col;
        char buffer[40];
        string outFileName;
        FILE *outFile = NULL;

	sprintf(buffer,"/event%.6d.kin",evt->getEventNumber());
        outFileName = OutDirName+buffer;
          if((outFile=fopen(outFileName.c_str(),"w"))==NULL)
            {
              perror(outFileName.c_str());
              exit(1);
            }

	printMCParticles( col, outFile ) ;
	fclose(outFile);
        break;
      }
    }

    for( name = strVec->begin() ; name != strVec->end() ; name++){
    
      col = evt->getCollection( *name ) ;
    
      cout << endl 
	   << " collection name : " << *name 
	   << endl 
	   << " parameters: " << endl ;

           printParameters( col->getParameters() ) ;


        char buffer[40];
        string outFileName;
        FILE *outFile = NULL;


      // call the detailed print functions depending on type name
      if( evt->getCollection( *name )->getTypeName() == LCIO::SIMTRACKERHIT ){

	string detectorName;

	if((name->find("TPC")) != string::npos)
        {
		detectorName = "tpc";
	}
	else if((name->find("ETD")) != string::npos)
        {
		detectorName = "etd";
	}
	else if((name->find("FTD")) != string::npos)
        {
		detectorName = "ftd";
	}
	else if((name->find("SET")) != string::npos)
        {
		detectorName = "set";
	}
	else if((name->find("SIT")) != string::npos)
        {
		detectorName = "sit";
	}
	else if((name->find("VXD")) != string::npos)
        {
		detectorName = "vxd";
	}
	else 
		continue;

	  
	sprintf(buffer,"/%s%.6d.hits",detectorName.c_str(), evt->getEventNumber());
        outFileName = OutDirName+buffer;

        if((outFile=fopen(outFileName.c_str(),"a"))==NULL)
        {
              perror(outFileName.c_str());
              exit(1);
        }

	printSimTrackerHits( col, outFile) ;
	fclose(outFile);

      }
      else if( evt->getCollection( *name )->getTypeName() == LCIO::SIMCALORIMETERHIT ){

        int pP;
	string detectorName;

	if((name->find("Ecal")) != string::npos) 
        {
        	pP = 2;//ECALBARREL
		if((name->find("BarrelPreShower")) != string::npos)
			pP = -pP;//PreShowerCollection
			
		if(((name->find("Endcap")) != string::npos) ||
			((name->find("EndCap")) != string::npos))
			//includes EndcapRings too
		{
			pP = 3;//ECALENDCAPPLUS

		 	if((name->find("PreShower")) != string::npos)
				pP = -pP;//PreShowerCollection
		}

		detectorName = "ecal";
	}
	else if((name->find("LHcal")) != string::npos)
        {
		pP = 3;//ECALENDCAPPLUS
		if((name->find("PreShower")) != string::npos)
				pP = -pP;//PreShowerCollection
		detectorName = "lhcal";
	}
	else if((name->find("Hcal")) != string::npos)
        {
        	pP = 5;//HCALBARREL
		if(((name->find("Endcap")) != string::npos) ||
			((name->find("EndCap")) != string::npos))
			pP = 6;//HCALENDCAPPLUS

		detectorName = "hcal";
	}
	else if((name->find("Muon")) != string::npos)
        {
        	pP = 1;//BARREL
		if(((name->find("Endcap")) != string::npos) ||
			((name->find("EndCap")) != string::npos))
			pP = 2;//ENDCAP

		detectorName = "yoke";
	}
	else if((name->find("BeamCal")) != string::npos)
        {
        	pP = 0;
		detectorName = "BeamCal";
	}
	else if((name->find("LumiCal")) != string::npos)
        {
		pP = 0;
		detectorName = "LumiCal";
	}
	else 
		continue;

	sprintf(buffer,"/%s%.6d.hits",detectorName.c_str(), evt->getEventNumber());
        outFileName = OutDirName+buffer;

        if((outFile=fopen(outFileName.c_str(),"a"))==NULL)
        {
              perror(outFileName.c_str());
              exit(1);
        }
	printSimCalorimeterHits( col, pP, outFile ) ;
	fclose(outFile);
    }
  }


}

  void tools::printTracks(const EVENT::LCCollection* col ){
    if( col->getTypeName() != LCIO::TRACK ){
      
      cout << " collection not of type " << LCIO::TRACK << endl ;
      return ;
    }
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::TRACK << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
    
    printParameters( col->getParameters() ) ;

    LCFlagImpl flag( col->getFlag() ) ;
    cout << "     LCIO::TRBIT_HITS : " << flag.bitSet( LCIO::TRBIT_HITS ) << endl ;

    int nTracks =  col->getNumberOfElements() ;
    int nPrint = nTracks > MAX_HITS ? MAX_HITS : nTracks ;
    
    std::cout << endl
	      << " [   id   ] |   type   |    d0    |  phi     | omega    |    z0     | tan lambda|   reference point(x,y,z)        |    dEdx  |  dEdxErr |   chi2   "
	      << endl 
	      << "------------|----------|----------|----------|----------|-----------|-----------|---------------------------------|----------|----------|-------- "
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      Track* trk = 
      	dynamic_cast<Track*>( col->getElementAt( i ) ) ;
      
      printf(" [%8.8x] | %8.8x | %4.2e | %4.2e | %4.2e | %5.3e | %5.3e | (%5.3e,%5.3e,%5.3e) | %4.2e | %4.2e | %4.2e \n"
	     , trk->id()
	     , trk->getType() 
	     , trk->getD0() 
	     , trk->getPhi()
	     , trk->getOmega() 
	     , trk->getZ0() 
	     , trk->getTanLambda()
// 	     , trk->getMomentum() 
// 	     , trk->getTheta() 
// 	     , trk->getPhi()
// 	     , trk->getD0() 
// 	     , trk->getZ0() 
	     , trk->getReferencePoint()[0]
	     , trk->getReferencePoint()[1]
	     , trk->getReferencePoint()[2]
	     , trk->getdEdx() 
	     , trk->getdEdxError() 
	     , trk->getChi2() 
	     ) ;
      cout << " errors: " ;
      for(int l=0;l<15;l++){
	printf("%4.2e, ", trk->getCovMatrix()[l] ) ; 
      }
      cout << endl << " tracks(id): " ;
      const TrackVec& tracks = trk->getTracks() ;

      for(unsigned int l=0;l<tracks.size();l++){
	if( tracks[l] != 0  ) 
	  printf("[%8.8x], ",  tracks[l]->id() ) ; 
	else
	  printf("[%8.8x], ",  0 ) ; 
      }

      cout << endl ;
      if( flag.bitSet( LCIO::TRBIT_HITS ) ) {
	cout << " hits ->" ;
	// 	const StringVec& hitColNames = trk->getHitCollectionNames() ;
	// 	for(unsigned int j=0;j<hitColNames.size();j++){
	// 	  cout << " " << hitColNames[j] << ": " ;
	// 	  const IntVec& hits = trk->getHitIndicesForCollection( hitColNames[j] ) ;
	// 	  for(unsigned int k=0;k<hits.size();k++){
	// 	    cout << hits[k] <<"," ;
	// 	  }
	// 	}
	const TrackerHitVec& hits= trk->getTrackerHits() ; 
	for(unsigned int k=0;k<hits.size();k++){
	  printf("[%8.8x] ", hits[k]->id() ) ;
	  //	  cout << "[" << hits[k]->id() <<"] " ;
	}
	cout << endl  ;
      }
      cout << " radius of innermost hit " << trk->getRadiusOfInnermostHit() << " / mm , "   
	<< " subdetector Hit numbers : " ;
      for( unsigned int i=0 ; i< trk->getSubdetectorHitNumbers().size() ; i++) {
	cout << trk->getSubdetectorHitNumbers()[i] << ", " ;
      }
      cout << endl ;
//       cout << trk->getType()     << " | "
// 	   << trk->getMomentum() << " | "
// 	   << trk->getTheta()     << " | "
// 	   << trk->getPhi()     << " | "
// 	   << trk->getD0()     << " | "
// 	   << trk->getZ0()     << " | "
// 	   << endl ;

      cout << "------------|----------|----------|----------|----------|-----------|-----------|---------------------------------|----------|----------|-------- "
	   << endl ;
    }
  }
  
  void tools::printSimTrackerHits(const EVENT::LCCollection* col, FILE *hitFile)
  {
    
    if( col->getTypeName() != LCIO::SIMTRACKERHIT ){
      
      cout << " collection not of type " << LCIO::SIMTRACKERHIT << endl ;
      return ;
    }
    
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::SIMTRACKERHIT << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;

    printParameters( col->getParameters() ) ;
    
    LCFlagImpl flag( col->getFlag() ) ;
    cout << "     LCIO::THBIT_BARREL : " << flag.bitSet( LCIO::THBIT_BARREL ) << endl ;
    cout << "     LCIO::THBIT_MOMENTUM : " << flag.bitSet( LCIO::THBIT_MOMENTUM ) << endl ;
    
    bool pStored = flag.bitSet( LCIO::THBIT_MOMENTUM ) ;

    int nHits =  col->getNumberOfElements() ;
    //int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;
    int nPrint = nHits;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      SimTrackerHit* hit = 
	dynamic_cast<SimTrackerHit*>( col->getElementAt( i ) ) ;
      
   //To avoid segfault due to a bug somewhere
   int PID = 0, PDG = 0;
   if(hit->getMCParticle()){
	      PID = (int)(p2i_map[hit->getMCParticle()])+1;
	      PDG = (int)(hit->getMCParticle()->getPDG());
   }
   else
	cout << "Null pointer to MCParticle in hit no " << i << 
		" saving PID = 0, PDG = 0 " << endl;

   if( pStored ) 
      fprintf(hitFile,"%d %7.2f %7.2f %7.2f %7.2f %7.2f %7.2f %d %d %15e\n",
              (int)(hit->getCellID()),
	      (double)(hit->getPosition()[0]),
	      (double)(hit->getPosition()[1]),
	      (double)(hit->getPosition()[2]),
	      (double)(hit->getMomentum()[0]),
	      (double)(hit->getMomentum()[1]),
	      (double)(hit->getMomentum()[2]),
	      PID,
	      PDG,
              (double)(hit->getdEdx())
              );
   else
      fprintf(hitFile,"%d %7.2f %7.2f %7.2f %d %d %15e\n",
              (int)(hit->getCellID()),
	      (double)(hit->getPosition()[0]),
	      (double)(hit->getPosition()[1]),
	      (double)(hit->getPosition()[2]),
	      PID,
	      PDG,
              (double)(hit->getdEdx())
              );
    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
    
  }

  void tools::printTrackerHits(const EVENT::LCCollection* col ){
    
    if( col->getTypeName() != LCIO::TRACKERHIT ){
      
      cout << " collection not of type " << LCIO::TRACKERHIT << endl ;
      return ;
    }
    
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::TRACKERHIT << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
    
    printParameters( col->getParameters() ) ;

    LCFlagImpl flag( col->getFlag() ) ;
    cout << "     LCIO::THBIT_BARREL : " << flag.bitSet( LCIO::THBIT_BARREL ) << endl ;
    
    
    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;
    
    std::cout << endl
	      << " [   id   ] | position (x,y,z)                | dEdx      | time     |  type" 
	      << endl 
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      TrackerHit* hit = 
	dynamic_cast<TrackerHit*>( col->getElementAt( i ) ) ;
      
      //       int rawHitId = 0 ;
      //       if( hit->getRawDataHit() != 0 ) 
      // 	rawHitId = hit->getRawDataHit()->id()  ;
      
      printf(" [%8.8x] | (%5.3e,%5.3e,%5.3e) | %5.3e | %5.3e | [%d] \n" 
	     , hit->id() 
	     , hit->getPosition()[0] 
	     , hit->getPosition()[1] 
	     , hit->getPosition()[2] 
	     , hit->getdEdx() 
	     , hit->getTime() 
	     , hit->getType() 
	     ) ;
      const LCObjectVec& rawHits = hit->getRawHits() ;
      cout << "    rawHits: " ;
      try{
	for( unsigned j=0 ; j < rawHits.size() ; j++ ) {
	  cout << hex << "[" << rawHits[j]->id() << "], " <<  dec ;
	}
      }catch(std::exception& e){}
      cout << endl ;
    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
    
  }
  
void tools::printTrackerRawData(const EVENT::LCCollection* col ) {

    if( col->getTypeName() != LCIO::TRACKERRAWDATA ){

      cout << " collection not of type " << LCIO::TRACKERRAWDATA << endl ;
      return ;
    }
    
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::TRACKERRAWDATA << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;

    LCFlagImpl flag( col->getFlag() ) ;
    cout << "     LCIO::TRAWBIT_ID1    : " << flag.bitSet( LCIO::TRAWBIT_ID1 ) << endl ;
    
    printParameters( col->getParameters() ) ;

    CellIDDecoder<TrackerRawData> id( col ) ;

    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;
    
//     std::cout << endl
// 	      << " [   id   ] |  cellid0 |  cellid1 |   time    | chargeADC "
// 	      << endl 
// 	      << endl ;
    
//     for( int i=0 ; i< nPrint ; i++ ){
      
//       TrackerRawData* hit = 
// 	dynamic_cast<TrackerRawData*>( col->getElementAt( i ) ) ;
      
//       printf(" [%8.8x] | %8.8x | %8.8x | %10d | " 
// 	     , hit->id() 
// 	     , hit->getCellID0()                 
// 	     , hit->getCellID1()                 
// 	     , hit->getTime ()  
// 	     ) ;
//       const ShortVec& charge = hit->getADCValues() ;
//       for( unsigned j=0 ; j < charge.size() ; j++ ) {
// 	cout << charge[j] << "," ;
//       }
//       cout << endl ;
//       std::cout << "        id-fields: (" << id( hit ).valueString() << ")" << std::endl ; 

//     }
    std::cout << endl
	      << " [   id   ] |  cellid0 |  cellid1 |   time    | cellid-fields: |"
	      << endl 
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      TrackerRawData* hit = 
	dynamic_cast<TrackerRawData*>( col->getElementAt( i ) ) ;
      
      printf(" [%8.8x] | %8.8x | %8.8x | %10d | " 
	     , hit->id() 
	     , hit->getCellID0()                 
	     , hit->getCellID1()                 
	     , hit->getTime ()  
	     ) ;
      std::cout << " " << id( hit ).valueString() << "|" << std::endl ; 

      std::cout << " adc values: " ; 
      const ShortVec& charge = hit->getADCValues() ;
      for( unsigned j=0 ; j < charge.size() ; j++ ) {
	cout << charge[j] << "," ;
      }
      cout << endl ;

    }

    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
    
  }

  void tools::printTrackerData(const EVENT::LCCollection* col ) {

    if( col->getTypeName() != LCIO::TRACKERDATA ){

      cout << " collection not of type " << LCIO::TRACKERDATA << endl ;
      return ;
    }
    
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::TRACKERDATA << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;

    LCFlagImpl flag( col->getFlag() ) ;
    cout << "     LCIO::TRAWBIT_ID1    : " << flag.bitSet( LCIO::TRAWBIT_ID1 ) << endl ;
    
    printParameters( col->getParameters() ) ;

//     LCFlagImpl flag( col->getFlag() ) ;
//     cout << "     LCIO::THBIT_BARREL : " << flag.bitSet( LCIO::THBIT_BARREL ) << endl ;
    
    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;
    
    CellIDDecoder<TrackerData> id( col ) ;

    std::cout << endl
	      << " [   id   ] |  cellid0 |  cellid1 |   time    | cellid-fields: |"
	      << endl 
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      TrackerData* hit = 
	dynamic_cast<TrackerData*>( col->getElementAt( i ) ) ;
      
      printf(" [%8.8x] | %8.8x | %8.8x | %5.3f  | " 
	     , hit->id() 
	     , hit->getCellID0()                 
	     , hit->getCellID1()                 
	     , hit->getTime ()  
	     ) ;
      std::cout << " " << id( hit ).valueString() << "|" << std::endl ; 

      std::cout << "chargeADC : " ;
      const FloatVec& charge = hit->getChargeValues() ;
      for( unsigned j=0 ; j < charge.size() ; j++ ) {
	cout << charge[j] << "," ;
      }
      cout << endl ;
    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;

  }

  void tools::printTrackerPulse(const EVENT::LCCollection* col ) {

    if( col->getTypeName() != LCIO::TRACKERPULSE ){

      cout << " collection not of type " << LCIO::TRACKERPULSE << endl ;
      return ;
    }
    
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::TRACKERPULSE << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;

    LCFlagImpl flag( col->getFlag() ) ;
    cout << "     LCIO::TRAWBIT_ID1    : " << flag.bitSet( LCIO::TRAWBIT_ID1 ) << endl ;
    
    printParameters( col->getParameters() ) ;

    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;
    
    CellIDDecoder<TrackerPulse> id( col ) ;

    std::cout << endl
	      << " [   id   ] | cellid0  | cellid1  | time | charge | quality  |corr.Data | cellid-fields: |"
 	      << endl 
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      TrackerPulse* hit = 
	dynamic_cast<TrackerPulse*>( col->getElementAt( i ) ) ;
      
      printf(" [%8.8x] | %8.8x | %8.8x | %5.3f | %5.3f | %8.8x | " 
	     , hit->id() 
	     , hit->getCellID0()                 
	     , hit->getCellID1()                 
	     , hit->getTime()  
	     , hit->getCharge()  
	     , hit->getQuality()  
	     ) ;

      TrackerData* corr =  hit->getTrackerData() ;
//       if( corr != 0 )
// 	std::cout << corr->id() 

      printf("[%8.8x] ",   ( corr != 0 ? corr->id() : 0 ) ) ;

      std::cout << " " << id( hit ).valueString() << "|" << std::endl ; 

    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
    
  }

  void tools::printTPCHits(const EVENT::LCCollection* col ) {
    
    if( col->getTypeName() != LCIO::TPCHIT ){
      
      cout << " collection not of type " << LCIO::TPCHIT << endl ;
      return ;
    }
    
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::TPCHIT << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
    
    printParameters( col->getParameters() ) ;

    LCFlagImpl flag( col->getFlag() ) ;
    cout << "  -> LCIO::TPCBIT_RAW   : " << flag.bitSet( LCIO::TPCBIT_RAW ) << endl ;
    cout << "  -> LCIO::TPCBIT_NO_PTR   : " << flag.bitSet( LCIO::TPCBIT_NO_PTR ) << endl ;
    
    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;
    
    std::cout << endl
	      << " [   id   ]  | cellId0 | time | charge | quality " 
	      << endl << "  -> raw data (bytes) : "
	      << endl 
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      TPCHit* hit = 
	dynamic_cast<TPCHit*>( col->getElementAt( i ) ) ;
      
      int id0 = hit->getCellID()  ;
      //      std::cout << hit->getCellID() << " | " 
      printf( " [%8.8x] |", hit->id() ) ;
      std::cout	<< ((id0& 0xff000000)>>24) << "/" 
		<< ((id0& 0x00ff0000)>>16) << "/" 
		<< ((id0& 0x0000ff00)>> 8) << "/" 
		<< ((id0& 0x000000ff)>> 0) << " | "
		<< hit->getTime() << " | "
		<< hit->getCharge() << " | ["
		<< hit->getQuality() << "] "
		<< std::endl ;
      if( flag.bitSet( LCIO::TPCBIT_RAW ) ){

	int nWords = hit->getNRawDataWords() ;
	std::cout << "  ->  " ;

	for(int  j=0;j<nWords;j++){
	  int rawData = hit->getRawDataWord( j ) ;
	  std::cout << ((rawData& 0xff000000)>>24) << ", " 
		    << ((rawData& 0x00ff0000)>>16) << ", " 
		    << ((rawData& 0x0000ff00)>> 8) << ", " 
		    << ((rawData& 0x000000ff)>> 0)  ;
	  if( j < nWords-1) std::cout << ", " ;
	}

	std::cout << std::endl ;

      }
    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
  }

  void tools::printLCStrVecs( const EVENT::LCCollection* col ) {

    if( col->getTypeName() != LCIO::LCSTRVEC ){

      cout << " collection not of type " << LCIO::LCSTRVEC << endl ;
      return ;
    }

    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::LCSTRVEC << " collection (user extension) "
	 << "--------------- " << endl ;

    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
 
    printParameters( col->getParameters() ) ;

    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;

    std::cout << endl
	      << " element index: val0, val1, ..." 
	      << endl 
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      LCStrVec* vec =
	dynamic_cast<LCStrVec*>( col->getElementAt( i ) ) ;
      
      std::cout << i << ": " ;
      for(unsigned int j=0;j< vec->size();j++){

	std::cout << (*vec)[j] ;
	if( j<vec->size()-1) std::cout << ", " ;

	if( ! ( (j+1) % 10)  ) std::cout << endl << "     " ;
      }
      std::cout << std::endl ;
    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;

  }

  void tools::printLCFloatVecs( const EVENT::LCCollection* col ) {

    if( col->getTypeName() != LCIO::LCFLOATVEC ){

      cout << " collection not of type " << LCIO::LCFLOATVEC << endl ;
      return ;
    }

    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::LCFLOATVEC << " collection (user extension) "
	 << "--------------- " << endl ;

    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
 
    printParameters( col->getParameters() ) ;

    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;

    std::cout << endl
	      << " element index: val0, val1, ..." 
	      << endl 
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      LCFloatVec* vec = 
	dynamic_cast<LCFloatVec*>( col->getElementAt( i ) ) ;
      
      std::cout << i << ": " ;
      for(unsigned int j=0;j< vec->size();j++){

	std::cout << (*vec)[j] ;
	if( j<vec->size()-1) std::cout << ", " ;

	if( ! ( (j+1) % 10)  ) std::cout << endl << "     " ;
      }
      std::cout << std::endl ;
    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;

  }
  void tools::printLCIntVecs( const EVENT::LCCollection* col ) {

    if( col->getTypeName() != LCIO::LCINTVEC ){

      cout << " collection not of type " << LCIO::LCINTVEC << endl ;
      return ;
    }

    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::LCINTVEC << " collection (user extension) "
	 << "--------------- " << endl ;

    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
 
    printParameters( col->getParameters() ) ;

    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;

    std::cout << endl
	      << " element index: val0, val1, ..." 
	      << endl 
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      LCIntVec* vec = 
	dynamic_cast<LCIntVec*>( col->getElementAt( i ) ) ;
      
      std::cout << i << ": " ;
      for(unsigned int j=0;j< vec->size();j++){

	std::cout << (*vec)[j] ;
	if( j<vec->size()-1) std::cout << ", " ;

	if( ! ( (j+1) % 10)  ) std::cout << endl << "     " ;
      }
      std::cout << std::endl ;
    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;

  }

  void tools::printSimCalorimeterHits(const EVENT::LCCollection* col, int pP,
		FILE *hitFile ){
    
    if( col->getTypeName() != LCIO::SIMCALORIMETERHIT ){
      
      cout << " collection not of type " << LCIO::SIMCALORIMETERHIT << endl ;
      return ;
    }

    cout << endl 
	 << "--------------- " << "converting "  << LCIO::SIMCALORIMETERHIT << " collection "
	 << "--------------- " << endl ;

    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
 
    int dummyAsciiFlag = col->getFlag();

    LCFlagImpl flag( col->getFlag() ) ;
    cout << "  -> LCIO::CHBIT_LONG   : " << flag.bitSet( LCIO::CHBIT_LONG ) << endl ;
    cout << "     LCIO::CHBIT_BARREL : " << flag.bitSet( LCIO::CHBIT_BARREL ) << endl ;
    cout << "     LCIO::CHBIT_ID1   :  " << flag.bitSet( LCIO::CHBIT_ID1 ) << endl ;
    cout << "     LCIO::CHBIT_PDG    : " << flag.bitSet( LCIO::CHBIT_PDG ) << endl ;

    int nHits =  col->getNumberOfElements() ;
    //int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;
    int nPrint = nHits;

    
    CellIDDecoder<SimCalorimeterHit> idDecoder( col ) ; 

  for( int i=0 ; i< nPrint ; i++ )
  {
      
      SimCalorimeterHit* hit = 
	dynamic_cast<SimCalorimeterHit*>( col->getElementAt( i ) ) ;
      
      int id0 = hit->getCellID0() ;
      int id1 = hit->getCellID1() ;
	    
      if(((pP == 3) || (pP == 6)) && (hit->getPosition()[2] < 0.))
		pP -= 2;

      if((pP == -3) && (hit->getPosition()[2] < 0.)) //PreShower
		pP += 2;

      typedef std::map<int, double> mapPDGEdep;
      typedef std::map<int, mapPDGEdep*> mapPID2PDG;
      int pPID = -1;
      int pPDG = -1;
      double ePDG = -1;
      mapPID2PDG PIDList;
      mapPDGEdep* detailLines = 0;

      for(int k=0;k < hit->getNMCContributions();k++){

	try{

	 if (pPID != ((int)(p2i_map[hit->getParticleCont(k)])+1))
	 {
		//new primary

	 	if(pPID != -1) {

		//save last PDG contribution to old primary

		if(!detailLines->insert(
			std::make_pair(pPDG, ePDG) 
				      ).second  ) {
			(*detailLines)[pPDG] += ePDG;
		}


		   std::map<int, mapPDGEdep*>::value_type 
				elem(pPID, detailLines);

	           if(!(PIDList.insert(elem).second)  ) {
			std::cout << "ERROR: cannot insert " << pPID << 
				" line into PIDList Map " << std::endl;
			exit(1);
		    }

		}

		pPID = p2i_map[hit->getParticleCont( k)] + 1;
		ePDG = 0;

		if(flag.bitSet( LCIO::CHBIT_PDG ))
			pPDG = hit->getPDGCont( k);
		else 
			pPDG = 0;
			
         	detailLines = new mapPDGEdep;


	 }
	 else if(flag.bitSet( LCIO::CHBIT_PDG ) && (pPDG != hit->getPDGCont( k))) {
		//new PDG contribution to old primary

		if(!detailLines->insert(
			std::make_pair(pPDG, ePDG) 
				      ).second  ) {
			(*detailLines)[pPDG] += ePDG;
		}

		pPDG = hit->getPDGCont( k);
			
		ePDG = 0;
	 }

	 ePDG += hit->getEnergyCont( k);

	 if( k == (int)(hit->getNMCContributions() - 1)) {

		if(!detailLines->insert(
			std::make_pair(pPDG, ePDG) 
				      ).second  ) {
			(*detailLines)[pPDG] += ePDG;
		}

		std::map<int, mapPDGEdep*>::value_type 
				elem(pPID, detailLines);

	        if(!(PIDList.insert(elem).second)  ) {
			std::cout << "ERROR: cannot insert last " << pPID << 
				" line into PIDList Map " << std::endl;
			exit(1);
		}
	 }


	} 
	catch(exception& e){
	  cout << e.what() << endl ;
	}

      }//for NMCContributions

      if(pP == 0) {
        if( flag.bitSet( LCIO::CHBIT_LONG ) ){
       	  fprintf(hitFile,
		"%d %d %d %3d %3d %2d %7.2f %7.2f %7.2f %15e %d %d %d %d\n",
		pP,
		(int)(idDecoder(hit)["S-1"]) + 1,
		0,
		(int)(idDecoder(hit)["I"]),
		(int)(idDecoder(hit)["J"]),
		(int)(idDecoder(hit)["K"]),
		(double)(hit->getPosition()[0]) ,
		(double)(hit->getPosition()[1]) ,
		(double)(hit->getPosition()[2]) ,
		(double)(hit->getEnergy()) ,
		id0,
		id1,
		dummyAsciiFlag,
		(int)(PIDList.size())
		) ;
        } else{
	  fprintf(hitFile,
		"%d %d %d %3d %3d %2d "
		"|    no position available         | %15e %d %d %d %d\n",
		pP,
		(int)(idDecoder(hit)["S-1"]) + 1,
		0,
		(int)(idDecoder(hit)["I"]),
		(int)(idDecoder(hit)["J"]),
		(int)(idDecoder(hit)["K"]),
		(double)(hit->getEnergy()) ,
		id0,
		id1,
		dummyAsciiFlag,
		(int)(PIDList.size())
		) ;
        }
      }
      else 
        if( flag.bitSet( LCIO::CHBIT_LONG ) ){
	  fprintf(hitFile,
		"%d %d %d %3d %3d %2d %7.2f %7.2f %7.2f %15e %d %d %d %d\n",
		pP,
		(int)(idDecoder(hit)["S-1"]) + 1,
		(int)(idDecoder(hit)["M"]),
		(int)(idDecoder(hit)["I"]),
		(int)(idDecoder(hit)["J"]),
		(int)(idDecoder(hit)["K-1"]) + 1,
		(double)(hit->getPosition()[0]) ,
		(double)(hit->getPosition()[1]) ,
		(double)(hit->getPosition()[2]) ,
		(double)(hit->getEnergy()) ,
		id0,
		id1,
		dummyAsciiFlag,
		(int)(PIDList.size())
		) ;
        } else{
	  fprintf(hitFile,
		"%d %d %d %3d %3d %2d "
		"|    no position available         | %15e %d %d %d %d\n",
		pP,
		(int)(idDecoder(hit)["S-1"]) + 1,
		(int)(idDecoder(hit)["M"]),
		(int)(idDecoder(hit)["I"]),
		(int)(idDecoder(hit)["J"]),
		(int)(idDecoder(hit)["K-1"]) + 1,
		(double)(hit->getEnergy()) ,
		id0,
		id1,
		dummyAsciiFlag,
		(int)(PIDList.size())
		) ;
         }

   for(mapPID2PDG::iterator q = PIDList.begin(); q != PIDList.end(); q++) {
	
        double etotPID = 0;
	for(mapPDGEdep::iterator qq = q->second->begin(); 
		qq != q->second->end(); qq++)
		
		etotPID += qq->second;
	fprintf(hitFile, "%d %15e %d\n", (int)(q->first),etotPID,(int)(q->second->size()));
	
	for(mapPDGEdep::iterator qq = q->second->begin(); 
		qq != q->second->end(); qq++)
		
		fprintf(hitFile, "%d %15e\n",(int)(qq->first),(double)(qq->second));
	
   }


  }//for nPrint
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
  }

  void tools::printCalorimeterHits(const EVENT::LCCollection* col ){

    if( col->getTypeName() != LCIO::CALORIMETERHIT ){

      cout << " collection not of type " << LCIO::CALORIMETERHIT << endl ;
      return ;
    }

    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::CALORIMETERHIT << " collection "
	 << "--------------- " << endl ;

    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
 
    printParameters( col->getParameters() ) ;

    LCFlagImpl flag( col->getFlag() ) ;
    cout << "  -> LCIO::RCHBIT_LONG   : " << flag.bitSet( LCIO::RCHBIT_LONG ) << endl ;
    cout << "     LCIO::RCHBIT_BARREL : " << flag.bitSet( LCIO::RCHBIT_BARREL ) << endl ;
    cout << "     LCIO::RCHBIT_ID1    : " << flag.bitSet( LCIO::RCHBIT_ID1 ) << endl ;
    cout << "     LCIO::RCHBIT_TIME   : " << flag.bitSet( LCIO::RCHBIT_TIME ) << endl ;
    cout << "     LCIO::RCHBIT_NO_PTR : " << flag.bitSet( LCIO::RCHBIT_NO_PTR ) << endl ;
    cout << "     LCIO::RCHBIT_ENERGY_ERROR  : " << flag.bitSet( LCIO::RCHBIT_ENERGY_ERROR ) << endl ;

    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;

    CellIDDecoder<CalorimeterHit> idDecoder( col ) ; 

//     std::cout << "Note: ( M, S, I, J, K) are decoded using the Mokka convention ! " << std::endl ; 


    std::cout << endl
	      << " [   id   ] |  cellId0 | cellId1  |   energy  | energyerr |        position (x,y,z)          |"
	      << endl ;

    for( int i=0 ; i< nPrint ; i++ ){
      
      CalorimeterHit* hit = 
	dynamic_cast<CalorimeterHit*>( col->getElementAt( i ) ) ;
      
      int id0 = hit->getCellID0() ;
      int id1 = hit->getCellID1() ;
      
	printf( " [%8.8x] | %8.8x | %8.8x | %5.3e | " , 
		hit->id(), 
		id0,
		id1,
		hit->getEnergy()
		) ;

	//if( flag.bitSet( LCIO::RCHBIT_ENERGY_ERROR ) ){
        printf( "%5.3e | ", hit->getEnergyError() );
	//}
	//else{
        //printf( "  - NA -  | " );
	//}

      if( flag.bitSet( LCIO::CHBIT_LONG ) ){
	printf( "(%5.3e,%5.3e,%5.3e)|\n" , 
		hit->getPosition()[0] ,
		hit->getPosition()[1] ,
		hit->getPosition()[2]
		) ;
      } else{
	printf( "    no position available         \n") ;
      }
      std::cout << "        id-fields: (" << idDecoder( hit ).valueString() << ")" << std::endl ; 


    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
  }
  void tools::printRawCalorimeterHits(const EVENT::LCCollection* col ){

    if( col->getTypeName() != LCIO::RAWCALORIMETERHIT ){

      cout << " collection not of type " << LCIO::RAWCALORIMETERHIT << endl ;
      return ;
    }

    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::RAWCALORIMETERHIT << " collection "
	 << "--------------- " << endl ;

    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
 
    printParameters( col->getParameters() ) ;

    LCFlagImpl flag( col->getFlag() ) ;

    cout << "     LCIO::RCHBIT_ID1    : " << flag.bitSet( LCIO::RCHBIT_ID1 ) << endl ;
    cout << "     LCIO::RCHBIT_TIME   : " << flag.bitSet( LCIO::RCHBIT_TIME ) << endl ;
    cout << "     LCIO::RCHBIT_NO_PTR : " << flag.bitSet( LCIO::RCHBIT_NO_PTR ) << endl ;

    int nHits =  col->getNumberOfElements() ;
    int nPrint = nHits > MAX_HITS ? MAX_HITS : nHits ;


    CellIDDecoder<RawCalorimeterHit> idDecoder( col ) ; 

    std::cout << endl
	      << " [   id   ] |  cellId0 ( M, S, I, J, K) | cellId1  | amplitude |  time  "
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      RawCalorimeterHit* hit = 
	dynamic_cast<RawCalorimeterHit*>( col->getElementAt( i ) ) ;
      
      int id0 = hit->getCellID0() ;
      int id1 = hit->getCellID1() ;
      
      printf( " [%8.8x] | %8.8x | %8.8x |"
	      " %10d |  %10d \n" , 
	      hit->id(), 
	      id0,
	      id1,
	      hit->getAmplitude() ,
	      hit->getTimeStamp()
	      ) ;
      std::cout << "        id-fields: (" << idDecoder( hit ).valueString() << ")" << std::endl ; 



    }
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
  }

  void tools::printClusters(const EVENT::LCCollection* col ){
    if( col->getTypeName() != LCIO::CLUSTER ){
      
      cout << " collection not of type " << LCIO::CLUSTER << endl ;
      return ;
    }
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::CLUSTER << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
    
    printParameters( col->getParameters() ) ;

    LCFlagImpl flag( col->getFlag() ) ;
    cout << "     LCIO::CLBIT_HITS : " << flag.bitSet( LCIO::CLBIT_HITS ) << endl ;
    
    int nClusters =  col->getNumberOfElements() ;
    int nPrint = nClusters > MAX_HITS ? MAX_HITS : nClusters ;
    
    std::cout << endl
	      << " [   id   ] |  type | energy    |      position ( x,y,z)          |  itheta   |  iphi "
//    | EMweight |HADweight |MUweight"
	      << endl	      
	      << "------------|-------|-----------|---------------------------------|----------|----------|----------|----------|---------"
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      Cluster* clu = 
      	dynamic_cast<Cluster*>( col->getElementAt( i ) ) ;
      
      printf(" [%8.8x] |0x%5x| %5.3e | (%5.3e,%5.3e,%5.3e) | %4.2e | %4.2e \n"// | %4.2e | %4.2e | %4.2e \n"
	     , clu->id()
	     , clu->getType() 
	     , clu->getEnergy() 
	     , clu->getPosition()[0]
	     , clu->getPosition()[1]
	     , clu->getPosition()[2]
	     , clu->getITheta() 
	     , clu->getIPhi()
// 	     , clu->getParticleType()[0] 
// 	     , clu->getParticleType()[1] 
// 	     , clu->getParticleType()[2] 
	     ) ;
      cout << " errors (6 pos)/( 3 dir): (" ;
      for(int l=0;l<6;l++){
	printf("%4.2e, ", clu->getPositionError()[l] ) ; 
      }
      cout << ")/("  ;
      for(int l=0;l<3;l++){
	printf("%4.2e, ", clu->getDirectionError()[l] ) ; 
      }
      cout << ")" << endl ;
      cout << " clusters(e): " ;
      const ClusterVec& clusters = clu->getClusters() ;

      for(unsigned int l=0;l<clusters.size();l++){
		printf("%4.2e, ",  clusters[l]->getEnergy() ) ; 
      }
      cout << endl ;
      if( flag.bitSet( LCIO::CLBIT_HITS ) ) {
	cout << " hits ->";
// 	const StringVec& hitColNames = clu->getHitCollectionNames() ;
// 	for(unsigned int j=0;j<hitColNames.size();j++){
// 	  cout << " " << hitColNames[j] << ": " ;

// 	  const IntVec& hits = clu->getHitIndicesForCollection( hitColNames[j] ) ;
// 	  const FloatVec& wgts = clu->getHitContributionsForCollection( hitColNames[j] ) ;

// 	  for(unsigned int k=0;k<hits.size();k++){
// 	    cout << hits[k] <<" ["<< wgts[k] <<"], " ;
// 	  }
// 	}

	const CalorimeterHitVec& hits= clu->getCalorimeterHits() ; 
	const FloatVec& contr = clu->getHitContributions() ; 
	for(unsigned int k=0;k<hits.size();k++){
	  printf( " [%8.8x] (%4.3e), " , hits[k]->id(), contr[k] ) ;
	  // 	  cout << "0x" << hits[k]  << "(" << contr[k] << "), " ;
	}
	cout << dec << endl  ;


      }
      cout << endl 
	   << "-------|-----------|---------------------------------|----------|----------|----------|----------|---------"
	   << endl ;
    }


    // --- detailed PID info:
    
    cout <<  endl 
	 << "  ------------ detailed PID info: --- " <<   endl  <<   endl 
	 << "   algorithms : " 
	 <<   endl ;
    
    
    PIDHandler pidH( col )  ;
    
    const IntVec& ids =  pidH.getAlgorithmIDs() ;

    for(unsigned i=0; i<ids.size() ; ++i){

      cout << "   [id: " << ids[i] << "]   " 
	   <<  pidH.getAlgorithmName( ids[i] ) 
	   << " - params: " ;
      
      const StringVec& pNames = pidH.getParameterNames( ids[i] ) ;
 
      for( StringVec::const_iterator it = pNames.begin() ; it != pNames.end() ; ++it ){

	cout << " " << *it  ;
      }
      cout << endl ;
    }
    cout << endl ;

    std::cout << endl
	      << "   [cluster ] |  PDG   | likelihood |  type  |  algoId  | parameters : " << endl
	      << "              |        |            |        |          |              "
      	      << endl ;


    for( int i=0 ; i< nPrint ; i++ ){
      
      Cluster* clu = 
	dynamic_cast<Cluster*>( col->getElementAt( i ) ) ;
      
      printf("   [%8.8x] " , clu->id() ) ;
      
      
      for(unsigned int l=0;l<clu->getParticleIDs().size();l++){
	
	if( l!=0)
	  printf("              " ) ;

	ParticleID* pid = clu->getParticleIDs()[l] ;
	
	printf("| %6d | %6.4e | %6.6d | %8d | [",  
	       pid->getPDG() , 
	       pid->getLikelihood()  ,
	       pid->getType() ,
	       pid->getAlgorithmType() 
	       ) ;

    try{
        const StringVec& pNames = pidH.getParameterNames(  pid->getAlgorithmType() ) ;
        
        for(unsigned j=0;j< pNames.size() ;++j){

          cout << " " <<  pNames[j]
               << " : " <<  pid->getParameters()[j] << "," ; 

        }
    }
    catch( UnknownAlgorithm &e ){
        cout << "- NA - ";
    }
    
	cout << "]"<< endl ;
	

      }
      cout << endl ;
      
    }
    
    
    
      cout << endl 
	   << "-------------------------------------------------------------------------------- " 
	   << endl ;
  }

  void tools::printVertices( const EVENT::LCCollection* col ){
    if( col->getTypeName() != LCIO::VERTEX ){
      
      cout << " collection not of type " << LCIO::VERTEX << endl ;
      return ;
    }
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::VERTEX << " collection "
	 << "--------------- " << endl << endl;
    
    printParameters( col->getParameters() ) ;
    
    int nVertices = col->getNumberOfElements() ;
    int nPrint = nVertices > MAX_HITS ? MAX_HITS : nVertices ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      Vertex* v = dynamic_cast<Vertex*>( col->getElementAt( i ) ) ;
      
      if( i==0) { cout<<header(v); }
      //cout<<lcshort(v,col);
      cout<<lcshort(v);
      if(i==nPrint-1){ cout<<tail(v); }
    }
  }

  void tools::printReconstructedParticles( const EVENT::LCCollection* col ){

    if( col->getTypeName() != LCIO::RECONSTRUCTEDPARTICLE ){
      
      cout << " collection not of type " << LCIO::RECONSTRUCTEDPARTICLE << endl ;
      return ;
    }
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::RECONSTRUCTEDPARTICLE << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
    
    printParameters( col->getParameters() ) ;

//     LCFlagImpl flag( col->getFlag() ) ;
//     cout << "     LCIO::CLBIT_HITS : " << flag.bitSet( LCIO::CLBIT_HITS ) << endl ;
    
    int nReconstructedParticles =  col->getNumberOfElements() ;
    int nPrint = nReconstructedParticles > MAX_HITS ? MAX_HITS : nReconstructedParticles ;
    
    std::cout << endl
	      << " [   id   ] |com|type|     momentum( px,py,pz)         | energy   | mass     | charge    |          position ( x,y,z)       | [pidUsed] "
	      << endl	      
	      << "  ----------|---|----|---------------------------------|----------|----------|-----------|----------------------------------|-----------"
	      << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
// #ifdef USE_CLHEP
//       ReconstructedParticle4V recP( col->getElementAt( i ) ) ;
// #else
      ReconstructedParticle* recP = 
      	dynamic_cast<ReconstructedParticle*>( col->getElementAt( i ) ) ;
// #endif

      
      int compound = recP->isCompound() ;
      int type =  recP->getType() ;


      int pidUsed = 0 ;

      if(  recP->getParticleIDUsed() != 0 ) 
	pidUsed  = recP->getParticleIDUsed()->id() ;
      
      printf(" [%8.8x] | %1d | %2d | (%5.3e,%5.3e,%5.3e) | %4.2e | %4.2e | %4.2e | (%5.3e,%5.3e,%5.3e) | [%8.8x]\n"
	     //	     , reinterpret_cast<int> ( recP )
	     , recP->id()
	     , compound
	     , type
	     , recP->getMomentum()[0]
	     , recP->getMomentum()[1]
	     , recP->getMomentum()[2]
	     , recP->getEnergy() 
	     , recP->getMass() 
	     , recP->getCharge()
	     , recP->getReferencePoint()[0] 
	     , recP->getReferencePoint()[1] 
	     , recP->getReferencePoint()[2] 
	     , pidUsed
	     //, (recP->getStartVertex()!=NULL?recP->getStartVertex()->id():0)
	     //, (recP->getEndVertex()!=NULL?recP->getEndVertex()->id():0)
	     ) ;
      cout << "    covariance( px,py,pz,E) : (" ;
      for(int l=0;l<10;l++){
 	printf("%4.2e, ", recP->getCovMatrix()[l] ) ; 
      }
      cout << ")" << endl ;

      cout << "    particles ( [   id   ] ):" ;
      for(unsigned int l=0;l<recP->getParticles().size();l++){
	printf("[%8.8x], ", recP->getParticles()[l]->id() ) ;
	//,  recP->getParticleWeights()[l]  ) ; 
      }
      cout << endl ;
      cout << "    tracks ( [   id   ] ): " ;
      for(unsigned int l=0;l<recP->getTracks().size();l++){
	printf("[%8.8x], ",  recP->getTracks()[l]->id() );
	//,	       recP->getTrackWeights()[l]  ) ; 
      }
      cout << endl ;
      cout << "    clusters ( [   id   ] ): " ;
      for(unsigned int l=0;l<recP->getClusters().size();l++){
	printf("[%8.8x], ",  recP->getClusters()[l]->id() );
	//,       recP->getClusterWeights()[l]  ) ; 
      }
      cout << endl ;
      cout << "    particle ids ( [id], PDG, (type)): " ;
      for(unsigned int l=0;l<recP->getParticleIDs().size();l++){
	ParticleID* pid = recP->getParticleIDs()[l] ;
	printf("[%8.8x], %6.6d, (%6.6d)  ",  pid->id() , pid->getPDG() , pid->getType() ) ;
      }
      cout << endl ;
      
      Vertex* sv = dynamic_cast<Vertex*>(recP->getStartVertex());
      Vertex* ev = dynamic_cast<Vertex*>(recP->getEndVertex());
      ReconstructedParticle* svr=0;
      ReconstructedParticle* evr=0;
      
      if(sv!=0){
         svr = dynamic_cast<ReconstructedParticle*>(sv->getAssociatedParticle());
      }
      if(ev!=0){
         evr = dynamic_cast<ReconstructedParticle*>(ev->getAssociatedParticle());
      }
      
      printf("    vertices: startVertex( id:[%8.8x], id_aRP:[%8.8x] )   endVertex( id:[%8.8x], id_aRP:[%8.8x] ) "
        , ( sv != 0 ? sv->id() : 0 )
        , ((sv != 0 && svr != 0) ? svr->id() : 0 )
        , ( ev != 0 ? ev->id() : 0 )
        , ((ev != 0 && evr != 0) ? evr->id() : 0 )
      ) ;
     
      cout << endl ;

//       cout << "    MCParticles ( [   id   ] (weight) ): " ;
//       for(unsigned int l=0;l<recP->getMCParticles().size();l++){
// 	printf("[%8.8x] (%f), ",  recP->getMCParticles()[l]->id() ,
// 	       recP->getMCParticleWeights()[l]  ) ; 
//       }
      cout << endl ;

//       cout << endl ;
//       if( flag.bitSet( LCIO::CLBIT_HITS ) ) {
// 	cout << " hits ->";
// 	const StringVec& hitColNames = recP->getHitCollectionNames() ;
// 	for(unsigned int j=0;j<hitColNames.size();j++){
// 	  cout << " " << hitColNames[j] << ": " ;

// 	  const IntVec& hits = recP->getHitIndicesForCollection( hitColNames[j] ) ;
// 	  const FloatVec& wgts = recP->getHitContributionsForCollection( hitColNames[j] ) ;

// 	  for(unsigned int k=0;k<hits.size();k++){
// 	    cout << hits[k] <<" ["<< wgts[k] <<"], " ;
// 	  }
// 	}
//       }
      cout  << "------------|---|----|---------------------------------|----------|----------|-----------|---------------------------------|-----------|"
	    << endl ;
    }
    // --- detailed PID info:
    
    cout <<  endl 
	 << "  ------------ detailed PID info: --- " <<   endl  <<   endl 
	 << "   algorithms : " 
	 <<   endl ;
    
    
    PIDHandler pidH( col )  ;
    
    const IntVec& ids =  pidH.getAlgorithmIDs() ;

    for(unsigned i=0; i<ids.size() ; ++i){

      cout << "   [id: " << ids[i] << "]   " 
	   <<  pidH.getAlgorithmName( ids[i] ) 
	   << " - params: " ;
      
      const StringVec& pNames = pidH.getParameterNames( ids[i] ) ;
 
      for( StringVec::const_iterator it = pNames.begin() ; it != pNames.end() ; ++it ){

	cout << " " << *it  ;
      }
      cout << endl ;
    }
    cout << endl ;

    std::cout << endl
	      << "   [particle] |  PDG   | likelihood |  type  |  algoId  | parameters : " << endl
	      << "              |        |            |        |          |              "
      	      << endl ;


    for( int i=0 ; i< nPrint ; i++ ){
      
      ReconstructedParticle* recP = 
	dynamic_cast<ReconstructedParticle*>( col->getElementAt( i ) ) ;
      
      printf("   [%8.8x] " , recP->id() ) ;
      
      
      for(unsigned int l=0;l<recP->getParticleIDs().size();l++){
	
	if( l!=0)
	  printf("              " ) ;

	ParticleID* pid = recP->getParticleIDs()[l] ;
	
	printf("| %6d | %6.4e | %6.6d | %8d | [",  
	       pid->getPDG() , 
	       pid->getLikelihood()  ,
	       pid->getType() ,
	       pid->getAlgorithmType() 
	       ) ;

	const StringVec& pNames = pidH.getParameterNames(  pid->getAlgorithmType() ) ;
	
	for(unsigned j=0;j< pNames.size() ;++j){

	  cout << " " <<  pNames[j]
	       << " : " <<  pid->getParameters()[j] << "," ; 

	}
	cout << "]"<< endl ;
	

      }
      cout << endl ;
      
    }
    
    
    
    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
    
  }

  void tools::printLCGenericObjects( const EVENT::LCCollection* col ){
    
    if( col->getTypeName() != LCIO::LCGENERICOBJECT ){
      
      cout << " collection not of type " << LCIO::LCGENERICOBJECT 
	   << " [ " <<  col->getParameters().getStringVal("TypeName") << " ] " 
	   << endl ;
      
      return ;
    }
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::LCGENERICOBJECT << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;

    printParameters( col->getParameters() ) ;

    int nLCGenericObjects =  col->getNumberOfElements() ;
    int nPrint = nLCGenericObjects > MAX_HITS ? MAX_HITS : nLCGenericObjects ;
    
    bool isFixedSize  =  LCFlagImpl( col->getFlag() ).bitSet( LCIO::GOBIT_FIXED ) ;
    
    cout << endl
	 << " [   id   ] " <<  col->getParameters().getStringVal("DataDescription") 
    	 << " - isFixedSize: "   <<  ( isFixedSize ? "true" : "false" )
	 << endl ;
    
    for( int i=0 ; i< nPrint ; i++ ){
      
      LCGenericObject* obj = dynamic_cast<LCGenericObject*> ( col->getElementAt(i) );
      
      printf("[%8.8x] ", obj->id() ) ;
      
      for(int j=0;j<obj->getNInt();j++)
	cout << "i:" << obj->getIntVal( j ) << "; " ;
      for(int j=0;j<obj->getNFloat();j++)
	cout << "f:" << obj->getFloatVal( j ) << "; " ;
      for(int j=0;j<obj->getNDouble();j++)
	cout << "d:" << obj->getDoubleVal( j ) << "; " ;
      
      cout << endl ;
      
    }
    cout << endl 
	   << "-------------------------------------------------------------------------------- " 
	   << endl ;
      
   
  }



  void tools::printMCParticles(const EVENT::LCCollection* col, FILE *kinFile ) {
    
    if( col->getTypeName() != LCIO::MCPARTICLE ){
      
      cout << " collection not of type " << LCIO::MCPARTICLE << endl ;
      return ;
    }
    
    cout << endl 
	 << "--------------- " << "converting "  << LCIO::MCPARTICLE << " collection "
	 << "--------------- " << endl ;
    
    int nParticles =  col->getNumberOfElements() ;


    // fill map with particle pointers and collection indices
    std::vector<MCParticle*> moms ;

    for( int k=0; k<nParticles; k++){

      MCParticle* part =  dynamic_cast<MCParticle*>( col->getElementAt( k ) ) ;
      p2i_map[ part ] = k ; 

      moms.push_back( part ) ;
    }


    cout << endl 
	 << "-------------------------- "  << " header : "
	 << "--------------------------- " << endl ;
    
  std::cout << endl
	    <<  "index|      PDG | vertex x,     y   ,   z     |    px,     py,        pz    |  charge | energy  |parent | endpoint x,    y  ,   z     |generator status |simulator status "    
	    << endl 
	    << endl ;
  


// loop over collection - preserve order
    for(  int index = 0 ; index < nParticles ; index++){
      
      MCParticle* part =  dynamic_cast<MCParticle*>( col->getElementAt( index ) ) ;

    fprintf(kinFile, "%d %d %15e %15e %15e %15e %15e %15e %15e %15e ",

	index+1,
	part->getPDG(),
	part->getVertex()[0] , 
	part->getVertex()[1] , 
	part->getVertex()[2] ,
	part->getMomentum()[0] ,
	part->getMomentum()[1] , 
	part->getMomentum()[2] ,
	part->getCharge() ,
	part->getEnergy());

      unsigned int nbPar = part->getParents().size();
      if(nbPar == 0)
    	fprintf(kinFile, "%d ",nbPar);
      else
      	for(unsigned int kP=0; kP<nbPar; kP++){
    	    fprintf(kinFile, "%d ",
		p2i_map[ part->getParents()[kP] ] + 1);
      	}

      fprintf(kinFile, "%15e %15e %15e %d %X\n",
	     part->getEndpoint()[0] , 
	     part->getEndpoint()[1] ,  
	     part->getEndpoint()[2] ,
		part->getGeneratorStatus() ,
		part->getSimulatorStatus() ); 

    }

    cout << endl 
	 << "-------------------------------------------------------------------------------- " 
	 << endl ;
  }

  void tools::printRelation( const EVENT::LCCollection* col ) {
    
    if( col->getTypeName() != LCIO::LCRELATION ){
      cout << " collection not of type " << LCIO::LCRELATION << endl ;
      return ;
    }
    
    cout << endl 
	 << "--------------- " << "print out of "  << LCIO::LCRELATION << " collection "
	 << "--------------- " << endl ;
    
    cout << endl 
	 << "  flag:  0x" << hex  << col->getFlag() << dec << endl ;
    
    printParameters( col->getParameters() ) ;

    int nRel =  col->getNumberOfElements() ;
    
    cout << " fromType : " << col->getParameters().getStringVal("FromType")  << endl ;
    cout << " toType : "   << col->getParameters().getStringVal("ToType")    << endl ;

    cout << endl <<  " |  [from_id]  |  [to_id]   | weight "  << endl ;
    

    for( int i=0; i < nRel ; i++ ){
	
      LCRelation* rel = dynamic_cast<LCRelation*>( col->getElementAt(i) ) ;
      printf(" | [%8.8x] |  [[%8.8x]   | %5.3e \n" 
	     , rel->getFrom()->id() 
	     , rel->getTo()->id() 
	     , rel->getWeight()
	     );
      
    }

  }


//   void tools::printRelation( const EVENT::LCRelation* rel_const ) {

//     LCRelation* rel = const_cast< LCRelation* >( rel_const )  ; 
    
//     cout << " relation from " << rel->getFromType() << " to " << rel->getToType() << endl ; 
    
//     int nRel = rel->numberOfRelations() ;
    
//     cout <<  " |  [from_id]  |  [to_id]   | weight "  << endl ;
    
//     // need to get a set of from objects first
//     set<LCObject*> objs ;
//     for( int i=0; i<nRel ; i++ ) {
//       objs.insert( rel->getRelation( i )  ) ;
//     } 

//     for( std::set< LCObject* >::const_iterator iter = objs.begin() ; iter != objs.end() ; iter++ ){

//       LCObject* from = *iter ;
      
//       for( int j=0; j < rel->numberOfRelations( from ) ; j++ ){
	
// 	printf(" | [%8.8x] |  [[%8.8x]   | %5.3e \n" 
// 	       , from->id() 
// 	       , rel->getRelation( from, j )->id()  
// 	       , rel->getWeight(  from, j )
// 	       );
	
//       }
      
//     }
//   }

  int tools::printDaughterParticles(const MCParticle* part, int index){

    int motherIndex = index - 1 ;
    // print this particles daughters
    for(unsigned int i=0; i<part->getDaughters().size();i++){
      
      MCParticle* d =  part->getDaughters()[i] ;

      cout << index++ << " [" << motherIndex << "] " 
	   <<  d->getPDG() << " | ("
	   <<  d->getMomentum()[0]  << ", "
	   <<  d->getMomentum()[1]  << ", "
	   <<  d->getMomentum()[2]  << ") | "
	   <<  part->getGeneratorStatus() << " | "
	   <<  part->getSimulatorStatus() << " | ("
	   <<  d->getVertex()[0]    << ", "
	   <<  d->getVertex()[1]    << ", "
	   <<  d->getVertex()[2]    << ") | (" ;
      if( part->getEndpoint() != 0 ){
	cout <<  part->getEndpoint()[0]  << ", "
	     <<  part->getEndpoint()[1]  << ", "
	     <<  part->getEndpoint()[2]  << ") | " ;
      }else{
	cout << " not set ) | " ; 
      }
      cout <<  d->getMass()         << " | " 
	   <<  d->getCharge()       << " | " 
	   <<  d->getEnergy()      
	   << endl ;	

      index = printDaughterParticles( part->getDaughters()[i] , index ) ;
    }
    return index ;

  }
  
  void tools::printParameters( const EVENT::LCParameters& params ){

    StringVec intKeys ;
    int nIntParameters = params.getIntKeys( intKeys ).size() ;
    for(int i=0; i< nIntParameters ; i++ ){
      IntVec intVec ;
      params.getIntVals(  intKeys[i], intVec ) ;
      int nInt  = intVec.size()  ;   
      cout << " parameter " << intKeys[i] << " [int]: " ; 
      
      if( nInt == 0 ){ 
	cout << " [empty] " << std::endl ;
      }
      for(int j=0; j< nInt ; j++ ){
	cout << intVec[j] << ", " ;
      }
      cout << endl ;
    }
    StringVec floatKeys ;
    int nFloatParameters = params.getFloatKeys( floatKeys ).size() ;
    for(int i=0; i< nFloatParameters ; i++ ){
      FloatVec floatVec ;
      params.getFloatVals(  floatKeys[i], floatVec ) ;
      int nFloat  = floatVec.size()  ;   
      cout << " parameter " << floatKeys[i] << " [float]: " ; 
      if( nFloat == 0 ){ 
	cout << " [empty] " << std::endl ;
      }
      for(int j=0; j< nFloat ; j++ ){
	cout << floatVec[j] << ", " ;
      }
      cout << endl ;
    }
    StringVec stringKeys ;
    int nStringParameters = params.getStringKeys( stringKeys ).size() ;
    for(int i=0; i< nStringParameters ; i++ ){
      StringVec stringVec ;
      params.getStringVals(  stringKeys[i], stringVec ) ;
      int nString  = stringVec.size()  ;   
      cout << " parameter " << stringKeys[i] << " [string]: " ; 
      if( nString == 0 ){ 
	cout << " [empty] " << std::endl ;
      }
      for(int j=0; j< nString ; j++ ){
	cout << stringVec[j] << ", " ;
      }
      cout << endl ;
    }

  }

  void tools::dumpRunHeader(const EVENT::LCRunHeader* run) {
    
    //    cout  << " --------------------------------------------------------------- " << endl ;
      
    cout  << "  Run : " << run->getRunNumber() 
	  << " - "      << run->getDetectorName() 
	  << ":  "      << run->getDescription()  << endl ;
    
    printParameters( const_cast<EVENT::LCRunHeader*>(run)->parameters() ) ;
    
    cout  << " --------------------------------------------------------------- " << endl ;
  }



  std::string tools::getSimulatorStatusString(MCParticle* mcp){
    
    if( mcp == 0  ) {
      
      std::stringstream str ;

      str << "simulator status bits: [sbvtcls] "
	  << " s: created in simulation"
	  << " b: backscatter"
	  << " v: vertex is not endpoint of parent" 
	  << " t: decayed in tracker"
	  << " c: decayed in calorimeter"
	  << " l: has left detector"
	  << " s: stopped"
	  << std::endl ;
      
      return str.str() ;
    }

    std::string s("[   0   ]") ;
    
    if( mcp->getSimulatorStatus() == 0 ) 

      return s ;
    
    if( mcp->isCreatedInSimulation() )
      s[1]='s' ;
    else
      s[1]=' ' ;
    if( mcp->isBackscatter() )
      s[2]='b' ;
    else
      s[2]=' ' ;
    if( mcp->vertexIsNotEndpointOfParent() )
      s[3]='v' ;
    else
      s[3]=' ' ;
    if( mcp->isDecayedInTracker() )
      s[4]='t' ;
    else
      s[4]=' ' ;
    if( mcp->isDecayedInCalorimeter() )
      s[5]='c' ;
    else
      s[5]=' ' ;
    if( mcp->hasLeftDetector() )
      s[6]='l' ;
    else
      s[6]=' ' ;
    if( mcp->isStopped() )
      s[7]='s' ;
    else
      s[7]=' ' ;
    
    return s ;
  }

} // namespace

