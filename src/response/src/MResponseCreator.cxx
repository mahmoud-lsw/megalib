/*
 * MResponseCreator.cxx
 *
 *
 * Copyright (C) by Andreas Zoglauer.
 * All rights reserved.
 *
 *
 * This code implementation is the intellectual property of
 * Andreas Zoglauer.
 *
 * By copying, distributing or modifying the Program (or any work
 * based on the Program) you indicate your acceptance of this statement,
 * and all its terms.
 *
 */


////////////////////////////////////////////////////////////////////////////////
//
// MResponseCreator
//
////////////////////////////////////////////////////////////////////////////////


// Include the header:
#include "MResponseCreator.h"

// Standard libs:
#include <iostream>
#include <sstream>
#include <limits>
using namespace std;

// ROOT libs:

// MEGAlib libs:
#include "MGlobal.h"
#include "MAssert.h"
#include "MStreams.h"
#include "MResponseMultipleCompton.h"
#include "MResponseMultipleComptonLens.h"
#include "MResponseFirstInteractionPosition.h"
#include "MResponseSpectral.h"
#include "MResponseImaging.h"
#include "MResponseImagingARM.h"
#include "MResponseImagingBinnedMode.h"
#include "MResponseImagingCodedMask.h"
#include "MResponseEarthHorizon.h"
#include "MResponseTracking.h"


////////////////////////////////////////////////////////////////////////////////


#ifdef ___CINT___
ClassImp(MResponseCreator)
#endif


////////////////////////////////////////////////////////////////////////////////


MResponseCreator::MResponseCreator()
{
  // Construct an instance of MResponseCreator

  m_Creator = 0;

  m_FileName = g_StringNotDefined;
  m_GeometryFileName = g_StringNotDefined;

  m_RevanCfgFileName = g_StringNotDefined;
  m_MimrecCfgFileName = g_StringNotDefined;

  m_ResponseName = g_StringNotDefined;

  m_MaxNEvents = numeric_limits<int>::max();
  m_SaveAfter = 100000;

  m_Mode = c_ModeUnknown;
  m_Verify = false;

  m_StartEvent = 0;
  m_MaxNEvents = numeric_limits<int>::max();

  m_NoAbsorptions = false;

  m_SaveAfter = 100000;
  
  m_Compress = false;
}


////////////////////////////////////////////////////////////////////////////////


MResponseCreator::~MResponseCreator()
{
  // Delete this instance of MResponseCreator
}


////////////////////////////////////////////////////////////////////////////////


bool MResponseCreator::ParseCommandLine(int argc, char** argv)
{
  ostringstream Usage;
  Usage<<endl;
  Usage<<"  Usage: responsecreator <options>"<<endl;
  Usage<<"    General options:"<<endl;
  Usage<<"      -g  --geometry        file  m  geometry file name"<<endl;
  Usage<<"      -f  --filename        file  m  file name"<<endl;  
  Usage<<"      -d  --debug                    enable debug"<<endl;
  Usage<<"      -r  --response-name   file     response file name"<<endl;
  Usage<<"      -v  --verify                   verify"<<endl;
  Usage<<"      -m  --mode            char  m  the modes are: "<<endl;
  Usage<<"                                         s  : spectral before reconstruction"<<endl;
  Usage<<"                                         t  : track"<<endl;
  Usage<<"                                         c  : compton"<<endl;
  Usage<<"                                         l  : lens"<<endl;
  Usage<<"                                         a  : ARM"<<endl;
  Usage<<"                                         il : list-mode imaging"<<endl;
  Usage<<"                                         ib : binned-mode imaging"<<endl;
  Usage<<"                                         ic : coded mask imaging"<<endl;
  Usage<<"                                         e  : earth horizon"<<endl;
  Usage<<"                                         f  : first interaction position"<<endl;
  Usage<<"      -i  --max-id          int      do the analysis up to id"<<endl;
  Usage<<"      -c  --revan-config    file     use this revan configuration file instead of defaults"<<endl;
  Usage<<"      -b  --mimrec-config   file     use this mimrec configuration file instead of defaults for the imaging response"<<endl;
  Usage<<"      -s  --save            int      save after this amount of entries"<<endl;
  Usage<<"          --no-absorptions           don't calculate absoption probabilities"<<endl;
  Usage<<"      -z                             gzip the generated files"<<endl;
  Usage<<"      -h  --help                     print this help"<<endl;
  Usage<<endl;

  string Option, SubOption;

  // Check for help
  for (int i = 1; i < argc; i++) {
    Option = argv[i];
    if (Option == "-h" || Option == "--help" || Option == "?" || Option == "-?") {
      cout<<Usage.str()<<endl;
      return false;
    }
  }

  // Now parse the command line options:
  for (int i = 1; i < argc; i++) {
    Option = argv[i];

    // First check if each option has sufficient arguments:
    // Single argument
    if (Option == "-f" || Option == "--filename" ||
        Option == "-g" || Option == "--geometry" ||
        Option == "-r" || Option == "--response-name" ||
        Option == "-m" || Option == "--mode" ||
        Option == "-i" || Option == "--max-id" ||
        Option == "-c" || Option == "--revan-config" ||
        Option == "-b" || Option == "--mimrec-config" ||
        Option == "-s" || Option == "--save") {
      if (!((argc > i+1) && argv[i+1][0] != '-')){
        cout<<"Error: Option "<<argv[i][1]<<" needs a second argument!"<<endl;
        cout<<Usage.str()<<endl;
        return false;
      }
    } 
    // Multiple arguments_
    //else if (Option == "-??") {
    //  if (!((argc > i+2) && argv[i+1][0] != '-' && argv[i+2][0] != '-')){
    //    cout<<"Error: Option "<<argv[i][1]<<" needs two arguments argument!"<<endl;
    //    cout<<Usage.str()<<endl;
    //    return false;
    //  }
    //}

    // Then fulfill the options:
    if (Option == "-f" || Option == "--filename") {
      m_FileName = argv[++i];
      cout<<"Accepting file name: "<<m_FileName<<endl;
    } else if (Option == "-g" || Option == "--geometry") {
      m_GeometryFileName = argv[++i];
      cout<<"Accepting geometry file name: "<<m_GeometryFileName<<endl;
    } else if (Option == "-c" || Option == "--revan-config") {
      m_RevanCfgFileName = argv[++i];
      cout<<"Accepting revan configuration file name: "<<m_RevanCfgFileName<<endl;
    } else if (Option == "-b" || Option == "--mimrec-config") {
      m_MimrecCfgFileName = argv[++i];
      cout<<"Accepting mimrec configuration file name: "<<m_MimrecCfgFileName<<endl;
    } else if (Option == "-r" || Option == "--response-name") {
      m_ResponseName = argv[++i];
      cout<<"Accepting response name: "<<m_ResponseName<<endl;
    } else if (Option == "-i" || Option == "--max-id") {
      m_MaxNEvents = atoi(argv[++i]);
      cout<<"Simulating up to max number of events: "<<m_MaxNEvents<<endl;
    } else if (Option == "-s" || Option == "--save") {
      m_SaveAfter = atoi(argv[++i]);
      cout<<"Saving after this amount of events: "<<m_SaveAfter<<endl;
    } else if (Option == "-m" || Option == "--mode") {
      SubOption = argv[++i];
      if (SubOption == "t") {
        m_Mode = c_ModeTracks;
        cout<<"Choosing track mode"<<endl;
      } else if (SubOption == "c") {
        m_Mode = c_ModeComptons;
        cout<<"Choosing Compton mode"<<endl;
      } else if (SubOption == "l") {
        m_Mode = c_ModeComptonsLens;
        cout<<"Choosing Compton lens mode"<<endl;
      } else if (SubOption == "s") {
        m_Mode = c_ModeSpectral;
        cout<<"Choosing spectral mode"<<endl;
      } else if (SubOption == "a") {
        m_Mode = c_ModeARM;
        cout<<"Choosing ARM mode"<<endl;
      } else if (SubOption == "il") {
        m_Mode = c_ModeImagingListMode;
        cout<<"Choosing list mode imaging mode"<<endl;
      } else if (SubOption == "ib") {
        m_Mode = c_ModeImagingBinnedMode;
        cout<<"Choosing binned imaging mode"<<endl;
      } else if (SubOption == "ic") {
        m_Mode = c_ModeImagingCodedMask;
        cout<<"Choosing coded mask maging mode"<<endl;
      } else if (SubOption == "e") {
        m_Mode = c_ModeEarthHorizon;
        cout<<"Choosing Earth Horizon mode"<<endl;
      } else if (SubOption == "f") {
        m_Mode = c_ModeFirstInteractionPosition;
        cout<<"Choosing First Interaction Position mode"<<endl;
      } else {
        cout<<"Error: Unknown suboption \""<<SubOption<<"\"!"<<endl;
        cout<<Usage.str()<<endl;
        return false;
      }
    } else if (Option == "-z") {
      m_Compress = true;
      cout<<"Choosing compression (gzip) mode"<<endl;
    } else if (Option == "-v") {
      m_Verify = true;
      cout<<"Choosing Verification mode"<<endl;
    } else if (Option == "--no-absorptions") {
      m_NoAbsorptions = true;
      cout<<"Calculating no absorptions"<<endl;
    } else if (Option == "-d") {
      if (g_Verbosity < 2) g_Verbosity = 2;
      cout<<"Enabling debug!"<<endl;
      mdebug<<"Debug enabled!"<<endl;
    } else {
      cout<<"Error: Unknown option \""<<Option<<"\"!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }
  }
  
  if (m_Mode == c_ModeUnknown) {
    cout<<"Error: you have to define a mode!"<<endl;
    cout<<Usage.str()<<endl;
    return false;
  }
  if (m_GeometryFileName == g_StringNotDefined) {
    cout<<"Error: No geometry file name given!"<<endl;
    cout<<Usage.str()<<endl;
    return false;
  }
  if (m_FileName == g_StringNotDefined) {
    cout<<"Error: No file name given!"<<endl;
    cout<<Usage.str()<<endl;
    return false;  
  }
  if (m_ResponseName == g_StringNotDefined) {
    cout<<"Error: No response name given!"<<endl;
    cout<<Usage.str()<<endl;
    return false;
  }

  // Launch the different response generators:
  if (m_Mode == c_ModeComptons) {
    if (m_RevanCfgFileName == g_StringNotDefined) {
      cout<<"Error: No revan configuration file name given!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }

    MResponseMultipleCompton Response;
    m_Creator = (&Response);

    // 
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);

    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);
    Response.SetDoAbsorptions(!m_NoAbsorptions);

    Response.CreateResponse();
  } else if (m_Mode == c_ModeComptonsLens) {
    if (m_RevanCfgFileName == g_StringNotDefined) {
      cout<<"Error: No revan configuration file name given!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }

    MResponseMultipleComptonLens Response;
    m_Creator = (&Response);
    // 
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);

    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);
    Response.SetDoAbsorptions(!m_NoAbsorptions);

    Response.CreateResponse();
  } else if (m_Mode == c_ModeTracks) {
    if (m_RevanCfgFileName == g_StringNotDefined) {
      cout<<"Error: No revan configuration file name given!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }

    MResponseTracking Response;
    m_Creator = (&Response);
    // 
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);

    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);

    Response.CreateResponse();

  } else if (m_Mode == c_ModeSpectral) {

    MResponseSpectral Response;
    m_Creator = (&Response);
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);
    
    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);
 
    Response.CreateResponse();
  } else if (m_Mode == c_ModeARM) {
    if (m_MimrecCfgFileName == g_StringNotDefined) {
      cout<<"Error: No mimrec configuration file name given!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }

    MResponseImagingARM Response;
    m_Creator = (&Response);
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);
    
    Response.SetMimrecConfigurationFileName(m_MimrecCfgFileName);
    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);
 
    Response.CreateResponse();
  } else if (m_Mode == c_ModeImagingListMode) {
    if (m_MimrecCfgFileName == g_StringNotDefined) {
      cout<<"Error: No mimrec configuration file name given!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }

    MResponseImaging Response;
    m_Creator = (&Response);
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);
    
    Response.SetMimrecConfigurationFileName(m_MimrecCfgFileName);
    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);
 
    Response.CreateResponse();
  } else if (m_Mode == c_ModeFirstInteractionPosition) {
    if (m_MimrecCfgFileName == g_StringNotDefined) {
      cout<<"Error: No mimrec configuration file name given!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }

    MResponseFirstInteractionPosition Response;
    m_Creator = (&Response);
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);
    
    Response.SetMimrecConfigurationFileName(m_MimrecCfgFileName);
    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);
 
    Response.CreateResponse();
  } else if (m_Mode == c_ModeImagingBinnedMode) {
    if (m_MimrecCfgFileName == g_StringNotDefined) {
      cout<<"Error: No mimrec configuration file name given!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }

    MResponseImagingBinnedMode Response;
    m_Creator = (&Response);
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);
    
    Response.SetMimrecConfigurationFileName(m_MimrecCfgFileName);
    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);
 
    Response.CreateResponse();
  } else if (m_Mode == c_ModeImagingCodedMask) {
    if (m_MimrecCfgFileName == g_StringNotDefined) {
      cout<<"Error: No mimrec configuration file name given!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }

    MResponseImagingCodedMask Response;
    m_Creator = (&Response);
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);
    
    Response.SetMimrecConfigurationFileName(m_MimrecCfgFileName);
    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);
 
    Response.CreateResponse();
  } else if (m_Mode == c_ModeEarthHorizon) {
    if (m_MimrecCfgFileName == g_StringNotDefined) {
      cout<<"Error: No mimrec configuration file name given!"<<endl;
      cout<<Usage.str()<<endl;
      return false;
    }

    MResponseEarthHorizon Response;
    m_Creator = (&Response);
    Response.SetSimulationFileName(m_FileName);
    Response.SetGeometryFileName(m_GeometryFileName);
    Response.SetResponseName(m_ResponseName);
    Response.SetCompression(m_Compress);
    // Response.SetStartEventID(0);
    Response.SetMaxNumberOfEvents(m_MaxNEvents);
    Response.SetSaveAfterNumberOfEvents(m_SaveAfter);
    
    Response.SetMimrecConfigurationFileName(m_MimrecCfgFileName);
    Response.SetRevanConfigurationFileName(m_RevanCfgFileName);
 
    Response.CreateResponse();
  }
  
  return true;
}


// MResponseCreator.cxx: the end...
////////////////////////////////////////////////////////////////////////////////
